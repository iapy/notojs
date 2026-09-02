import * as acorn from "./acorn.js";
import * as acornLoose from "./acorn-loose.js";

CodeMirror.notojs = CodeMirror.notojs || {};

CodeMirror.notojs.complete = new class {
    #keywords = `as async await break case catch class const continue default
        delete do else export extends false finally for from function if import in
        instanceof let new null of return static super switch this throw true try
        typeof var void while with yield`.split(/\s+/).sort();
    #globalLoads = new WeakMap();
    #globalState = new WeakMap();
    #editors = new WeakMap();
    #globalSource = `{
        const identifier = /^[A-Za-z_$][A-Za-z0-9_$]*$/;
        const names = Object.getOwnPropertyNames(globalThis).filter(name => identifier.test(name)).sort();
        const members = {};
        const properties = value => value !== null &&
            (typeof value === 'object' || typeof value === 'function')
                ? Object.getOwnPropertyNames(value).filter(member => identifier.test(member)).sort()
                : [];
        const inspect = (name, value) => {
            members[name] = properties(value);
            for(const member of members[name]) {
                try {
                    const descriptor = Object.getOwnPropertyDescriptor(value, member);
                    members[name + '.' + member] = properties(descriptor && descriptor.value);
                } catch(_) {
                    members[name + '.' + member] = [];
                }
            }
        };
        for(const name of names) {
            try {
                const descriptor = Object.getOwnPropertyDescriptor(globalThis, name);
                inspect(name, descriptor && descriptor.value);
            } catch(_) {
                members[name] = [];
            }
        }
        print({names, members});
    }`;
    #required = new Set();
    #requiredNames = new Set();
    #requiredMembers = {};
    #imports = new WeakMap();
    #indexes = new WeakMap();
    #timers = new WeakMap();

    constructor() {
        document.addEventListener("notoui.Changed", event => this.#changed(event));
        document.addEventListener("notoui.Connected", event => this.#connected(event));
        document.addEventListener("notoui.Disconnected", event => this.#disconnected(event));
    }

    init(cm) {
        const book = this.#book(cm);
        if(book) {
            let editors = this.#editors.get(book);
            if(!editors) this.#editors.set(book, editors = new Set());
            editors.add(cm);
        }
        cm.on("inputRead", (cm, change) => this.#inputRead(cm, change));
        cm.on("cursorActivity", cm => this.#inspectRequire(cm));
        cm.on("cursorActivity", cm => this.#inspectImport(cm));
        cm.on("focus", _ => this.#get_globals(cm));
        cm.on("blur", cm => this.#cancel(cm));
        this.#get_globals(cm);
    }

    context(cm) {
        const cursor = cm.getCursor();
        const token = cm.getTokenAt(cursor);
        const state = token.state;
        const line = cm.getLine(cursor.line).slice(0, cursor.ch);
        let language = "javascript";
        let suppressed = false;

        if(state?.inMarkdown) {
            language = "markdown";
            suppressed = true;
        } else if(state?.inTemplate) {
            language = state.templateKind || "javascript";
            suppressed = true;
        } else if(!state?.jsState) {
            language = cm.getOption("mode") || "plain";
            suppressed = language !== "javascript";
        }

        let kind = "identifier";
        let receiver = null;
        let match = line.match(/\b(?:from|import)\s*["']([^"']*)$/);
        if(match) {
            kind = "import-source";
            suppressed = language !== "javascript";
        } else if(match = line.match(/\brequire\s*\(\s*["']([^"']*)$/)) {
            kind = "require-source";
            suppressed = language !== "javascript";
        } else if(/\bimport\s*\{[^}]*$/.test(line)) {
            kind = "import-name";
            match = line.match(/[A-Za-z_$][\w$]*$/);
        } else {
            match = line.match(/(?:^|[^\w$])([A-Za-z_$][\w$]*(?:\.[A-Za-z_$][\w$]*)*)\.([A-Za-z_$][\w$]*)?$/);
            if(match) {
                kind = "member";
                receiver = match[1];
            } else match = line.match(/[A-Za-z_$][\w$]*$/);
        }

        const prefix = kind === "member" ? match?.[2] ?? ""
            : match?.[1] ?? match?.[0] ?? "";
        const type = token.type || "";
        if(kind !== "import-source" && kind !== "require-source" &&
           (type.includes("comment") || type.includes("string")))
            suppressed = true;

        return {
            language,
            kind,
            receiver,
            prefix,
            from: CodeMirror.Pos(cursor.line, cursor.ch - prefix.length),
            to: cursor,
            suppressed
        };
    }

    show(cm) {
        if(typeof cm.showHint !== "function") return CodeMirror.Pass;

        const context = this.context(cm);
        const source = context.kind === "import-source" || context.kind === "require-source";
        cm.showHint({
            hint: cm => this.#hint(cm),
            completeSingle: false,
            alignWithWord: true,
            closeCharacters: source ? /[\s()\[\]{};>,]/ : /[\s()\[\]{};:>,]/
        });
    }

    async #hint(cm) {
        const context = this.context(cm);
        let list = [];
        if(!context.suppressed && context.language === "javascript") {
            const prefix = context.prefix.toLowerCase();
            if(context.kind === "import-source" || context.kind === "require-source") {
                const packages = await window.connection.packages;
                const paths = packages[
                    context.kind === "import-source" ? "modules" : "scripts"
                ];
                list = paths.filter(path =>
                    path.toLowerCase().startsWith(prefix)
                ).sort((a, b) => a.localeCompare(b));
            } else if(context.kind === "member") {
                this.#get_globals(cm);
                list = this.#members(cm, context.receiver).filter(member =>
                    member.toLowerCase().startsWith(prefix)
                ).sort((a, b) => a.localeCompare(b));
            } else {
                this.#get_globals(cm);
                const symbols = this.#symbols(cm).filter(candidate =>
                    candidate.text.toLowerCase().startsWith(prefix)
                );
                const names = new Set(symbols.map(candidate => candidate.text));
                const globals = context.kind === "identifier" ? this.#globals(cm).filter(name =>
                    name.toLowerCase().startsWith(prefix) && !names.has(name)
                ).map(name => {
                    names.add(name);
                    return {
                        text: name,
                        displayText: `${name}  global`,
                        className: "cm-hint-global",
                        detail: "global",
                        render: this.#render
                    };
                }) : [];
                const keywords = this.#keywords.filter(candidate =>
                    candidate.startsWith(prefix) && !names.has(candidate)
                );
                list = this.#rank(symbols.concat(globals, keywords), context.prefix);
            }
        }
        list = list.filter(candidate => {
            const text = typeof candidate === "string" ? candidate : candidate.text;
            return !text.startsWith("__");
        });
        return {list, from: context.from, to: context.to};
    }

    #rank(candidates, prefix) {
        const folded = prefix.toLowerCase();
        return candidates.map((candidate, index) => {
            const text = typeof candidate === "string" ? candidate : candidate.text;
            const lower = text.toLowerCase();
            const rank = text === prefix ? 0
                : text.startsWith(prefix) ? 1
                : lower === folded ? 2
                : 3;
            return {candidate, index, rank};
        }).sort((a, b) => a.rank - b.rank || a.index - b.index)
          .map(entry => entry.candidate);
    }

    #book(cm) {
        return cm.getWrapperElement().closest("nj-cell")?.book;
    }

    #globals(cm) {
        const book = this.#book(cm);
        const names = this.#globalState.get(book)?.names || [];
        return [...new Set(names.concat([...this.#requiredNames]))];
    }

    #members(cm, name) {
        const imported = this.#imports.get(cm)?.members;
        if(imported && Object.prototype.hasOwnProperty.call(imported, name))
            return imported[name];

        const book = this.#book(cm);
        const members = this.#globalState.get(book)?.members?.[name] || [];
        return [...new Set(members.concat(this.#requiredMembers[name] || []))];
    }

    #changed(event) {
        if(event.detail?.source === "run") this.#refresh(event.target);
    }

    #connected(event) {
        const book = event.target;
        if(book?.localName !== "nj-book") return;

        for(const cm of this.#editors.get(book) || []) {
            this.#imports.delete(cm);
            this.#indexes.delete(cm);
        }
        this.#refresh(book);
    }

    #disconnected(event) {
        const book = event.target;
        if(book?.localName !== "nj-book") return;

        for(const cm of this.#editors.get(book) || []) {
            this.#imports.delete(cm);
            this.#indexes.delete(cm);
        }
        this.#globalState.delete(book);
    }

    #refresh(book) {
        if(book?.localName !== "nj-book") return;
        this.#globalState.delete(book);
        this.#load_globals(book).then(names => {
            if(!names.length) return;
            for(const cm of this.#editors.get(book) || []) {
                if(cm.state.completionActive) cm.state.completionActive.update();
            }
        });
    }

    #inspectRequire(cm) {
        const cursor = cm.getCursor();
        const line = cm.getLine(cursor.line).slice(0, cursor.ch);
        const match = line.match(/\brequire\s*\(\s*(["'])([^"']+)\1\s*\)\s*;?\s*$/);
        if(!match || this.#required.has(match[2])) return;

        const type = cm.getTokenTypeAt(CodeMirror.Pos(cursor.line, match.index + 1)) || "";
        if(type.includes("comment") || type.includes("string")) return;

        const script = match[2];
        this.#required.add(script);
        const source = `{
            const identifier = /^[A-Za-z_$][A-Za-z0-9_$]*$/;
            const properties = value => value !== null &&
                (typeof value === 'object' || typeof value === 'function')
                    ? Object.getOwnPropertyNames(value).filter(member => identifier.test(member)).sort()
                    : [];
            const value = name => {
                try {
                    const descriptor = Object.getOwnPropertyDescriptor(globalThis, name);
                    return descriptor && descriptor.value;
                } catch(_) {
                    return undefined;
                }
            };
            const inspect = (name, object, members) => {
                members[name] = properties(object);
                for(const member of members[name]) {
                    try {
                        const descriptor = Object.getOwnPropertyDescriptor(object, member);
                        members[name + '.' + member] = properties(descriptor && descriptor.value);
                    } catch(_) {
                        members[name + '.' + member] = [];
                    }
                }
            };
            const before = new Set(Object.getOwnPropertyNames(globalThis));
            const dollarBefore = new Set(properties(value('$')));
            require(${JSON.stringify(script)});
            const names = Object.getOwnPropertyNames(globalThis)
                .filter(name => !before.has(name) && identifier.test(name)).sort();
            const members = {};
            for(const name of names) inspect(name, value(name), members);

            if(before.has('$')) {
                const dollar = value('$');
                const changed = properties(dollar).filter(member => !dollarBefore.has(member));
                if(changed.length) {
                    names.push('$');
                    names.sort();
                    members.$ = changed;
                    for(const member of changed) {
                        try {
                            const descriptor = Object.getOwnPropertyDescriptor(dollar, member);
                            members['$.' + member] = properties(descriptor && descriptor.value);
                        } catch(_) {
                            members['$.' + member] = [];
                        }
                    }
                }
            }
            print({names, members});
        }`;

        window.connection.run(source).then(output => {
                const data = output.find(part => part.type === "notojs.Output")?.data?.[0]?.[0];
                if(!Array.isArray(data?.names) || !data.members || typeof data.members !== "object")
                    throw new Error(`Invalid globals response for ${script}`);

                for(const name of data.names) this.#requiredNames.add(name);
                for(const [name, members] of Object.entries(data.members)) {
                    this.#requiredMembers[name] = [...new Set(
                        (this.#requiredMembers[name] || []).concat(members || [])
                    )];
                }
                if(cm.state.completionActive) cm.state.completionActive.update();
            }).catch(_ => {});
    }

    #inspectImport(cm) {
        const cursor = cm.getCursor();
        const line = cm.getLine(cursor.line).slice(0, cursor.ch);
        const namespace = line.match(/\bimport\s*\*\s*as\s+([A-Za-z_$][\w$]*)\s+from\s*(["'])([^"']+)\2\s*;?\s*$/);
        const named = namespace ? null
            : line.match(/\bimport\s*\{([^}]*)\}\s*from\s*(["'])([^"']+)\2\s*;?\s*$/);
        const defaultImport = namespace || named ? null
            : line.match(/\bimport\s+([A-Za-z_$][\w$]*)\s+from\s*(["'])([^"']+)\2\s*;?\s*$/);
        const match = namespace || named || defaultImport;
        if(!match) return;

        const type = cm.getTokenTypeAt(CodeMirror.Pos(cursor.line, match.index + 1)) || "";
        if(type.includes("comment") || type.includes("string")) return;

        let statement;
        let bindings;
        let module;
        if(namespace) {
            bindings = [namespace[1]];
            module = namespace[3];
            statement = `import * as ${bindings[0]} from ${JSON.stringify(module)};`;
        } else if(named) {
            const specifiers = named[1].split(",").map(specifier => {
                const match = specifier.trim().match(/^([A-Za-z_$][\w$]*)(?:\s+as\s+([A-Za-z_$][\w$]*))?$/);
                return match ? {imported: match[1], local: match[2] || match[1]} : null;
            }).filter(Boolean);
            if(!specifiers.length) return;

            bindings = specifiers.map(specifier => specifier.local);
            module = named[3];
            statement = `import {${specifiers.map(specifier =>
                specifier.imported === specifier.local ? specifier.imported
                    : `${specifier.imported} as ${specifier.local}`
            ).join(", ")}} from ${JSON.stringify(module)};`;
        } else {
            bindings = [defaultImport[1]];
            module = defaultImport[3];
            statement = `import ${bindings[0]} from ${JSON.stringify(module)};`;
        }

        let imports = this.#imports.get(cm);
        if(!imports) {
            imports = {seen: new Set(), members: {}};
            this.#imports.set(cm, imports);
        }
        const key = `${module}\n${statement}`;
        if(imports.seen.has(key)) return;
        imports.seen.add(key);

        const values = bindings.map(binding => `[${JSON.stringify(binding)}, ${binding}]`).join(", ");
        const source = `${statement}
        {
            const identifier = /^[A-Za-z_$][A-Za-z0-9_$]*$/;
            const values = [${values}];
            const names = values.map(entry => entry[0]);
            const members = {};
            const properties = value => value !== null &&
                (typeof value === 'object' || typeof value === 'function')
                    ? Object.getOwnPropertyNames(value).filter(member => identifier.test(member)).sort()
                    : [];
            for(const entry of values) {
                const name = entry[0];
                const value = entry[1];
                members[name] = properties(value);
                for(const member of members[name]) {
                    try {
                        const descriptor = Object.getOwnPropertyDescriptor(value, member);
                        members[name + '.' + member] = properties(descriptor && descriptor.value);
                    } catch(_) {
                        members[name + '.' + member] = [];
                    }
                }
            }
            print({names, members});
        }`;

        window.connection.run(source).then(output => {
            const data = output.find(part => part.type === "notojs.Output")?.data?.[0]?.[0];
            if(!Array.isArray(data?.names) || !data.members || typeof data.members !== "object")
                throw new Error(`Invalid import response for ${module}`);

            for(const [name, members] of Object.entries(data.members))
                imports.members[name] = members || [];
            if(cm.state.completionActive) cm.state.completionActive.update();
            else if(cm.hasFocus()) {
                const context = this.context(cm);
                if(context.kind === "member" &&
                   Object.prototype.hasOwnProperty.call(data.members, context.receiver))
                    this.show(cm);
            }
        }).catch(_ => {});
    }


    #load_globals(book) {
        const kernel = book?.kernel;
        if(!kernel) return Promise.resolve([]);

        let globals = this.#globalState.get(book);
        if(globals?.kernel === kernel) {
            if(globals.request) return globals.request;
            if(globals.names) return Promise.resolve(globals.names);
        }

        globals = {kernel, request: null, names: null, members: null};
        this.#globalState.set(book, globals);
        const request = window.connection.run(this.#globalSource, kernel)
            .then(output => {
                if(!output.ok) throw new Error(`Failed to load globals: ${output.status}`);
                const data = output.find(part => part.type === "notojs.Output")?.data?.[0]?.[0];
                if(!Array.isArray(data?.names) || !data.members || typeof data.members !== "object")
                    throw new Error("Invalid globals response");
                return data;
            }).then(data => {
                if(this.#globalState.get(book) === globals && globals.request === request) {
                    globals.names = data.names;
                    globals.members = data.members;
                }
                return data.names;
            }).catch(_ => []).finally(() => {
                if(this.#globalState.get(book) === globals && globals.request === request)
                    globals.request = null;
            });
        globals.request = request;
        return request;
    }

    #get_globals(cm) {
        const book = this.#book(cm);
        const globals = this.#globalState.get(book);
        if(!book?.kernel || (globals?.kernel === book.kernel && globals.names)) return;

        const request = this.#load_globals(book);
        if(this.#globalLoads.get(cm) === request) return;
        this.#globalLoads.set(cm, request);
        request.then(names => {
            if(this.#globalLoads.get(cm) !== request) return;
            this.#globalLoads.delete(cm);
            if(!names.length) return;
            if(cm.state.completionActive) cm.state.completionActive.update();
            else if(cm.hasFocus() && this.context(cm).kind === "member")
                this.show(cm);
        });
    }

    #symbols(cm) {
        const source = cm.getValue();
        let index = this.#indexes.get(cm);
        if(!index || index.source !== source) {
            index = {source, root: null};
            try {
                const parse = acornLoose.parse || acorn.parse;
                if(parse) {
                    const ast = parse(this.#parseSource(cm), {
                        ecmaVersion: "latest",
                        sourceType: "module"
                    });
                    index.root = this.#scope(ast, null, "program");
                    this.#visit(ast, index.root);
                    this.#markdownBindings(cm, index.root);
                }
            } catch {
                index.root = null;
            }
            this.#indexes.set(cm, index);
        }
        if(!index.root) return [];

        const offset = cm.indexFromPos(cm.getCursor());
        let scope = this.#scopeAt(index.root, offset);
        const seen = new Set();
        const candidates = [];
        while(scope) {
            for(const binding of scope.bindings) {
                if(!binding.name || binding.name.includes("✖") || seen.has(binding.name)) continue;
                seen.add(binding.name);
                const detail = String(binding.detail || binding.kind).replaceAll("✖", "").trim() || binding.kind;
                candidates.push({
                    text: binding.name,
                    displayText: `${binding.name}  ${detail}`,
                    className: `cm-hint-${binding.kind}`,
                    detail,
                    render: this.#render
                });
            }
            scope = scope.parent;
        }
        return candidates;
    }

    #render(element, _, candidate) {
        const name = document.createElement("span");
        name.textContent = candidate.text;
        element.appendChild(name);

        const detail = document.createElement("span");
        detail.className = "CodeMirror-hint-detail";
        detail.textContent = candidate.detail;
        element.appendChild(detail);
    }

    #parseSource(cm) {
        const marker = /^(?:<\[(?:!?(?:[A-Za-z_$][A-Za-z0-9_$]*)?|:)\[|\]\]>)$/;
        const lines = [];
        for(let line = 0; line < cm.lineCount(); ++line) {
            const text = cm.getLine(line);
            const state = cm.getTokenAt(CodeMirror.Pos(line, text.length)).state;
            lines.push(state?.inMarkdown || marker.test(text.trim()) ? " ".repeat(text.length) : text);
        }
        return lines.join("\n");
    }

    #markdownBindings(cm, root) {
        const marker = /^<\[(!?)([A-Za-z_$][A-Za-z0-9_$]*)\[$/;
        for(let line = 0; line < cm.lineCount(); ++line) {
            const match = cm.getLine(line).match(marker);
            if(!match) continue;

            if(line > 0) {
                const previous = cm.getLine(line - 1);
                const state = cm.getTokenAt(CodeMirror.Pos(line - 1, previous.length)).state;
                if(state?.inMarkdown || state?.inCodeEcho || state?.inTemplate) continue;
            }

            const offset = cm.indexFromPos(CodeMirror.Pos(line, 0));
            this.#scopeAt(root, offset).bindings.push({
                name: match[2],
                kind: "const",
                detail: match[1] ? "export const" : "const"
            });
        }
    }

    #scope(node, parent, kind) {
        const scope = {
            start: node.start ?? 0,
            end: node.end ?? Infinity,
            kind,
            parent,
            bindings: [],
            children: []
        };
        if(parent) parent.children.push(scope);
        return scope;
    }

    #scopeAt(scope, offset) {
        for(const child of scope.children) {
            if(child.start <= offset && offset <= child.end)
                return this.#scopeAt(child, offset);
        }
        return scope;
    }

    #binding(scope, pattern, kind, detail = kind) {
        if(!pattern) return;
        switch(pattern.type) {
            case "Identifier":
                scope.bindings.push({name: pattern.name, kind, detail});
                break;
            case "AssignmentPattern":
                this.#binding(scope, pattern.left, kind, detail);
                break;
            case "RestElement":
                this.#binding(scope, pattern.argument, kind, detail);
                break;
            case "ArrayPattern":
                for(const element of pattern.elements)
                    this.#binding(scope, element, kind, detail);
                break;
            case "ObjectPattern":
                for(const property of pattern.properties)
                    this.#binding(scope, property.type === "RestElement" ? property : property.value, kind, detail);
                break;
        }
    }

    #visit(node, scope) {
        if(!node) return;
        if(Array.isArray(node)) {
            for(const child of node) this.#visit(child, scope);
            return;
        }

        switch(node.type) {
            case "Program":
                this.#visit(node.body, scope);
                return;
            case "ImportDeclaration": {
                const source = node.source?.value;
                const detail = source && source !== "✖" ? source : "import";
                for(const specifier of node.specifiers)
                    this.#binding(scope, specifier.local, "import", detail);
                return;
            }
            case "VariableDeclaration": {
                let target = scope;
                if(node.kind === "var")
                    while(target.parent && target.kind !== "function" && target.kind !== "program") target = target.parent;
                for(const declaration of node.declarations) {
                    this.#binding(target, declaration.id, node.kind);
                    this.#visit(declaration.init, scope);
                }
                return;
            }
            case "FunctionDeclaration":
                this.#binding(scope, node.id, "function");
                this.#function(node, scope);
                return;
            case "FunctionExpression":
            case "ArrowFunctionExpression":
                this.#function(node, scope);
                return;
            case "ClassDeclaration":
                this.#binding(scope, node.id, "class");
                this.#class(node, scope, false);
                return;
            case "ClassExpression":
                this.#class(node, scope, true);
                return;
            case "BlockStatement": {
                const block = this.#scope(node, scope, "block");
                this.#visit(node.body, block);
                return;
            }
            case "CatchClause": {
                const caught = this.#scope(node, scope, "catch");
                this.#binding(caught, node.param, "catch");
                this.#visit(node.body?.body, caught);
                return;
            }
            case "ForStatement":
            case "ForInStatement":
            case "ForOfStatement": {
                const loop = this.#scope(node, scope, "block");
                this.#visit(node.init || node.left, loop);
                this.#visit(node.test, loop);
                this.#visit(node.update, loop);
                this.#visit(node.right, loop);
                this.#visit(node.body, loop);
                return;
            }
            case "SwitchStatement": {
                const switched = this.#scope(node, scope, "block");
                this.#visit(node.discriminant, scope);
                this.#visit(node.cases, switched);
                return;
            }
            case "StaticBlock": {
                const block = this.#scope(node, scope, "block");
                this.#visit(node.body, block);
                return;
            }
        }

        for(const [key, child] of Object.entries(node)) {
            if(key === "type" || key === "start" || key === "end" || key === "loc" || key === "range") continue;
            if(child && typeof child === "object") this.#visit(child, scope);
        }
    }

    #function(node, parent) {
        const scope = this.#scope(node, parent, "function");
        if(node.type === "FunctionExpression") this.#binding(scope, node.id, "function");
        for(const parameter of node.params) {
            this.#binding(scope, parameter, "param");
            this.#visit(parameter, scope);
        }
        if(node.body?.type === "BlockStatement") this.#visit(node.body.body, scope);
        else this.#visit(node.body, scope);
    }

    #class(node, parent, expression) {
        const scope = this.#scope(node, parent, "class");
        if(expression) this.#binding(scope, node.id, "class");
        this.#visit(node.superClass, scope);
        this.#visit(node.body?.body, scope);
    }

    #inputRead(cm, change) {
        if(change.origin !== "+input" || cm.somethingSelected()) return;
        if(change.text?.length !== 1) return;
        if(!/^[A-Za-z_$]$/.test(change.text[0])) {
            const context = this.context(cm);
            const source = context.kind === "import-source" || context.kind === "require-source";
            const member = context.kind === "member" && change.text[0] === ".";
            if(context.suppressed || (!member &&
               (!source || !/^["'A-Za-z0-9_@:./$-]$/.test(change.text[0])))) return;
        }

        this.#cancel(cm);
        this.#timers.set(cm, setTimeout(() => {
            this.#timers.delete(cm);
            if(!cm.state.completionActive) this.show(cm);
        }, 125));
    }

    #cancel(cm) {
        const timer = this.#timers.get(cm);
        if(timer !== undefined) clearTimeout(timer);
        this.#timers.delete(cm);
    }
};

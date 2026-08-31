import * as acorn from "./acorn.js";
import * as acornLoose from "./acorn-loose.js";

CodeMirror.notojs = CodeMirror.notojs || {};

CodeMirror.notojs.complete = class Completion {
    static #keywords = `as async await break case catch class const continue default
        delete do else export extends false finally for from function if import in
        instanceof let new null of return static super switch this throw true try
        typeof var void while with yield`.split(/\s+/).sort();
    static #globalLoads = new WeakMap();
    static #globalState = new WeakMap();
    static #editors = new WeakMap();
    static #globalSource = `{
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
    static #pathLoad = null;
    static #paths = null;
    static #required = new Set();
    static #requiredNames = new Set();
    static #requiredMembers = {};
    static #imports = new WeakMap();
    static #indexes = new WeakMap();
    static #timers = new WeakMap();

    static listen() {
        document.addEventListener("notoui.Changed", Completion.#changed);
        document.addEventListener("notoui.Connected", Completion.#connected);
        document.addEventListener("notoui.Disconnected", Completion.#disconnected);
    }

    static init(cm) {
        const book = Completion.#book(cm);
        if(book) {
            let editors = Completion.#editors.get(book);
            if(!editors) Completion.#editors.set(book, editors = new Set());
            editors.add(cm);
        }
        cm.on("inputRead", Completion.#inputRead);
        cm.on("cursorActivity", Completion.#inspectRequire);
        cm.on("cursorActivity", Completion.#inspectImport);
        cm.on("focus", _ => Completion.#get_globals(cm));
        cm.on("blur", Completion.#cancel);
        Completion.#get_globals(cm);
        Completion.#getPaths(cm);
    }

    static context(cm) {
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

    static show(cm) {
        if(typeof cm.showHint !== "function") return CodeMirror.Pass;
        if(!Completion.#hint(cm).list.length) return CodeMirror.Pass;

        const context = Completion.context(cm);
        const source = context.kind === "import-source" || context.kind === "require-source";
        cm.showHint({
            hint: Completion.#hint,
            completeSingle: false,
            alignWithWord: true,
            closeCharacters: source ? /[\s()\[\]{};>,]/ : /[\s()\[\]{};:>,]/
        });
    }

    static #hint(cm) {
        const context = Completion.context(cm);
        let list = [];
        if(!context.suppressed && context.language === "javascript") {
            const prefix = context.prefix.toLowerCase();
            if(context.kind === "import-source" || context.kind === "require-source") {
                Completion.#getPaths(cm);
                const paths = Completion.#paths?.[
                    context.kind === "import-source" ? "modules" : "scripts"
                ] || [];
                list = paths.filter(path =>
                    path.toLowerCase().startsWith(prefix)
                ).sort((a, b) => a.localeCompare(b));
            } else if(context.kind === "member") {
                Completion.#get_globals(cm);
                list = Completion.#members(cm, context.receiver).filter(member =>
                    member.toLowerCase().startsWith(prefix)
                ).sort((a, b) => a.localeCompare(b));
            } else {
                Completion.#get_globals(cm);
                const symbols = Completion.#symbols(cm).filter(candidate =>
                    candidate.text.toLowerCase().startsWith(prefix)
                );
                const names = new Set(symbols.map(candidate => candidate.text));
                const globals = context.kind === "identifier" ? Completion.#globals(cm).filter(name =>
                    name.toLowerCase().startsWith(prefix) && !names.has(name)
                ).map(name => {
                    names.add(name);
                    return {
                        text: name,
                        displayText: `${name}  global`,
                        className: "cm-hint-global",
                        detail: "global",
                        render: Completion.#render
                    };
                }) : [];
                const keywords = Completion.#keywords.filter(candidate =>
                    candidate.startsWith(prefix) && !names.has(candidate)
                );
                list = Completion.#rank(symbols.concat(globals, keywords), context.prefix);
            }
        }
        list = list.filter(candidate => {
            const text = typeof candidate === "string" ? candidate : candidate.text;
            return !text.startsWith("__");
        });
        return {list, from: context.from, to: context.to};
    }

    static #rank(candidates, prefix) {
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

    static #book(cm) {
        return cm.getWrapperElement().closest("nj-cell")?.book;
    }

    static #globals(cm) {
        const book = Completion.#book(cm);
        const names = Completion.#globalState.get(book)?.names || [];
        return [...new Set(names.concat([...Completion.#requiredNames]))];
    }

    static #members(cm, name) {
        const imported = Completion.#imports.get(cm)?.members;
        if(imported && Object.prototype.hasOwnProperty.call(imported, name))
            return imported[name];

        const book = Completion.#book(cm);
        const members = Completion.#globalState.get(book)?.members?.[name] || [];
        return [...new Set(members.concat(Completion.#requiredMembers[name] || []))];
    }

    static #changed(event) {
        if(event.detail?.source === "run") Completion.#refresh(event.target);
    }

    static #connected(event) {
        Completion.#refresh(event.target);
    }

    static #disconnected(event) {
        if(event.target?.localName === "nj-book")
            Completion.#globalState.delete(event.target);
    }

    static #refresh(book) {
        if(book?.localName !== "nj-book") return;
        Completion.#globalState.delete(book);
        Completion.#load_globals(book).then(names => {
            if(!names.length) return;
            for(const cm of Completion.#editors.get(book) || []) {
                if(cm.state.completionActive) cm.state.completionActive.update();
            }
        });
    }

    static #inspectRequire(cm) {
        const cursor = cm.getCursor();
        const line = cm.getLine(cursor.line).slice(0, cursor.ch);
        const match = line.match(/\brequire\s*\(\s*(["'])([^"']+)\1\s*\)\s*;?\s*$/);
        if(!match || Completion.#required.has(match[2])) return;

        const type = cm.getTokenTypeAt(CodeMirror.Pos(cursor.line, match.index + 1)) || "";
        if(type.includes("comment") || type.includes("string")) return;

        const script = match[2];
        Completion.#required.add(script);
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

                for(const name of data.names) Completion.#requiredNames.add(name);
                for(const [name, members] of Object.entries(data.members)) {
                    Completion.#requiredMembers[name] = [...new Set(
                        (Completion.#requiredMembers[name] || []).concat(members || [])
                    )];
                }
                if(cm.state.completionActive) cm.state.completionActive.update();
            }).catch(_ => {});
    }

    static #inspectImport(cm) {
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

        let imports = Completion.#imports.get(cm);
        if(!imports) {
            imports = {seen: new Set(), members: {}};
            Completion.#imports.set(cm, imports);
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
                const context = Completion.context(cm);
                if(context.kind === "member" &&
                   Object.prototype.hasOwnProperty.call(data.members, context.receiver))
                    Completion.show(cm);
            }
        }).catch(_ => {});
    }

    static #getPaths(cm) {
        if(Completion.#paths) return Promise.resolve(Completion.#paths);

        if(!Completion.#pathLoad) {
            Completion.#pathLoad = fetch(`${window.connection.server}p`, {
                method: "PROPFIND"
            }).then(response => {
                if(!response.ok) throw new Error(`Failed to load paths: ${response.status}`);
                return response.json();
            }).then(paths => {
                Completion.#paths = {
                    modules: Array.isArray(paths?.modules)
                        ? paths.modules.filter(path => typeof path === "string") : [],
                    scripts: Array.isArray(paths?.scripts)
                        ? paths.scripts.filter(path => typeof path === "string") : []
                };
                return Completion.#paths;
            }).catch(_ => {
                Completion.#paths = {modules: [], scripts: []};
                return Completion.#paths;
            }).finally(() => Completion.#pathLoad = null);
        }

        return Completion.#pathLoad.then(paths => {
            if(cm.state.completionActive) cm.state.completionActive.update();
            return paths;
        });
    }

    static #load_globals(book) {
        const kernel = book?.kernel;
        if(!kernel) return Promise.resolve([]);

        let globals = Completion.#globalState.get(book);
        if(globals?.kernel === kernel) {
            if(globals.request) return globals.request;
            if(globals.names) return Promise.resolve(globals.names);
        }

        globals = {kernel, request: null, names: null, members: null};
        Completion.#globalState.set(book, globals);
        const request = window.connection.run(Completion.#globalSource, kernel)
            .then(output => {
                if(!output.ok) throw new Error(`Failed to load globals: ${output.status}`);
                const data = output.find(part => part.type === "notojs.Output")?.data?.[0]?.[0];
                if(!Array.isArray(data?.names) || !data.members || typeof data.members !== "object")
                    throw new Error("Invalid globals response");
                return data;
            }).then(data => {
                if(Completion.#globalState.get(book) === globals && globals.request === request) {
                    globals.names = data.names;
                    globals.members = data.members;
                }
                return data.names;
            }).catch(_ => []).finally(() => {
                if(Completion.#globalState.get(book) === globals && globals.request === request)
                    globals.request = null;
            });
        globals.request = request;
        return request;
    }

    static #get_globals(cm) {
        const book = Completion.#book(cm);
        const globals = Completion.#globalState.get(book);
        if(!book?.kernel || (globals?.kernel === book.kernel && globals.names)) return;

        const request = Completion.#load_globals(book);
        if(Completion.#globalLoads.get(cm) === request) return;
        Completion.#globalLoads.set(cm, request);
        request.then(names => {
            if(Completion.#globalLoads.get(cm) !== request) return;
            Completion.#globalLoads.delete(cm);
            if(!names.length) return;
            if(cm.state.completionActive) cm.state.completionActive.update();
            else if(cm.hasFocus() && Completion.context(cm).kind === "member")
                Completion.show(cm);
        });
    }

    static #symbols(cm) {
        const source = cm.getValue();
        let index = Completion.#indexes.get(cm);
        if(!index || index.source !== source) {
            index = {source, root: null};
            try {
                const parse = acornLoose.parse || acorn.parse;
                if(parse) {
                    const ast = parse(Completion.#parseSource(cm), {
                        ecmaVersion: "latest",
                        sourceType: "module"
                    });
                    index.root = Completion.#scope(ast, null, "program");
                    Completion.#visit(ast, index.root);
                    Completion.#markdownBindings(cm, index.root);
                }
            } catch {
                index.root = null;
            }
            Completion.#indexes.set(cm, index);
        }
        if(!index.root) return [];

        const offset = cm.indexFromPos(cm.getCursor());
        let scope = Completion.#scopeAt(index.root, offset);
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
                    render: Completion.#render
                });
            }
            scope = scope.parent;
        }
        return candidates;
    }

    static #render(element, _, candidate) {
        const name = document.createElement("span");
        name.textContent = candidate.text;
        element.appendChild(name);

        const detail = document.createElement("span");
        detail.className = "CodeMirror-hint-detail";
        detail.textContent = candidate.detail;
        element.appendChild(detail);
    }

    static #parseSource(cm) {
        const marker = /^(?:<\[(?:!?(?:[A-Za-z_$][A-Za-z0-9_$]*)?|:)\[|\]\]>)$/;
        const lines = [];
        for(let line = 0; line < cm.lineCount(); ++line) {
            const text = cm.getLine(line);
            const state = cm.getTokenAt(CodeMirror.Pos(line, text.length)).state;
            lines.push(state?.inMarkdown || marker.test(text.trim()) ? " ".repeat(text.length) : text);
        }
        return lines.join("\n");
    }

    static #markdownBindings(cm, root) {
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
            Completion.#scopeAt(root, offset).bindings.push({
                name: match[2],
                kind: "const",
                detail: match[1] ? "export const" : "const"
            });
        }
    }

    static #scope(node, parent, kind) {
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

    static #scopeAt(scope, offset) {
        for(const child of scope.children) {
            if(child.start <= offset && offset <= child.end)
                return Completion.#scopeAt(child, offset);
        }
        return scope;
    }

    static #binding(scope, pattern, kind, detail = kind) {
        if(!pattern) return;
        switch(pattern.type) {
            case "Identifier":
                scope.bindings.push({name: pattern.name, kind, detail});
                break;
            case "AssignmentPattern":
                Completion.#binding(scope, pattern.left, kind, detail);
                break;
            case "RestElement":
                Completion.#binding(scope, pattern.argument, kind, detail);
                break;
            case "ArrayPattern":
                for(const element of pattern.elements)
                    Completion.#binding(scope, element, kind, detail);
                break;
            case "ObjectPattern":
                for(const property of pattern.properties)
                    Completion.#binding(scope, property.type === "RestElement" ? property : property.value, kind, detail);
                break;
        }
    }

    static #visit(node, scope) {
        if(!node) return;
        if(Array.isArray(node)) {
            for(const child of node) Completion.#visit(child, scope);
            return;
        }

        switch(node.type) {
            case "Program":
                Completion.#visit(node.body, scope);
                return;
            case "ImportDeclaration": {
                const source = node.source?.value;
                const detail = source && source !== "✖" ? source : "import";
                for(const specifier of node.specifiers)
                    Completion.#binding(scope, specifier.local, "import", detail);
                return;
            }
            case "VariableDeclaration": {
                let target = scope;
                if(node.kind === "var")
                    while(target.parent && target.kind !== "function" && target.kind !== "program") target = target.parent;
                for(const declaration of node.declarations) {
                    Completion.#binding(target, declaration.id, node.kind);
                    Completion.#visit(declaration.init, scope);
                }
                return;
            }
            case "FunctionDeclaration":
                Completion.#binding(scope, node.id, "function");
                Completion.#function(node, scope);
                return;
            case "FunctionExpression":
            case "ArrowFunctionExpression":
                Completion.#function(node, scope);
                return;
            case "ClassDeclaration":
                Completion.#binding(scope, node.id, "class");
                Completion.#class(node, scope, false);
                return;
            case "ClassExpression":
                Completion.#class(node, scope, true);
                return;
            case "BlockStatement": {
                const block = Completion.#scope(node, scope, "block");
                Completion.#visit(node.body, block);
                return;
            }
            case "CatchClause": {
                const caught = Completion.#scope(node, scope, "catch");
                Completion.#binding(caught, node.param, "catch");
                Completion.#visit(node.body?.body, caught);
                return;
            }
            case "ForStatement":
            case "ForInStatement":
            case "ForOfStatement": {
                const loop = Completion.#scope(node, scope, "block");
                Completion.#visit(node.init || node.left, loop);
                Completion.#visit(node.test, loop);
                Completion.#visit(node.update, loop);
                Completion.#visit(node.right, loop);
                Completion.#visit(node.body, loop);
                return;
            }
            case "SwitchStatement": {
                const switched = Completion.#scope(node, scope, "block");
                Completion.#visit(node.discriminant, scope);
                Completion.#visit(node.cases, switched);
                return;
            }
            case "StaticBlock": {
                const block = Completion.#scope(node, scope, "block");
                Completion.#visit(node.body, block);
                return;
            }
        }

        for(const [key, child] of Object.entries(node)) {
            if(key === "type" || key === "start" || key === "end" || key === "loc" || key === "range") continue;
            if(child && typeof child === "object") Completion.#visit(child, scope);
        }
    }

    static #function(node, parent) {
        const scope = Completion.#scope(node, parent, "function");
        if(node.type === "FunctionExpression") Completion.#binding(scope, node.id, "function");
        for(const parameter of node.params) {
            Completion.#binding(scope, parameter, "param");
            Completion.#visit(parameter, scope);
        }
        if(node.body?.type === "BlockStatement") Completion.#visit(node.body.body, scope);
        else Completion.#visit(node.body, scope);
    }

    static #class(node, parent, expression) {
        const scope = Completion.#scope(node, parent, "class");
        if(expression) Completion.#binding(scope, node.id, "class");
        Completion.#visit(node.superClass, scope);
        Completion.#visit(node.body?.body, scope);
    }

    static #inputRead(cm, change) {
        if(change.origin !== "+input" || cm.somethingSelected()) return;
        if(change.text?.length !== 1) return;
        if(!/^[A-Za-z_$]$/.test(change.text[0])) {
            const context = Completion.context(cm);
            const source = context.kind === "import-source" || context.kind === "require-source";
            const member = context.kind === "member" && change.text[0] === ".";
            if(context.suppressed || (!member &&
               (!source || !/^["'A-Za-z0-9_@:./$-]$/.test(change.text[0])))) return;
        }

        Completion.#cancel(cm);
        Completion.#timers.set(cm, setTimeout(() => {
            Completion.#timers.delete(cm);
            if(!cm.state.completionActive) Completion.show(cm);
        }, 125));
    }

    static #cancel(cm) {
        const timer = Completion.#timers.get(cm);
        if(timer !== undefined) clearTimeout(timer);
        Completion.#timers.delete(cm);
    }
};

CodeMirror.notojs.complete.listen();

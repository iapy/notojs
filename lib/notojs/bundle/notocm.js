CodeMirror.defineMode("pkg", function () {
    function isWS(ch) { return ch === " " || ch === "\t"; }
    function allowedSection(line) {
        return line === "[modules]" || line === "[scripts]";
    }
    function validURL(s) {
        try {
            const u = URL.parse(s, s.startsWith('/') ? 'http://127.0.0.1/' : undefined);
            return u.protocol === 'http:' || u.protocol === 'https:';
        } catch {
            return false;
        }
    }

    const keyRe = /^[A-Za-z0-9_.@-]+$/;
    const fullLineRe = /^([A-Za-z0-9_.@-]+) = (.+)$/;

    return {
        startState() {
            return { inSection: false, phase: "sol", m: null };
        },

        token(stream, state) {
            if (stream.sol()) {
                state.phase = "sol";
                state.m = null;

                const first = stream.peek();
                if (first != null && isWS(first)) { stream.skipToEnd(); return "error"; }
                if (stream.eol()) return null;

                if (first === "[") {
                    const line = stream.string;
                    stream.skipToEnd();
                    state.inSection = allowedSection(line);
                    return state.inSection ? "header" : "error";
                }
                else if (first == '#') {
                    stream.skipToEnd();
                    return "comment";
                }

                if (!state.inSection) { stream.skipToEnd(); return "error"; }

                const line = stream.string;
                const m = line.match(fullLineRe);

                if (!m || !keyRe.test(m[1]) || !validURL(m[2])) {
                    stream.skipToEnd();
                    return "error";
                }

                state.m = m;
                state.phase = "key";
            }

            if (state.phase === "key") {
                stream.match(/^[A-Za-z0-9_.@-]+/);
                state.phase = "ws1";
                return "keyword";
            }

            if (state.phase === "ws1") {
                stream.next();
                state.phase = "eq";
                return null;
            }

            if (state.phase === "eq") {
                stream.next();
                state.phase = "ws2";
                return "operator";
            }

            if (state.phase === "ws2") {
                stream.next();
                state.phase = "value";
                return null;
            }

            if (state.phase === "value") {
                stream.skipToEnd();
                state.phase = "done";
                return "def";
            }

            stream.skipToEnd();
            return null;
        }
    };
});

CodeMirror.defineMode("js-md", config => {
    const jsMode = CodeMirror.getMode(config, "javascript");
    const mdMode = CodeMirror.getMode(config, "markdown");
    const htmlMode = CodeMirror.getMode(config, "htmlmixed");
    const xmlMode = CodeMirror.getMode(config, "xml");
    const mdMark = [/^<\[(?:!?(?:[a-zA-Z_$][a-zA-Z0-9_$]*)?|:)\[/, ']]>'];

    function tryTask(stream, state) {
        if (!stream.sol()) return false;

        state.mdLineTask = false;
        if (!state.inMarkdown) return false;

        const start = stream.pos;
        stream.eatWhile(ch => ch === " " || ch === "\t");

        let isList = false;
        if (stream.match(/(?:[-*+])(?:\s+|$)/, false)) {
            stream.match(/[-*+]/, true);
            stream.eatWhile(ch => ch === " " || ch === "\t");
            isList = true;
        } else if (stream.match(/\d+[.)](?:\s+|$)/, false)) {
            stream.match(/\d+[.)]/, true);
            stream.eatWhile(ch => ch === " " || ch === "\t");
            isList = true;
        }

        if (!isList) {
            stream.pos = start;
            return false;
        }

        if (stream.match(/\[(?: |x|X)\]/, false)) {
            state.mdLineTask = true;
        }

        stream.pos = start;
        return state.mdLineTask;
    }

    function tryBox(stream, state) {
        if (!state.inMarkdown) return false;
        if (!state.mdLineTask) return false;

        const start = stream.pos;
        stream.eatWhile(ch => ch === " " || ch === "\t");

        if (stream.match(/[-*+]/, false)) {
            stream.next();
            stream.eatWhile(ch => ch === " " || ch === "\t");
        } else if (stream.match(/\d+[.)]/, false)) {
            stream.match(/\d+[.)]/, true);
            stream.eatWhile(ch => ch === " " || ch === "\t");
        } else {
            stream.pos = start;
            state.mdLineTask = false;
            return false;
        }

        if (stream.match("[ ]", true)) {
            state.mdLineTask = false;
            return "task";
        }
        if (stream.match("[x]", true) || stream.match("[X]", true)) {
            state.mdLineTask = false;
            return "task task-checked";
        }

        stream.pos = start;
        state.mdLineTask = false;
    }

    function templateKind(stream) {
        const prefix = stream.string.slice(0, stream.pos);
        const match = prefix.match(/(?:^|[^\w$.])(?:Document\.)?(html|xml)\s*\(\s*$/);
        return match ? match[1] : null;
    }

    function templateMode(state) {
        return state.templateKind == "html" ? htmlMode : (state.templateKind == "xml" ? xmlMode : null);
    }

    function templateEnd(stream, state) {
        const quote = state.templateQuote;
        for(let i = stream.pos; i < stream.string.length; ++i) {
            if(stream.string[i] !== quote) continue;

            let escapes = 0;
            for(let j = i - 1; j >= 0 && stream.string[j] === "\\"; --j)
                ++escapes;
            if(escapes % 2 == 0) return i;
        }
        return -1;
    }

    function closeTemplate(state) {
        state.inTemplate = false;
        state.templateKind = null;
        state.templateQuote = null;
        state.templateState = null;
    }

    function templateToken(stream, state) {
        if(stream.peek() === state.templateQuote) {
            stream.next();
            closeTemplate(state);
            return "string";
        }

        const mode = templateMode(state);
        const end = templateEnd(stream, state);
        if(!mode) {
            if(end >= 0) stream.pos = end;
            else stream.skipToEnd();
            return "string";
        }

        if(end < 0) {
            const style = mode.token(stream, state.templateState);
            return style ? style + " notojs-embedded" : "notojs-embedded";
        }

        const line = stream.string;
        stream.string = line.slice(0, end);
        const style = mode.token(stream, state.templateState);
        stream.string = line;
        return style ? style + " notojs-embedded" : "notojs-embedded";
    }

    return {
        startState() {
            return {
                jsState: CodeMirror.startState(jsMode),
                mdState: CodeMirror.startState(mdMode),
                inCodeEcho: false,
                inMarkdown: false,
                inTemplate: false,
                templateKind: null,
                templateQuote: null,
                templateState: null,
                mdLineTask: false
            };
        },

        copyState(state) {
            return {
                jsState: CodeMirror.copyState(jsMode, state.jsState),
                mdState: CodeMirror.copyState(mdMode, state.mdState),
                inCodeEcho: state.inCodeEcho,
                inMarkdown: state.inMarkdown,
                inTemplate: state.inTemplate,
                templateKind: state.templateKind,
                templateQuote: state.templateQuote,
                templateState: state.templateKind ? CodeMirror.copyState(templateMode(state), state.templateState) : null
            };
        },

        token(stream, state) {
            if (!state.inTemplate && "`'\"".includes(stream.peek())) {
                state.templateKind = templateKind(stream);
                if(stream.peek() !== "`" && !state.templateKind) return jsMode.token(stream, state.jsState);

                state.templateQuote = stream.peek();
                stream.next();
                state.inTemplate = true;
                state.templateState = templateMode(state) ? CodeMirror.startState(templateMode(state)) : null;
                return "string";
            }

            if (state.inTemplate) {
                return templateToken(stream, state);
            }

            if (!state.inTemplate && !state.inMarkdown && stream.sol()) {
                const match = stream.match(mdMark[0], true);
                if (match && stream.eol()) {
                    state.inCodeEcho = !(state.inMarkdown = (match[0] != '<[!['));
                    return "bracket";
                } else if (match) {
                    stream.backUp(match[0].length);
                }
            }

            if (!state.inTemplate && (state.inCodeEcho || state.inMarkdown) && stream.sol() && stream.match(mdMark[1], false)) {
                const startPos = stream.pos;
                stream.skipToEnd();
                if (stream.string.slice(startPos).trim() === mdMark[1]) {
                    stream.match(mdMark[1], true);
                    state.inCodeEcho = false;
                    state.inMarkdown = false;
                    return "bracket";
                } else {
                    stream.pos = startPos;
                }
            }

            if (state.inMarkdown) {
                if (stream.sol()) tryTask(stream, state);
                return tryBox(stream, state) ||  mdMode.token(stream, state.mdState) || null;
            } else {
                return jsMode.token(stream, state.jsState);
            }
        },

        indent(state, textAfter) {
            if (state.inTemplate) {
                const mode = templateMode(state);
                return mode?.indent ? mode.indent(state.templateState, textAfter) : CodeMirror.Pass;
            }
            return jsMode.indent(state.jsState, textAfter);
        },

        innerMode(state) {
            if(state.inTemplate && templateMode(state)) return {mode: templateMode(state), state: state.templateState};
            return state.inMarkdown
                ? { mode: mdMode, state: state.mdState }
                : { mode: jsMode, state: state.jsState };
        },
    };
});

CodeMirror.notojs = CodeMirror.notojs || {
    complete: new class {
        init(cm) {
        }
    }
};

CodeMirror.notojs.edit = new class {
    extraKeys = {
        "'{'": this.#autoClose('{', '}'),
        "'('": this.#autoClose('(', ')'),
        "'['": cm => this.#modeSwitch(cm),
        "'\"'": this.#quote('"'),
        "'''": this.#quote("'"),
        "'`'": cm => this.#backtick(cm),
        "')'": this.#close(')'),
        "']'": cm => this.#closeSquare(cm),
        "'}'": this.#close('}'),
        "'>'": cm => this.#closeMacro(cm),
        "';'": cm => this.#semicolon(cm),
        "Cmd-;": cm => this.#ignore(cm),
        "Ctrl-;": cm => this.#ignore(cm),
        "Backspace": cm => this.#backspace(cm),
        "Enter": cm => this.#enter(cm),
        "Space": cm => this.#space(cm),
        "Tab": "indentMore",
        "Shift-Tab": "indentLess"
    };

    init(cm) {
        CodeMirror.notojs.complete.init(cm);
        cm.on('cursorActivity', cm => this.#macroMatch(cm));
        cm.on('inputRead', (cm, change) => this.#normalizeHeading(cm, change));
        cm.on('blur', cm => this.#clearMatches(cm));
        this.#macroMatch(cm);
    }

    #active(cm) {
        const state = cm.getTokenAt(cm.getCursor()).state;
        return state?.jsState && !state.inMarkdown && !state.inTemplate;
    }

    #ignore() {
    }

    #inMarkdownFence(cm, line) {
        let start = line - 1;
        const markdownStart = /^<\[(?:!?(?:[a-zA-Z_$][a-zA-Z0-9_$]*)?|:)\[$/;
        while(start >= 0 && !markdownStart.test(cm.getLine(start))) start--;

        let fence = null;
        for(let i = start + 1; i < line; i++) {
            const text = cm.getLine(i);
            if(fence) {
                const close = text.match(/^\s{0,3}(`{3,}|~{3,})\s*$/);
                if(close && close[1][0] === fence.ch && close[1].length >= fence.length) fence = null;
                continue;
            }

            const open = text.match(/^\s{0,3}(`{3,}|~{3,})(.*)$/);
            if(open && (open[1][0] !== '`' || !open[2].includes('`'))) {
                fence = {ch: open[1][0], length: open[1].length};
            }
        }

        return !!fence;
    }

    #normalizeHeading(cm, change) {
        if(change.origin !== '+input') return;
        if(!change.text || change.text.length != 1 || change.text[0].length != 1) return;

        const cursor = cm.getCursor();
        const state = cm.getTokenAt(cursor).state;
        if(!state?.inMarkdown) return;

        const line = cm.getLine(cursor.line);
        const heading = line.match(/^(\s{0,3}#{1,6})([^#\s].*)$/);
        if(!heading || this.#inMarkdownFence(cm, cursor.line)) return;

        cm.replaceRange(heading[1] + ' ' + heading[2], {line: cursor.line, ch: 0}, {line: cursor.line, ch: line.length}, '+input');
        cm.setCursor(cursor.line, cursor.ch + 1);
    }

    #macroMatch(cm) {
        this.#clearMatches(cm);

        const cursor = cm.getCursor();
        const line = cm.getLine(cursor.line);
        const open = /^<\[(?:!?(?:[A-Za-z_$][A-Za-z0-9_$]*)?|:)\[$/.test(line);
        const close = line === ']]>';
        if(/^<\[:?$/.test(line)) this.#clearBracketMatch(cm);

        if(open || close) {
            this.#clearBracketMatch(cm);

            let from = cursor.line;
            let to = cursor.line;
            if(open) {
                while(++to < cm.lineCount() && cm.getLine(to) !== ']]>');
                if(to == cm.lineCount()) return;
            } else {
                while(--from >= 0 && !/^<\[(?:!?(?:[A-Za-z_$][A-Za-z0-9_$]*)?|:)\[$/.test(cm.getLine(from)));
                if(from < 0) return;
            }

            this.#markLines(cm, from, to, 'cm-notojs-matchingmacro');
            return;
        }

        if(/^\s*```/.test(line)) {
            const opening = this.#fenceCount(cm, cursor.line) % 2 == 0;
            let from = cursor.line;
            let to = cursor.line;
            if(opening) {
                while(++to < cm.lineCount() && !/^\s*```/.test(cm.getLine(to)));
                if(to == cm.lineCount()) return;
            } else {
                while(--from >= 0 && !/^\s*```/.test(cm.getLine(from)));
                if(from < 0) return;
            }

            this.#markLines(cm, from, to, 'cm-notojs-matchingfence');
        }
    }

    #clearMatches(cm) {
        cm.notojs = cm.notojs || {};
        cm.notojs.edit = cm.notojs.edit || {};
        for(const mark of cm.notojs.edit.macroMarks || []) mark.clear();
        cm.notojs.edit.macroMarks = [];
    }

    #markLines(cm, from, to, className) {
        cm.notojs = cm.notojs || {};
        cm.notojs.edit = cm.notojs.edit || {};
        cm.notojs.edit.macroMarks = [
            cm.markText({line: from, ch: 0}, {line: from, ch: cm.getLine(from).length}, {className}),
            cm.markText({line: to, ch: 0}, {line: to, ch: cm.getLine(to).length}, {className})
        ];
    }

    #clearBracketMatch(cm) {
        setTimeout(() => {
            for(const el of cm.getWrapperElement().querySelectorAll('.CodeMirror-matchingbracket, .CodeMirror-nonmatchingbracket')) {
                el.classList.remove('CodeMirror-matchingbracket', 'CodeMirror-nonmatchingbracket');
            }
        }, 0);
    }

    #autoClose(o, c) {
        return cm => {
            if(!this.#active(cm)) return CodeMirror.Pass;

            cm.replaceSelection(o + c, "around");
            const cursor = cm.getCursor();
            cm.setCursor(cursor.line, cursor.ch - 1);
        };
    }

    #enter(cm) {
        if(cm.somethingSelected()) return CodeMirror.Pass;

        const cursor = cm.getCursor();
        const state = cm.getTokenAt(cursor).state;
        const line = cm.getLine(cursor.line);
        if(state?.inMarkdown) return this.#markdownEnter(cm, cursor);
        if(state?.inTemplate && line[cursor.ch - 1] === '`' && /^\)*\s*$/.test(line.slice(cursor.ch))) {
            const base = line.match(/^\s*/)[0];
            const next = this.#nextText(cm, cursor.line);
            if(this.#hasTemplateClose(cm, cursor.line)) {
                cm.replaceRange('\n' + base, cursor, cursor, '+input');
            } else {
                const close = base + '`' + (line.slice(cursor.ch).trim().startsWith(')') || next?.startsWith(')') ? '' : ')');
                cm.replaceRange('\n' + base + '\n' + close, cursor, cursor, '+input');
            }
            cm.setCursor(cursor.line + 1, base.length);
            return;
        }
        if(!this.#active(cm)) return CodeMirror.Pass;

        if(cursor.ch == line.length && /^<\[(?:!?(?:[A-Za-z_$][A-Za-z0-9_$]*)?|:)\[$/.test(line)) {
            cm.replaceSelection('\n\n]]>', 'around');
            cm.setCursor(cursor.line + 1, 0);
            return;
        }

        if(line[cursor.ch - 1] !== '{' || line[cursor.ch] !== '}') return CodeMirror.Pass;

        const base = line.match(/^\s*/)[0];
        const inner = base + ' '.repeat(cm.getOption('indentUnit'));
        cm.replaceSelection('\n' + inner + '\n' + base, 'around');
        cm.setCursor(cursor.line + 1, inner.length);
    }

    #nextText(cm, line) {
        for(let i = line + 1; i < cm.lineCount(); ++i) {
            const text = cm.getLine(i).trim();
            if(text) return text;
        }
        return null;
    }

    #hasTemplateClose(cm, line) {
        for(let i = line + 1; i < cm.lineCount(); ++i) {
            const text = cm.getLine(i).trim();
            if(text.startsWith('`')) return true;
            if(text.endsWith('`') || text.endsWith('`)')) return true;
        }
        return false;
    }

    #markdownEnter(cm, cursor) {
        let line = cm.getLine(cursor.line);
        const heading = line.match(/^(\s{0,3}#{1,6})([^#\s].*)$/);
        if(heading && !this.#inMarkdownFence(cm, cursor.line)) {
            line = heading[1] + ' ' + heading[2];
            cm.replaceRange(line, {line: cursor.line, ch: 0}, {line: cursor.line, ch: cm.getLine(cursor.line).length}, '+input');
            cursor = {line: cursor.line, ch: line.length};
            cm.setCursor(cursor);
        }
        if(line.slice(cursor.ch).trim()) return CodeMirror.Pass;

        const fence = line.match(/^(\s*)(`{3,})[^`]*$/);
        const nextFence = this.#nextText(cm, cursor.line)?.match(/^(`{3,})\s*$/);
        if(fence && this.#fenceCount(cm, cursor.line) % 2 == 0 &&
            (!nextFence || nextFence[1].length < fence[2].length)) {
            cm.replaceSelection('\n' + fence[1] + '\n' + fence[1] + fence[2], 'around');
            cm.setCursor(cursor.line + 1, fence[1].length);
            return;
        }

        const emptyTask = line.match(/^(\s*)(?:[-*+]|\d+[.)])\s+\[[ xX]\]\s*$/);
        if(emptyTask) {
            cm.replaceRange('', {line: cursor.line, ch: emptyTask[1].length}, {line: cursor.line, ch: line.length}, '+input');
            return;
        }

        const empty = line.match(/^(\s*)(?:[-*+]|\d+[.)])\s*$/);
        if(empty) {
            cm.replaceRange('', {line: cursor.line, ch: empty[1].length}, {line: cursor.line, ch: line.length}, '+input');
            return;
        }

        let next;
        const unorderedTask = line.match(/^(\s*)([-*+])\s+\[[ xX]\]\s+\S/);
        const orderedTask = line.match(/^(\s*)(\d+)([.)])\s+\[[ xX]\]\s+\S/);
        const unordered = line.match(/^(\s*)([-*+])\s+\S/);
        const ordered = line.match(/^(\s*)(\d+)([.)])\s+\S/);
        if(unorderedTask) {
            next = unorderedTask[1] + unorderedTask[2] + ' [ ] ';
        } else if(orderedTask) {
            next = orderedTask[1] + (parseInt(orderedTask[2]) + 1) + orderedTask[3] + ' [ ] ';
        } else if(unordered) {
            next = unordered[1] + unordered[2] + ' ';
        } else if(ordered) {
            next = ordered[1] + (parseInt(ordered[2]) + 1) + ordered[3] + ' ';
        } else {
            return CodeMirror.Pass;
        }

        cm.replaceSelection('\n' + next, 'around');
        cm.setCursor(cursor.line + 1, next.length);
    }

    #fenceCount(cm, line) {
        let count = 0;
        for(let i = 0; i < line; ++i)
            if(/^\s*```/.test(cm.getLine(i))) ++count;
        return count;
    }

    #close(c) {
        return cm => {
            if(cm.somethingSelected()) return CodeMirror.Pass;
            if(!this.#active(cm)) return CodeMirror.Pass;

            const cursor = cm.getCursor();
            if(cm.getLine(cursor.line)[cursor.ch] !== c) return CodeMirror.Pass;
            cm.setCursor(cursor.line, cursor.ch + 1);
        };
    }

    #closeSquare(cm) {
        if(cm.somethingSelected()) return CodeMirror.Pass;

        const cursor = cm.getCursor();
        const line = cm.getLine(cursor.line);
        if(line.slice(cursor.ch).startsWith(']]>')) {
            cm.setCursor(cursor.line, cursor.ch + 3);
            return;
        }

        return this.#close(']')(cm);
    }

    #closeMacro(cm) {
        if(cm.somethingSelected()) return CodeMirror.Pass;

        const cursor = cm.getCursor();
        const line = cm.getLine(cursor.line);
        if(line[cursor.ch] === '>' && line.slice(cursor.ch - 2, cursor.ch) === ']]') {
            cm.setCursor(cursor.line, cursor.ch + 1);
            return;
        }

        return CodeMirror.Pass;
    }

    #quote(q) {
        return cm => {
            if(cm.somethingSelected()) return CodeMirror.Pass;
            if(!this.#active(cm)) return CodeMirror.Pass;

            const cursor = cm.getCursor();
            const line = cm.getLine(cursor.line);
            if(line[cursor.ch] === q) {
                cm.setCursor(cursor.line, cursor.ch + 1);
                return;
            }
            if(/[$\w]/.test(line[cursor.ch] || '')) return CodeMirror.Pass;

            cm.replaceSelection(q + q, 'around');
            cm.setCursor(cursor.line, cursor.ch + 1);
        };
    }

    #backtick(cm) {
        if(cm.somethingSelected()) return CodeMirror.Pass;

        const cursor = cm.getCursor();
        const state = cm.getTokenAt(cursor).state;
        if(!state?.inTemplate) return CodeMirror.Pass;
        if(cm.getLine(cursor.line)[cursor.ch] !== '`') return CodeMirror.Pass;

        cm.setCursor(cursor.line, cursor.ch + 1);
    }

    #modeSwitch(cm) {
        if(cm.somethingSelected()) return CodeMirror.Pass;

        const cursor = cm.getCursor();
        const state = cm.getTokenAt(cursor).state;
        const line = cm.getLine(cursor.line);
        if(state?.inMarkdown && /^(?:[-*+]|\d+[.)])\s+$/.test(line.slice(0, cursor.ch)) && !line.slice(cursor.ch).trim()) {
            cm.replaceRange('[ ] ', cursor, cursor, '+input');
            cm.setCursor(cursor.line, cursor.ch + 4);
            return;
        }
        if(!this.#active(cm)) return CodeMirror.Pass;

        const text = line.slice(0, cursor.ch) + '[';
        if(text === '<[') return CodeMirror.Pass;
        if(!/^<\[(?:!?(?:[A-Za-z_$][A-Za-z0-9_$]*)?|:)\[$/.test(text)) return this.#autoClose('[', ']')(cm);
        if(line.slice(cursor.ch).trim()) return CodeMirror.Pass;

        cm.replaceRange('[\n\n]]>', cursor, cursor, '+input');
        cm.setCursor(cursor.line + 1, 0);
    }

    #backspace(cm) {
        if(cm.somethingSelected()) return CodeMirror.Pass;
        if(!this.#active(cm)) return CodeMirror.Pass;

        const cursor = cm.getCursor();
        const line = cm.getLine(cursor.line);
        if(cursor.ch == 0) return CodeMirror.Pass;

        const pairs = {'(': ')', '[': ']', '{': '}', '"': '"', "'": "'"};
        if(pairs[line[cursor.ch - 1]] !== line[cursor.ch]) return CodeMirror.Pass;

        cm.replaceRange('', {line: cursor.line, ch: cursor.ch - 1}, {line: cursor.line, ch: cursor.ch + 1}, '+delete');
    }

    #space(cm) {
        if(cm.somethingSelected()) return CodeMirror.Pass;

        const cursor = cm.getCursor();
        const state = cm.getTokenAt(cursor).state;
        if(!state?.inMarkdown) return CodeMirror.Pass;

        const line = cm.getLine(cursor.line);
        const before = line.slice(0, cursor.ch);
        const after = line.slice(cursor.ch);
        const heading = before.match(/^(\s{0,3}#{1,6})\s*$/);
        if(!heading) return CodeMirror.Pass;

        const text = after.replace(/^\s*/, '');
        cm.replaceRange(heading[1] + ' ' + text, {line: cursor.line, ch: 0}, {line: cursor.line, ch: line.length}, '+input');
        cm.setCursor(cursor.line, heading[1].length + 1);
    }

    #forHeader(cm, cursor) {
        const stack = [];
        for(let l = 0; l <= cursor.line; ++l) {
            const line = cm.getLine(l);
            const end = l == cursor.line ? cursor.ch : line.length;
            for(let ch = 0; ch < end; ++ch) {
                if(line[ch] === '(') {
                    stack.push(/\bfor$/.test(line.slice(0, ch).trimEnd()));
                } else if(line[ch] === ')' && stack.length) {
                    stack.pop();
                }
            }
        }
        return stack.includes(true);
    }

    #semicolon(cm) {
        if(cm.somethingSelected()) return CodeMirror.Pass;
        if(!this.#active(cm)) return CodeMirror.Pass;

        const cursor = cm.getCursor();
        const line = cm.getLine(cursor.line);
        if(this.#forHeader(cm, cursor)) return CodeMirror.Pass;

        let ch = cursor.ch;

        while(")]}".includes(line[ch])) ++ch;
        if(line[ch] === ';') {
            cm.setCursor(cursor.line, ch + 1);
            return;
        }
        if(ch == cursor.ch) return CodeMirror.Pass;

        cm.replaceRange(';', {line: cursor.line, ch}, null, '+input');
        cm.setCursor(cursor.line, ch + 1);
    }
};

CodeMirror.notojs.app = class Handler {
    static init(cm) {
        cm.setValue(Handler.#template(cm.getValue()));
        cm.setOption('firstLineNumber', -3);
        Handler.#readonlyLineClasses(cm);

        const head = cm.markText(
            {line: 0, ch: 0},
            {line: 3, ch: Infinity},
            {readOnly: true, atomic: true, inclusiveLeft: true, inclusiveRight: true}
        );
        const tail = cm.markText(
            {line: cm.lineCount() - 3, ch: 0},
            {line: cm.lineCount() - 1, ch: Infinity},
            {readOnly: true, atomic: true, inclusiveLeft: true, inclusiveRight: true}
        );

        cm.notojs = cm.notojs || {};
        cm.notojs.app = {head, tail};
    }

    static code(cm, c) {
        const marks = cm.notojs?.app;
        const head = marks?.head.find();
        const tail = marks?.tail.find();
        if(!head || !tail) return c === undefined ? cm.getValue() : cm.setValue(c);

        if(c === undefined) {
            const s = cm.indexFromPos(head.to);
            const e = cm.indexFromPos(tail.from);
            return cm.getValue().slice(s + 1, e - 1);
        }

        const f = cm.findPosH(head.to, +1, "char");
        const t = cm.findPosH(tail.from, -1, "char");
        cm.operation(() => cm.replaceRange(c, f, t, "setValue"));
    }

    static #readonlyLineClasses(cm) {
        [0, 1, 2, 3, cm.lineCount() - 3, cm.lineCount() - 2, cm.lineCount() - 1].forEach(l => {
            cm.addLineClass(l, "line", "cm-readonly-line");
            cm.addLineClass(l, "gutter", "cm-readonly-line");
        });
    }

    static #template(value) {
        return `$.fetch = (async function(request) {
  const response = new ServerResponse();
  response.status = 405;

${value}

  return response;
});`
    }
};

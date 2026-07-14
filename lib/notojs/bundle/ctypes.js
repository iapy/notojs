CodeMirror.notojs = {
    app: (class Handler{
        static init(cm) {
            cm.setValue(Handler.#template(cm.getValue()));
            cm.setOption('firstLineNumber', -3);
            [0, 1, 2, 3, cm.lineCount() - 3, cm.lineCount() - 2, cm.lineCount() - 1].map((l) => {
                cm.addLineClass(l, "line", "cm-readonly-line");
                cm.addLineClass(l, "gutter", "cm-readonly-line");
            });
            cm.markText(
                {line: 0, ch: 0},
                {line: 3, ch: Infinity},
                {readOnly: true, atomic: true, inclusiveLeft: true, inclusiveRight: true}
            );
            cm.markText(
                {line: cm.lineCount() - 3, ch: 0},
                {line: cm.lineCount() - 1, ch: Infinity},
                {readOnly: true, atomic: true, inclusiveLeft: true, inclusiveRight: true}
            );
        }
        static code(cm, c) {
            const [head, tail] = cm.getAllMarks().filter(m => {
                const opts = m.className || m.readOnly || (m._options && m._options.readOnly);
                return opts || (m.replacedWith && m.replacedWith.classList.contains("cm-readonly"));
            }).map(m => m.find()).filter(Boolean);
            if(c === undefined) {
                const s = cm.indexFromPos(head.to);
                const e = cm.indexFromPos(tail.from);
                return cm.getValue().substr(s + 1, e - s - 2);
            } else {
                const f = cm.findPosH(head.to, +1, "char");
                const t = cm.findPosH(tail.from, -1, "char");
                cm.operation(() => cm.replaceRange(c, f, t, "setValue"));
            }
        }
        static #template(value) {
            return `$.fetch = (async function(request) {
  const response = new ServerResponse();
  response.status = 405;

${value}

  return response;
});`
        }
    })
};

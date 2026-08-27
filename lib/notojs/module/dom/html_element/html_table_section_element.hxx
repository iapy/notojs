struct HTMLTableSectionElement : bridge::Interface<HTMLTableSectionElement, dom::HTMLElement, HTMLElement>
{
    HTMLTableSectionElement(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    HTMLTableSectionElement(std::reference_wrapper<dom::HTMLElement> &&rw) : Base(std::move(rw)) {}

    JSValue rows(JSContext *ctx) const
    {
        return HTMLCollection::from(ctx, dom::HTMLCollection{dom::Node{ref().doc, ref().node}, "tr"});
    }

    std::vector<lxb_dom_node_t *> sectionRows() const
    {
        std::vector<lxb_dom_node_t *> rows;
        for(lxb_dom_node_t *node = static_cast<lxb_dom_node_t *>(ref())->first_child; node; node = node->next)
            if(LXB_TAG_TR == lxb_dom_node_tag_id(node))
                rows.push_back(node);
        return rows;
    }

    JSValue insertRow_(JSContext *ctx, std::int64_t index)
    {
        auto rows = sectionRows();
        if(index < -1 || index > static_cast<std::int64_t>(rows.size()))
            return JS_ThrowRangeError(ctx, "Index out of range");

        auto doc = dynamic_cast<dom::HTMLBackend *>(ref().doc.get());
        auto *row = doc->createElement("tr", 2);
        if(index >= 0 && index < static_cast<std::int64_t>(rows.size()))
            lxb_dom_node_insert_before_spec(static_cast<lxb_dom_node_t *>(ref()), row, rows[static_cast<std::size_t>(index)]);
        else
            lxb_dom_node_append_child(static_cast<lxb_dom_node_t *>(ref()), row);
        return doc->make(row);
    }

    JSValue insertRow_0(JSContext *ctx)
    {
        return insertRow_(ctx, -1);
    }

    JSValue insertRow_1(JSContext *ctx, bridge::Number index)
    {
        return insertRow_(ctx, static_cast<std::int64_t>(index));
    }

    using insertRow = bridge::Function
    <
        &HTMLTableSectionElement::insertRow_0,
        &HTMLTableSectionElement::insertRow_1
    >;

    JSValue deleteRow(JSContext *ctx, bridge::Number index)
    {
        auto rows = sectionRows();
        auto i = static_cast<std::int64_t>(index);
        if(-1 == i)
            i = static_cast<std::int64_t>(rows.size()) - 1;

        if(rows.empty() || i < 0 || i >= static_cast<std::int64_t>(rows.size()))
            return JS_ThrowRangeError(ctx, "Index out of range");

        lxb_dom_node_remove(rows[static_cast<std::size_t>(i)]);
        return JS_UNDEFINED;
    }

    using Base::Base;
    using ctor = bridge::Unconstructable<HTMLTableSectionElement>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const HTMLTableSectionElement::funcs[] = {
    JS_CGETSET_DEF("rows", &bridge::Getter<&HTMLTableSectionElement::rows>, NULL),
    JS_CFUNC_DEF("insertRow", 1, &HTMLTableSectionElement::insertRow::invoke),
    JS_CFUNC_DEF("deleteRow", 1, &bridge::Function<&HTMLTableSectionElement::deleteRow>::invoke)
};

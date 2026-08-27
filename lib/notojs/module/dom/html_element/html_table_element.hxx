struct HTMLTableElement : bridge::Interface<HTMLTableElement, dom::HTMLElement, HTMLElement>
{
    HTMLTableElement(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    HTMLTableElement(std::reference_wrapper<dom::HTMLElement> &&rw) : Base(std::move(rw)) {}

    static bool isSection(lxb_dom_node_t *node)
    {
        return LXB_TAG_THEAD == lxb_dom_node_tag_id(node)
            || LXB_TAG_TBODY == lxb_dom_node_tag_id(node)
            || LXB_TAG_TFOOT == lxb_dom_node_tag_id(node);
    }

    std::vector<lxb_dom_node_t *> tableRows() const
    {
        std::vector<lxb_dom_node_t *> rows;
        for(lxb_dom_node_t *node = static_cast<lxb_dom_node_t *>(ref())->first_child; node; node = node->next)
        {
            if(LXB_TAG_TR == lxb_dom_node_tag_id(node))
            {
                rows.push_back(node);
            }
            else if(isSection(node))
            {
                for(lxb_dom_node_t *row = node->first_child; row; row = row->next)
                    if(LXB_TAG_TR == lxb_dom_node_tag_id(row))
                        rows.push_back(row);
            }
        }
        return rows;
    }

    lxb_dom_node_t *lastTBody() const
    {
        lxb_dom_node_t *tbody{nullptr};
        for(lxb_dom_node_t *node = static_cast<lxb_dom_node_t *>(ref())->first_child; node; node = node->next)
            if(LXB_TAG_TBODY == lxb_dom_node_tag_id(node))
                tbody = node;
        return tbody;
    }

    JSValue caption(JSContext *ctx) const
    {
        if(auto *node = ref().directChild(LXB_TAG_CAPTION))
            return dynamic_cast<dom::HTMLBackend *>(ref().doc.get())->make(node);
        return JS_NULL;
    }

    JSValue tHead(JSContext *ctx) const
    {
        if(auto *node = ref().directChild(LXB_TAG_THEAD))
            return dynamic_cast<dom::HTMLBackend *>(ref().doc.get())->make(node);
        return JS_NULL;
    }

    JSValue tFoot(JSContext *ctx) const
    {
        if(auto *node = ref().directChild(LXB_TAG_TFOOT))
            return dynamic_cast<dom::HTMLBackend *>(ref().doc.get())->make(node);
        return JS_NULL;
    }

    JSValue tBodies(JSContext *ctx) const
    {
        return HTMLCollection::from(ctx, dom::HTMLCollection{dom::Node{ref().doc, ref().node}, "tbody"});
    }

    JSValue rows(JSContext *ctx) const
    {
        return HTMLCollection::from(ctx, dom::HTMLCollection{dom::Node{ref().doc, ref().node}, "tr"});
    }

    JSValue createCaption(JSContext *ctx)
    {
        auto doc = dynamic_cast<dom::HTMLBackend *>(ref().doc.get());
        if(auto *node = ref().directChild(LXB_TAG_CAPTION))
            return doc->make(node);

        auto *table = static_cast<lxb_dom_node_t *>(ref());
        auto *caption = doc->createElement("caption", 7);
        if(table->first_child)
            lxb_dom_node_insert_before_spec(table, caption, table->first_child);
        else
            lxb_dom_node_append_child(table, caption);
        return doc->make(caption);
    }

    JSValue deleteCaption(JSContext *)
    {
        if(auto *node = ref().directChild(LXB_TAG_CAPTION))
            lxb_dom_node_remove(node);
        return JS_UNDEFINED;
    }

    JSValue createTHead(JSContext *ctx)
    {
        auto doc = dynamic_cast<dom::HTMLBackend *>(ref().doc.get());
        if(auto *node = ref().directChild(LXB_TAG_THEAD))
            return doc->make(node);

        auto *table = static_cast<lxb_dom_node_t *>(ref());
        auto *thead = doc->createElement("thead", 5);
        for(lxb_dom_node_t *node = table->first_child; node; node = node->next)
        {
            auto const tag = lxb_dom_node_tag_id(node);
            if(LXB_TAG_CAPTION != tag && LXB_TAG_COLGROUP != tag)
            {
                lxb_dom_node_insert_before_spec(table, thead, node);
                return doc->make(thead);
            }
        }

        lxb_dom_node_append_child(table, thead);
        return doc->make(thead);
    }

    JSValue deleteTHead(JSContext *)
    {
        if(auto *node = ref().directChild(LXB_TAG_THEAD))
            lxb_dom_node_remove(node);
        return JS_UNDEFINED;
    }

    JSValue createTFoot(JSContext *ctx)
    {
        auto doc = dynamic_cast<dom::HTMLBackend *>(ref().doc.get());
        if(auto *node = ref().directChild(LXB_TAG_TFOOT))
            return doc->make(node);

        auto *tfoot = doc->createElement("tfoot", 5);
        lxb_dom_node_append_child(static_cast<lxb_dom_node_t *>(ref()), tfoot);
        return doc->make(tfoot);
    }

    JSValue deleteTFoot(JSContext *)
    {
        if(auto *node = ref().directChild(LXB_TAG_TFOOT))
            lxb_dom_node_remove(node);
        return JS_UNDEFINED;
    }

    JSValue createTBody(JSContext *ctx)
    {
        auto doc = dynamic_cast<dom::HTMLBackend *>(ref().doc.get());
        auto *table = static_cast<lxb_dom_node_t *>(ref());
        auto *tbody = doc->createElement("tbody", 5);
        if(auto *tfoot = ref().directChild(LXB_TAG_TFOOT))
            lxb_dom_node_insert_before_spec(table, tbody, tfoot);
        else
            lxb_dom_node_append_child(table, tbody);
        return doc->make(tbody);
    }

    JSValue insertRow_(JSContext *ctx, std::int64_t index)
    {
        auto rows = tableRows();
        if(index < -1 || index > static_cast<std::int64_t>(rows.size()))
            return JS_ThrowRangeError(ctx, "Index out of range");

        auto doc = dynamic_cast<dom::HTMLBackend *>(ref().doc.get());
        auto *row = doc->createElement("tr", 2);
        if(index >= 0 && index < static_cast<std::int64_t>(rows.size()))
        {
            auto *target = rows[static_cast<std::size_t>(index)];
            lxb_dom_node_insert_before_spec(target->parent, row, target);
        }
        else
        {
            auto *tbody = lastTBody();
            if(!tbody)
            {
                tbody = doc->createElement("tbody", 5);
                if(auto *tfoot = ref().directChild(LXB_TAG_TFOOT))
                    lxb_dom_node_insert_before_spec(static_cast<lxb_dom_node_t *>(ref()), tbody, tfoot);
                else
                    lxb_dom_node_append_child(static_cast<lxb_dom_node_t *>(ref()), tbody);
            }
            lxb_dom_node_append_child(tbody, row);
        }
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
        &HTMLTableElement::insertRow_0,
        &HTMLTableElement::insertRow_1
    >;

    JSValue deleteRow(JSContext *ctx, bridge::Number index)
    {
        auto rows = tableRows();
        auto i = static_cast<std::int64_t>(index);
        if(-1 == i)
            i = static_cast<std::int64_t>(rows.size()) - 1;

        if(rows.empty() || i < 0 || i >= static_cast<std::int64_t>(rows.size()))
            return JS_ThrowRangeError(ctx, "Index out of range");

        lxb_dom_node_remove(rows[static_cast<std::size_t>(i)]);
        return JS_UNDEFINED;
    }

    using Base::Base;
    using ctor = bridge::Unconstructable<HTMLTableElement>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const HTMLTableElement::funcs[] = {
    JS_CGETSET_DEF("caption", &bridge::Getter<&HTMLTableElement::caption>, NULL),
    JS_CGETSET_DEF("tHead", &bridge::Getter<&HTMLTableElement::tHead>, NULL),
    JS_CGETSET_DEF("tFoot", &bridge::Getter<&HTMLTableElement::tFoot>, NULL),
    JS_CGETSET_DEF("tBodies", &bridge::Getter<&HTMLTableElement::tBodies>, NULL),
    JS_CGETSET_DEF("rows", &bridge::Getter<&HTMLTableElement::rows>, NULL),
    JS_CFUNC_DEF("createCaption", 0, &bridge::Function<&HTMLTableElement::createCaption>::invoke),
    JS_CFUNC_DEF("deleteCaption", 0, &bridge::Function<&HTMLTableElement::deleteCaption>::invoke),
    JS_CFUNC_DEF("createTHead", 0, &bridge::Function<&HTMLTableElement::createTHead>::invoke),
    JS_CFUNC_DEF("deleteTHead", 0, &bridge::Function<&HTMLTableElement::deleteTHead>::invoke),
    JS_CFUNC_DEF("createTFoot", 0, &bridge::Function<&HTMLTableElement::createTFoot>::invoke),
    JS_CFUNC_DEF("deleteTFoot", 0, &bridge::Function<&HTMLTableElement::deleteTFoot>::invoke),
    JS_CFUNC_DEF("createTBody", 0, &bridge::Function<&HTMLTableElement::createTBody>::invoke),
    JS_CFUNC_DEF("insertRow", 1, &HTMLTableElement::insertRow::invoke),
    JS_CFUNC_DEF("deleteRow", 1, &bridge::Function<&HTMLTableElement::deleteRow>::invoke)
};

struct HTMLTableRowElement : bridge::Interface<HTMLTableRowElement, dom::HTMLElement, HTMLElement>
{
    HTMLTableRowElement(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    HTMLTableRowElement(std::reference_wrapper<dom::HTMLElement> &&rw) : Base(std::move(rw)) {}

    JSValue cells(JSContext *ctx) const
    {
        return HTMLCollection::from(ctx, dom::HTMLCollection{dom::Node{ref().doc, ref().node}, "td, th"});
    }

    std::vector<lxb_dom_node_t *> tableCells() const
    {
        std::vector<lxb_dom_node_t *> cells;
        for(lxb_dom_node_t *node = static_cast<lxb_dom_node_t *>(ref())->first_child; node; node = node->next)
            if(LXB_TAG_TD == lxb_dom_node_tag_id(node) || LXB_TAG_TH == lxb_dom_node_tag_id(node))
                cells.push_back(node);
        return cells;
    }

    JSValue insertCell_(JSContext *ctx, std::int64_t index)
    {
        auto cells = tableCells();
        if(index < -1 || index > static_cast<std::int64_t>(cells.size()))
            return JS_ThrowRangeError(ctx, "Index out of range");

        auto doc = dynamic_cast<dom::HTMLBackend *>(ref().doc.get());
        auto *cell = doc->createElement("td", 2);
        if(index >= 0 && index < static_cast<std::int64_t>(cells.size()))
            lxb_dom_node_insert_before_spec(static_cast<lxb_dom_node_t *>(ref()), cell, cells[static_cast<std::size_t>(index)]);
        else
            lxb_dom_node_append_child(static_cast<lxb_dom_node_t *>(ref()), cell);
        return doc->make(cell);
    }

    JSValue insertCell_0(JSContext *ctx)
    {
        return insertCell_(ctx, -1);
    }

    JSValue insertCell_1(JSContext *ctx, bridge::Number index)
    {
        return insertCell_(ctx, static_cast<std::int64_t>(index));
    }

    using insertCell = bridge::Function
    <
        &HTMLTableRowElement::insertCell_0,
        &HTMLTableRowElement::insertCell_1
    >;

    JSValue deleteCell(JSContext *ctx, bridge::Number index)
    {
        auto cells = tableCells();
        auto i = static_cast<std::int64_t>(index);
        if(-1 == i)
            i = static_cast<std::int64_t>(cells.size()) - 1;

        if(cells.empty() || i < 0 || i >= static_cast<std::int64_t>(cells.size()))
            return JS_ThrowRangeError(ctx, "Index out of range");

        lxb_dom_node_remove(cells[static_cast<std::size_t>(i)]);
        return JS_UNDEFINED;
    }

    JSValue rowIndex(JSContext *ctx) const
    {
        std::int64_t index{-1};
        lxb_dom_node_t *nn = static_cast<lxb_dom_node_t *>(ref());
        if(lxb_dom_node_t *table = ref().closest(LXB_TAG_TABLE))
        {
            for(lxb_dom_node_t* n = table->first_child; n; n = n->next)
            {
                if(LXB_TAG_TR == lxb_dom_node_tag_id(n))
                {
                    ++index;
                    if(n == nn) goto RETURN;
                }
                else if(LXB_TAG_TBODY == lxb_dom_node_tag_id(n)
                    || LXB_TAG_THEAD == lxb_dom_node_tag_id(n)
                    || LXB_TAG_TFOOT == lxb_dom_node_tag_id(n))
                {
                    for(lxb_dom_node_t* p = n->first_child; p; p = p->next)
                    {
                        if(LXB_TAG_TR == lxb_dom_node_tag_id(p))
                        {
                            ++index;
                            if(p == nn) goto RETURN;
                        }
                    }
                }
            }
        }
        RETURN:
            return bridge::Number{ctx, index};
    }

    JSValue sectionRowIndex(JSContext *ctx) const
    {
        std::int64_t index{-1};
        if(lxb_dom_node_t *nn = static_cast<lxb_dom_node_t *>(ref()); nn->parent && (
            LXB_TAG_TABLE == lxb_dom_node_tag_id(nn->parent)
            || LXB_TAG_TBODY == lxb_dom_node_tag_id(nn->parent)
            || LXB_TAG_THEAD == lxb_dom_node_tag_id(nn->parent)
            || LXB_TAG_TFOOT == lxb_dom_node_tag_id(nn->parent)
        ))
        {
            for(lxb_dom_node_t* n = nn->parent->first_child; n; n = n->next)
            {
                if(LXB_TAG_TR == lxb_dom_node_tag_id(n))
                {
                    ++index;
                    if(n == nn) break;
                }
            }
        }
        return bridge::Number{ctx, index};
    }

    using Base::Base;
    using ctor = bridge::Unconstructable<HTMLTableRowElement>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const HTMLTableRowElement::funcs[] = {
    JS_CGETSET_DEF("cells", &bridge::Getter<&HTMLTableRowElement::cells>, NULL),
    JS_CGETSET_DEF("rowIndex", &bridge::Getter<&HTMLTableRowElement::rowIndex>, NULL),
    JS_CGETSET_DEF("sectionRowIndex", &bridge::Getter<&HTMLTableRowElement::sectionRowIndex>, NULL),
    JS_CFUNC_DEF("insertCell", 1, &HTMLTableRowElement::insertCell::invoke),
    JS_CFUNC_DEF("deleteCell", 1, &bridge::Function<&HTMLTableRowElement::deleteCell>::invoke)
};

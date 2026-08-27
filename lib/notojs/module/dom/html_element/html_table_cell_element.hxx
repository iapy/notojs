struct HTMLTableCellElement : bridge::Interface<HTMLTableCellElement, dom::HTMLElement, HTMLElement>
{
    HTMLTableCellElement(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    HTMLTableCellElement(std::reference_wrapper<dom::HTMLElement> &&rw) : Base(std::move(rw)) {}

    JSValue cellIndex(JSContext *ctx) const
    {
        std::int64_t index{-1};
        if(lxb_dom_node_t *nn = static_cast<lxb_dom_node_t *>(ref()); nn->parent && (
            LXB_TAG_TR == lxb_dom_node_tag_id(nn->parent)
        ))
        {
            for(lxb_dom_node_t* n = nn->parent->first_child; n; n = n->next)
            {
                index += (LXB_TAG_TD == lxb_dom_node_tag_id(n) || LXB_TAG_TH == lxb_dom_node_tag_id(n));
                if(n == nn) break;
            }
        }
        return bridge::Number{ctx, index};
    }

    JSValue colSpan(JSContext *ctx) const
    {
        if(auto value = ref().getAttribute({"colspan"}))
        {
            std::string str{*value};
            auto span = u64(str.c_str());
            if(span > 0) return bridge::Number{ctx, span};
        }
        return bridge::Number{ctx, std::int64_t{1}};
    }

    void set_colSpan(JSContext *, bridge::Value value)
    {
        ref().setAttribute({"colspan"}, value.toString());
    }

    JSValue rowSpan(JSContext *ctx) const
    {
        if(auto value = ref().getAttribute({"rowspan"}))
        {
            std::string str{*value};
            auto span = u64(str.c_str());
            if(span >= 0) return bridge::Number{ctx, span};
        }
        return bridge::Number{ctx, std::int64_t{1}};
    }

    void set_rowSpan(JSContext *, bridge::Value value)
    {
        ref().setAttribute({"rowspan"}, value.toString());
    }

    using Base::Base;
    using ctor = bridge::Unconstructable<HTMLTableCellElement>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const HTMLTableCellElement::funcs[] = {
    REFLECTING_ATTRIBUTE(abbr),
    REFLECTING_ATTRIBUTE(headers),
    REFLECTING_ATTRIBUTE(scope),

    JS_CGETSET_DEF("cellIndex", &bridge::Getter<&HTMLTableCellElement::cellIndex>, NULL),
    JS_CGETSET_DEF("colSpan", &bridge::Getter<&HTMLTableCellElement::colSpan>, &bridge::Setter<&HTMLTableCellElement::set_colSpan>),
    JS_CGETSET_DEF("rowSpan", &bridge::Getter<&HTMLTableCellElement::rowSpan>, &bridge::Setter<&HTMLTableCellElement::set_rowSpan>),
};

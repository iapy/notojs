JSValue CSSRuleList::get_property(JSContext *ctx, char const *n) const
{
    if(auto index = u64(n); index >= 0)
    {
        auto doc = dynamic_cast<dom::HTMLBackend *>(ref().doc.get());
        if(auto it = doc->sheets.find(static_cast<lxb_html_element_t *>(ref())); it != std::end(doc->sheets))
            if(auto rule = ref().item(CSSStyleSheet::get(it->second), index))
                return CSSRule::from(ctx, std::move(*rule));
    }
    return JS_UNDEFINED;
}

JSValue CSSRuleList::length(JSContext *ctx) const
{
    auto doc = dynamic_cast<dom::HTMLBackend *>(ref().doc.get());
    if(auto it = doc->sheets.find(static_cast<lxb_html_element_t *>(ref())); it != std::end(doc->sheets))
        return bridge::Number{ctx, ref().length(CSSStyleSheet::get(it->second))};
    return JS_UNDEFINED;
}

JSValue CSSRuleList::item(JSContext *ctx, bridge::Number n) const
{
    auto doc = dynamic_cast<dom::HTMLBackend *>(ref().doc.get());
    if(auto it = doc->sheets.find(static_cast<lxb_html_element_t *>(ref())); it != std::end(doc->sheets))
        if(auto rule = ref().item(CSSStyleSheet::get(it->second), n))
            return CSSRule::from(ctx, std::move(*rule), it->second);
    return JS_NULL;
}

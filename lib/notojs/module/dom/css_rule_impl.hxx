JSValue CSSRule::parentStyleSheet(JSContext *ctx) const
{
    if(!ref().attached()) return JS_NULL;

    auto *doc = dynamic_cast<dom::HTMLBackend *>(ref().doc.get());
    if(auto it = doc->sheets.find(static_cast<lxb_html_element_t *>(ref().node));
        it != std::end(doc->sheets))
        return JS_DupValue(ctx, it->second);
    return JS_NULL;
}

JSValue CSSRuleList::make_rule(JSContext *ctx, dom::CSSRule &&rule, JSValue owner) const
{
    auto *doc = dynamic_cast<dom::HTMLBackend *>(ref().doc.get());
    if(auto cached = doc->rules.find(rule.id); cached != std::end(doc->rules))
        return JS_DupValue(ctx, cached->second);

    auto const id = rule.id;
    JSValue result = CSSRule::from(ctx, std::move(rule), owner);
    doc->rules.emplace(id, result);
    return result;
}

JSValue CSSRuleList::get_property(JSContext *ctx, char const *n) const
{
    if(auto index = u64(n); index >= 0)
    {
        auto *doc = dynamic_cast<dom::HTMLBackend *>(ref().doc.get());
        if(auto sheet = doc->sheets.find(static_cast<lxb_html_element_t *>(ref()));
            sheet != std::end(doc->sheets))
            if(auto rule = ref().item(CSSStyleSheet::get(sheet->second), index))
                return make_rule(ctx, std::move(*rule), sheet->second);
    }
    return JS_UNDEFINED;
}

JSValue CSSRuleList::length(JSContext *ctx) const
{
    auto *doc = dynamic_cast<dom::HTMLBackend *>(ref().doc.get());
    if(auto it = doc->sheets.find(static_cast<lxb_html_element_t *>(ref())); it != std::end(doc->sheets))
        return bridge::Number{ctx, ref().length(CSSStyleSheet::get(it->second))};
    return JS_UNDEFINED;
}

JSValue CSSRuleList::item(JSContext *ctx, bridge::Number n) const
{
    auto *doc = dynamic_cast<dom::HTMLBackend *>(ref().doc.get());
    if(auto sheet = doc->sheets.find(static_cast<lxb_html_element_t *>(ref()));
        sheet != std::end(doc->sheets))
        if(auto rule = ref().item(CSSStyleSheet::get(sheet->second), n))
            return make_rule(ctx, std::move(*rule), sheet->second);
    return JS_NULL;
}

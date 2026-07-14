#include <bridge.hpp>
namespace {

struct Headers_
{
    std::string header;
};

struct Request;
struct Headers : bridge::Interface<Headers, Headers_>
{
    Headers() = default;

    Headers(JSContext *ctx, JSValue val)
    : Base{ctx, val} {}

    Headers(bridge::String string)
    {
        ref().header = static_cast<std::string>(string);
    }

    void set_value(JSContext *ctx, bridge::String value)
    {
        ref().header = static_cast<std::string>(value);
    }

    JSValue get_value(JSContext *ctx) const
    {
        return bridge::String(ctx, ref().header);
    }

    using ctor = bridge::Constructor
    <
        Headers(),
        Headers(bridge::String)
    >;

    static JSCFunctionListEntry const funcs[];
    friend class Request;
};

JSCFunctionListEntry const Headers::funcs[] = {
    JS_CGETSET_DEF("value", &bridge::Getter<&Headers::get_value>, &bridge::Setter<&Headers::set_value>)
};

struct Request_ : Headers_
{
    std::string body;
};

struct Request : bridge::Interface<Request, Request_>
{
    Request() = default;

    Request(JSContext *ctx, JSValue val)
    : Base{ctx, val} {}

    Request(Request &&request)
    :  bridge::Interface<Request, Request_>(std::move(request)) {}

    Request(Request base, Headers headers)
    {
        ref().header = headers.ref().header;
        ref().body = base.ref().body;
    }

    Request(bridge::String body)
    {
        ref().body = static_cast<std::string>(body);
    }

    Request(Headers headers, bridge::String body)
    {
        ref().header = headers.ref().header;
        ref().body = static_cast<std::string>(body);
    }

    Request(bridge::String headers, bridge::String body)
    {
        ref().header = static_cast<std::string>(headers);
        ref().body = static_cast<std::string>(body);
    }

    void set_body(JSContext *, bridge::String body)
    {
        ref().body = static_cast<std::string>(body);
    }

    JSValue get_body(JSContext *ctx) const
    {
        return bridge::String(ctx, ref().body);
    }

    JSValue get_headers(JSContext *ctx, JSValue self)
    {
        return Headers::from(ctx, ref(), self);
    }

    void set_headers_string(JSContext *, bridge::String headers)
    {
        ref().header = static_cast<std::string>(headers);
    }

    void set_headers_headers(JSContext *, Headers headers)
    {
        ref().header = static_cast<std::string>(headers.ref().header);
    }

    using ctor = bridge::Constructor
    <
        Request(),
        Request(bridge::String),
        Request(Request, Headers),
        Request(Headers, bridge::String),
        Request(bridge::String, bridge::String)
    >;

    using header_ = bridge::Setters
    <
        &Request::set_headers_string,
        &Request::set_headers_headers
    >;

    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const Request::funcs[] = {
    JS_CGETSET_DEF("body", &bridge::Getter<&Request::get_body>, &bridge::Setter<&Request::set_body>),
    JS_CGETSET_DEF("headers", &bridge::Getter<&Request::get_headers>, &Request::header_::invoke)
};

JSValue bodyof(JSContext *ctx, JSValue self, int argc, JSValue *argv)
{
    if(auto request = Request::ctor::invoke(ctx, argc, argv); request)
        return request->get_body(ctx);
    return JS_EXCEPTION;
}

JSCFunctionListEntry const func[] = {
    JS_CFUNC_DEF("bodyof", 1, &bodyof)
};

} // namespace

static int init(JSContext *ctx, JSModuleDef *m)
{
    Headers::init(ctx, m);
    Request::init(ctx, m);
    return JS_SetModuleExportList(ctx, m, func, sizeof(func)/sizeof(func[0]));
}

extern "C" {

JSModuleDef *js_init_module(JSContext *ctx, const char *module_name)
{
    JSModuleDef *mod = JS_NewCModule(ctx, module_name, &init);
    if(!mod) return NULL;

    JS_AddModuleExport(ctx, mod, Headers::name());
    JS_AddModuleExport(ctx, mod, Request::name());
    JS_AddModuleExportList(ctx, mod, func, sizeof(func)/sizeof(func[0]));
    return mod;
}

} // extern "C"

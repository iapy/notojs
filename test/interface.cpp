#include <bridge.hpp>
namespace {

struct ITest : bridge::Interface<ITest, void*>
{
    virtual std::string get_name() = 0;
    virtual ~ITest() {}
};

struct A : bridge::Interface<A, std::string>
{
    A(std::reference_wrapper<std::string> &&rw) : Base(std::move(rw)) {}

    A(bridge::String value)
    {
        ref() = value;
    }

    struct I : Base::I<I, ITest>
    {
        using Base::Base;

        std::string get_name() override
        {
            return ref;
        }
    };

    using impl = bridge::Implements
    <
        I
    >;

    using ctor = bridge::Constructor
    <
        A(bridge::String)
    >;
};

struct B : bridge::Interface<B, std::int32_t>
{
    B(bridge::Number value)
    {
        ref() = static_cast<std::int32_t>(value);
    }

    struct I : Base::I<I, ITest>
    {
        using Base::Base;

        std::string get_name() override
        {
            return std::to_string(ref);
        }
    };

    using impl = bridge::Implements
    <
        I
    >;

    using ctor = bridge::Constructor
    <
        B(bridge::Number)
    >;
};

struct C : bridge::Interface<C, std::string, A>
{
    C(std::reference_wrapper<std::string> &&rw) : Base(std::move(rw)) {}
    C(bridge::String value)
    {
        ref() = value;
    }

    using ctor = bridge::Constructor
    <
        C(bridge::String)
    >;
};

struct D : bridge::Interface<D, std::string, C>
{
    D(bridge::String value)
    {
        ref() = value;
    }

    struct I : Base::I<I, ITest>
    {
        using Base::Base;

        std::string get_name() override
        {
            return "D:" + ref;
        }
    };

    using impl = bridge::Implements
    <
        I
    >;

    using ctor = bridge::Constructor
    <
        D(bridge::String)
    >;
};

JSValue get_name(JSContext *ctx, ITest::Impl value)
{
    return bridge::String(ctx, value->get_name());
}

JSCFunctionListEntry const func[] = {
    JS_CFUNC_DEF("get_name", 1, &bridge::Function<&get_name>::invoke)
};

} // namespace

static int init(JSContext *ctx, JSModuleDef *m)
{
    A::init(ctx, m);
    B::init(ctx, m);
    C::init(ctx, m);
    D::init(ctx, m);
    return JS_SetModuleExportList(ctx, m, func, sizeof(func)/sizeof(func[0]));
}

extern "C" {

JSModuleDef *js_init_module(JSContext *ctx, const char *module_name)
{
    JSModuleDef *mod = JS_NewCModule(ctx, module_name, &init);
    if(!mod) return NULL;

    JS_AddModuleExportList(ctx, mod, func, sizeof(func)/sizeof(func[0]));
    JS_AddModuleExport(ctx, mod, A::name());
    JS_AddModuleExport(ctx, mod, B::name());
    JS_AddModuleExport(ctx, mod, C::name());
    JS_AddModuleExport(ctx, mod, D::name());
    return mod;
}

} // extern "C"


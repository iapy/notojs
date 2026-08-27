#pragma once
#include <bridge.hpp>

namespace notojs {

struct ScriptConfig : bridge::Struct<ScriptConfig>
{
    struct Scope : bridge::String
    {
        using bridge::String::String;
        static bool valid(JSContext *ctx, JSValue *value, std::string &message)
        {
            Scope r(ctx, *value);
            if(auto rv = static_cast<std::string_view>(r);
                (rv != "cell" && rv != "notebook"))
            {
                message.append("invalid scope [");
                message.append(std::begin(rv), std::end(rv));
                message.append("]");
                return false;
            }
            return true;
        }
    };

    BRIDGE_DEFINE_STRUCT(ScriptConfig);
    static constexpr auto fields = bridge::fields(
        bridge::field<Scope>("scope")
    );
};

} // namespace notojs

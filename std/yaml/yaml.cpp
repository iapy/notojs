#include <yaml-cpp/yaml.h>
#include <bridge.hpp>

#include <algorithm>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

namespace {

JSValue parse(JSContext *ctx, bridge::String source)
{
    static constexpr std::string_view str_tag{"tag:yaml.org,2002:str"};
    static constexpr std::string_view bool_tag{"tag:yaml.org,2002:bool"};
    static constexpr std::string_view int_tag{"tag:yaml.org,2002:int"};
    static constexpr std::string_view float_tag{"tag:yaml.org,2002:float"};

    try
    {
        std::vector<YAML::Node> parents;
        return boost::hana::fix([ctx, &parents](auto self, YAML::Node const &node) -> JSValue {
            switch(node.Type())
            {
            case YAML::NodeType::Undefined:
                return JS_UNDEFINED;
            case YAML::NodeType::Null:
                return JS_NULL;
            case YAML::NodeType::Scalar:
            {
                auto const &value = node.Scalar();
                auto const &tag = node.Tag();

                if(tag == "!" || tag == str_tag)
                    return JS_NewStringLen(ctx, value.data(), value.size());

                bool boolean;
                if((tag == "?" || tag == bool_tag) && YAML::convert<bool>::decode(node, boolean))
                    return JS_NewBool(ctx, boolean);
                if(tag == bool_tag)
                    return JS_ThrowSyntaxError(ctx, "Invalid YAML boolean [%s]", value.c_str());

                std::int64_t integer;
                if((tag == "?" || tag == int_tag) && YAML::convert<std::int64_t>::decode(node, integer))
                    return JS_NewInt64(ctx, integer);

                std::uint64_t unsigned_integer;
                if((tag == "?" || tag == int_tag) && YAML::convert<std::uint64_t>::decode(node, unsigned_integer))
                    return unsigned_integer <= static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())
                        ? JS_NewInt64(ctx, static_cast<std::int64_t>(unsigned_integer))
                        : JS_NewFloat64(ctx, static_cast<double>(unsigned_integer));
                if(tag == int_tag)
                    return JS_ThrowSyntaxError(ctx, "Invalid YAML integer [%s]", value.c_str());

                double number;
                if((tag == "?" || tag == float_tag) && YAML::convert<double>::decode(node, number))
                    return JS_NewFloat64(ctx, number);
                if(tag == float_tag)
                    return JS_ThrowSyntaxError(ctx, "Invalid YAML number [%s]", value.c_str());

                return JS_NewStringLen(ctx, value.data(), value.size());
            }
            case YAML::NodeType::Sequence:
            case YAML::NodeType::Map:
                break;
            }

            if(std::find(std::begin(parents), std::end(parents), node) != std::end(parents))
                return JS_ThrowTypeError(ctx, "Cyclic YAML aliases are not supported");

            parents.push_back(node);

            JSValue result;
            if(node.IsSequence())
            {
                if(node.size() > std::numeric_limits<std::uint32_t>::max())
                {
                    parents.pop_back();
                    return JS_ThrowRangeError(ctx, "YAML sequence is too large");
                }

                result = JS_NewArray(ctx);
                if(!JS_IsException(result))
                {
                    for(std::uint32_t i = 0; i < node.size(); ++i)
                    {
                        JSValue value = self(node[i]);
                        if(JS_IsException(value) || JS_SetPropertyUint32(ctx, result, i, value) < 0)
                        {
                            JS_FreeValue(ctx, result);
                            result = JS_EXCEPTION;
                            break;
                        }
                    }
                }
            }
            else
            {
                result = JS_NewObject(ctx);
                if(!JS_IsException(result))
                {
                    for(auto const &entry : node)
                    {
                        std::string key;
                        if(entry.first.IsScalar())
                            key = entry.first.Scalar();
                        else if(entry.first.IsNull())
                            key = "null";
                        else
                        {
                            JS_FreeValue(ctx, result);
                            result = JS_ThrowTypeError(ctx, "YAML mappings require scalar keys");
                            break;
                        }

                        JSValue value = self(entry.second);
                        if(JS_IsException(value))
                        {
                            JS_FreeValue(ctx, result);
                            result = value;
                            break;
                        }

                        JSAtom atom = JS_NewAtomLen(ctx, key.data(), key.size());
                        int const status = JS_DefinePropertyValue(ctx, result, atom, value, JS_PROP_C_W_E);
                        JS_FreeAtom(ctx, atom);
                        if(status < 0)
                        {
                            JS_FreeValue(ctx, result);
                            result = JS_EXCEPTION;
                            break;
                        }
                    }
                }
            }

            parents.pop_back();
            return result;
        })(YAML::Load(static_cast<std::string>(source)));
    }
    catch(YAML::Exception const &error)
    {
        return JS_ThrowSyntaxError(ctx, "%s", error.what());
    }
    catch(std::exception const &error)
    {
        return JS_ThrowInternalError(ctx, "%s", error.what());
    }
}

JSValue stringify(JSContext *ctx, bridge::Value value)
{
    JSValue json = JS_JSONStringify(ctx, value, JS_UNDEFINED, JS_UNDEFINED);
    if(JS_IsException(json) || JS_IsUndefined(json)) return json;

    std::size_t size;
    char const *data = JS_ToCStringLen(ctx, &size, json);
    if(!data)
    {
        JS_FreeValue(ctx, json);
        return JS_EXCEPTION;
    }

    try
    {
        YAML::Node document = YAML::Load(std::string(data, size));
        boost::hana::fix([](auto self, YAML::Node node) -> void {
            if(node.IsScalar() && node.Tag() == "!")
            {
                bool ambiguous = true;
                try
                {
                    YAML::Node const plain = YAML::Load(node.Scalar());
                    bool boolean;
                    std::int64_t integer;
                    std::uint64_t unsigned_integer;
                    double number;
                    ambiguous = !plain.IsScalar()
                        || YAML::convert<bool>::decode(plain, boolean)
                        || YAML::convert<std::int64_t>::decode(plain, integer)
                        || YAML::convert<std::uint64_t>::decode(plain, unsigned_integer)
                        || YAML::convert<double>::decode(plain, number);
                }
                catch(YAML::Exception const &) {}
                if(ambiguous) node.SetTag("tag:yaml.org,2002:str");
            }
            else if(node.IsSequence())
            {
                node.SetStyle(YAML::EmitterStyle::Block);
                for(auto child : node) self(child);
            }
            else if(node.IsMap())
            {
                node.SetStyle(YAML::EmitterStyle::Block);
                for(auto entry : node) self(entry.second);
            }
        })(document);

        std::string const output = YAML::Dump(document);
        JS_FreeCString(ctx, data);
        JS_FreeValue(ctx, json);
        return JS_NewStringLen(ctx, output.data(), output.size());
    }
    catch(YAML::Exception const &error)
    {
        JS_FreeCString(ctx, data);
        JS_FreeValue(ctx, json);
        return JS_ThrowInternalError(ctx, "%s", error.what());
    }
    catch(std::exception const &error)
    {
        JS_FreeCString(ctx, data);
        JS_FreeValue(ctx, json);
        return JS_ThrowInternalError(ctx, "%s", error.what());
    }
}

struct YAML : bridge::Interface<YAML>
{
    static constexpr bool constructible = false;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const YAML::funcs[] = {
    JS_CFUNC_DEF("parse", 1, bridge::Function<parse>::invoke),
    JS_CFUNC_DEF("stringify", 1, bridge::Function<stringify>::invoke)
};

int init(JSContext *ctx, JSModuleDef *m)
{
    YAML::init(ctx);
    JSValue yaml = YAML::ctor(ctx);
    if(JS_IsException(yaml)) return -1;

    if(JS_PreventExtensions(ctx, yaml) < 0)
    {
        JS_FreeValue(ctx, yaml);
        return -1;
    }
    return JS_SetModuleExport(ctx, m, "default", yaml);
}

} // namespace

extern "C" {

JSModuleDef *js_init_module(JSContext *ctx, const char *name)
{
    JSModuleDef *mod = JS_NewCModule(ctx, name, init);
    if(!mod) return NULL;

    JS_AddModuleExport(ctx, mod, "default");
    return mod;
}

} // extern "C"

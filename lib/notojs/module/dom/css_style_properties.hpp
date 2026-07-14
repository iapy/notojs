#include <notojs/module/dom/css_style_declaration.hpp>
#include <variant>
#include <map>

namespace notojs::dom {

struct CSSStyleProperties : CSSStyleDeclaration
{
    template<typename T>
    CSSStyleProperties(HTMLElement const &el, std::in_place_type_t<T> ip)
    : CSSStyleDeclaration{el.doc, el}, impl{ip}
    {
        if constexpr (std::is_same_v<T, Comp>)
            std::get<Comp>(impl).init(el);
    }

    struct Attr
    {
        void free(HTMLElement &) const;
        JSValue cssText(HTMLElement const &) const;
        JSValue item(HTMLElement const &, std::int32_t) const;
        JSValue getPropertyPriority(HTMLElement const &, std::string_view const &) const;
        JSValue getPropertyValue(HTMLElement const &, std::string_view const &) const;
        JSValue removeProperty(HTMLElement &, std::string_view const &);
        void setProperty(HTMLElement &, std::string_view const &, std::string_view const &);
        void setProperty(HTMLElement &, std::string_view const &, std::string_view const &, std::string_view const &);

        std::uint32_t length(HTMLElement const &) const;

        JSValue get_property(HTMLElement const &, char const *) const;
        bool has_property(HTMLElement const &, char const *) const;
        void set_property(HTMLElement &, char const *, std::string_view const &);

        JSValue names(HTMLElement const &) const;
        std::vector<std::pair<std::size_t, std::size_t>> bounds(HTMLElement const &) const;
    };

    struct Comp
    {
        void free(HTMLElement &) const;
        void init(HTMLElement const &) const;
        std::uint32_t length(HTMLElement const &, Attr const &) const;
        JSValue item(HTMLElement const &, Attr const &, std::int32_t) const;
        JSValue getPropertyValue(HTMLElement const &, Attr const &, std::string_view const &) const;

        JSValue get_property(HTMLElement const &, Attr const &, char const *) const;
        bool has_property(HTMLElement const &, Attr const &, char const *) const;

        JSValue names(HTMLElement const &, Attr const &a) const;

    private:
        mutable std::map<std::string, std::pair<std::string, std::uint32_t>> style;
        mutable std::uint32_t gen{std::numeric_limits<std::uint32_t>::max()};
        void update(HTMLElement const &el) const;
    };

    std::variant<Attr, Comp> impl;
};

} // namespace notojs::dom

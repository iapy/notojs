#pragma once
#include <lexbor/css/css.h>

#include <boost/config.hpp>
#include <optional>
#include <cstdint>
#include <limits>
#include <memory>
#include <string>
#include <vector>

namespace notojs::dom {

struct HTMLElement;

struct CSSStyleSheetState
{
    struct Deleter
    {
        BOOST_FORCEINLINE void operator () (lxb_css_parser_t *parser) const
        {
            lxb_css_parser_destroy(parser, true);
        }

        BOOST_FORCEINLINE void operator () (lxb_css_stylesheet_t *sheet) const
        {
            lxb_css_stylesheet_destroy(sheet, true);
        }
    };

    CSSStyleSheetState() = default;
    CSSStyleSheetState(CSSStyleSheetState &&) = delete;
    CSSStyleSheetState(CSSStyleSheetState const &) = delete;

    bool update(HTMLElement const &);

    BOOST_FORCEINLINE lxb_css_rule_t *root() const
    {
        return sst ? sst->root : nullptr;
    }

    BOOST_FORCEINLINE std::string const *source() const
    {
        return data ? &*data : nullptr;
    }

    BOOST_FORCEINLINE std::size_t ruleCount() const { return rule_ids.size(); }
    lxb_css_rule_t *rule(std::size_t) const;
    std::optional<std::uint64_t> ruleId(std::size_t) const;
    std::optional<std::size_t> ruleIndex(std::uint64_t) const;

private:
    std::optional<std::string> data;
    std::unique_ptr<lxb_css_parser_t, Deleter> parser;
    std::unique_ptr<lxb_css_stylesheet_t, Deleter> sst;
    std::vector<std::uint64_t> rule_ids;
    std::optional<std::vector<std::uint64_t>> pending_rule_ids;
    std::uint32_t gen{std::numeric_limits<std::uint32_t>::max()};
    bool valid{false};

    friend struct CSSStyleSheet;
};

} // namespace notojs::dom

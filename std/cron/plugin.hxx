#pragma once
#include <plugin.hpp>
#include <string>

namespace NOTOJS_UNIQUE_NAMESPACE {

struct Plugin : notojs::IPlugin
{
    Plugin(boost::property_tree::ptree const &);
    void set_context(JSContext *, JSValue &) override;

    bool run() override;
    void end() override;
    void removed(std::string const &) override;
    bool updated(std::string const &) override;

private:
    boost::optional<std::string> prefix;
};

} // namespace NOTOJS_UNIQUE_NAMESPACE

#include <plugin.hpp>
#include <plugin.hxx>

extern "C" {

notojs::IPlugin *notojs_plugin(boost::property_tree::ptree const &pt)
{
    return new NOTOJS_UNIQUE_NAMESPACE::Plugin(pt);
}

} // extern "C"

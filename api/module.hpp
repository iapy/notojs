#pragma once
#include <boost/url.hpp>

#include "bridge.hpp"
#include <filesystem>

namespace notojs::core {
namespace facade {

JSValue html(JSContext *,
    std::string const &);

JSValue image(JSContext *,
    boost::urls::url const &);

JSValue image(JSContext *,
    std::uint8_t const *data,
    std::size_t size,
    std::string const &mime = "");

JSValue markdown(JSContext *,
    std::string const &);

JSValue svg(JSContext *,
    std::string const &);

JSValue xml(JSContext *,
    std::string const &);

} // namespace facade
} // namespace notojs::core

namespace noto::core {
using namespace notojs::core::facade;
} // namespace noto::core

namespace notojs::fs {

struct IPath : bridge::Interface<IPath, void*>
{
    virtual std::pair<std::filesystem::path, bool> native() const = 0;
    struct Static
    {
        static JSValue make(JSContext *, std::filesystem::path &&);
        static std::filesystem::path n2v(std::filesystem::path const &);
        static std::filesystem::path v2n(std::filesystem::path const &);
    };
    virtual ~IPath() {}
};

void init(JSContext *);

namespace facade {

using Path = notojs::fs::IPath::Impl;

} // namespace facade
} // namespace notojs::fs

namespace noto::fs {
using namespace notojs::fs::facade;
} // namespace noto::fs

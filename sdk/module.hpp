#pragma once

#ifndef NOTOJS_VERSION_NS
#ifndef NOTOJS_INTERNAL_MODULE
#cmakedefine NOTOJS_VERSION_NS @NOTOJS_VERSION_NS@
#else
#error "NOTOJS_VERSION_NS must be defined"
#endif
#endif

#include <boost/url.hpp>
#include <bridge.hpp>
#include <filesystem>

namespace lmdb {

class dbi;
class txn;
class val;

} // namespace lmdb

namespace notojs::NOTOJS_VERSION_NS {
namespace core {

struct HTML : bridge::Interface<HTML>
{
    using Base::Base;
    struct Methods
    {
        virtual bool json() const = 0;
        virtual std::string data() const = 0;
        virtual ~Methods() {}
    };

    static JSValue make(JSContext *, std::string const &, bool json = true);
};

struct Image : bridge::Interface<Image>
{
    using Base::Base;
    struct Methods
    {
        virtual std::string data() const = 0;
        virtual ~Methods() {}
    };

    static JSValue make(JSContext *, boost::urls::url const &);
    static JSValue make(JSContext *, std::uint8_t const *data, std::size_t size, std::string const &mime = "");
};

struct Markdown : bridge::Interface<Markdown>
{
    using Base::Base;
    struct Methods
    {
        virtual std::string data() const = 0;
        virtual ~Methods() {}
    };

    static JSValue make(JSContext *, std::string const &);
};

struct SVG : bridge::Interface<SVG>
{
    using Base::Base;
    struct Methods
    {
        virtual std::string data() const = 0;
        virtual ~Methods() = default;
    };

    static JSValue make(JSContext *, std::string const &);
};

struct XML : bridge::Interface<XML>
{
    using Base::Base;
    struct Methods
    {
        virtual std::string data() const = 0;
        virtual ~Methods() = default;
    };

    static JSValue make(JSContext *, std::string const &);
};

} // namespace core

struct db final
{
    struct Handle : bridge::Interface<Handle>
    {
        using Base::Base;
        struct type
        {
            lmdb::txn *tx{nullptr};
            lmdb::dbi *db{nullptr};
        };
        struct Methods
        {
            struct ref : private std::reference_wrapper<Handle::type const>
            {
                ref() = delete;
                ref(ref &&) = delete;
                ref(ref const &) = delete;

                BOOST_FORCEINLINE ref(Handle::type const &r)
                : std::reference_wrapper<Handle::type const>(r) {}

                BOOST_FORCEINLINE Handle::type const *operator -> ()
                { return &std::reference_wrapper<Handle::type const>::get(); }
            };

            virtual ref get() const = 0;
            virtual ~Methods() = default;
        };
    };

    using Allocator = std::variant
    <
        std::monostate,
        std::array<std::uint8_t, 9>,
        bridge::Strong<bridge::String>
    >;

    enum class Type
    {
        Number,
        Object,
        String
    };

    static Type type(lmdb::val const &);
    static void set(lmdb::val &, bridge::String &);

    template<typename T>
    static auto set(lmdb::val &, T &, Allocator &) -> std::enable_if_t
    <
        std::is_same_v<T, bridge::Number>
    ||  std::is_same_v<T, bridge::String>
    ||  std::is_same_v<T, bridge::Value>
    >;

    static JSValue get(JSContext *, lmdb::val const &);

    static std::uint64_t db_to_host(std::uint64_t);
    static std::uint64_t host_to_db(std::uint64_t);

    struct Interface_
    {
        virtual ~Interface_() = default;
        virtual void init(JSRuntime *) const = 0;
        virtual void init(JSContext *) const = 0;
        virtual Handle::type& from(JSValue value) const = 0;
        virtual JSValue from(JSContext*, Handle::type &&) const = 0;
    };

    template<typename Impl>
    struct Interface : bridge::Interface<Impl, Handle::type>
    {
        using Base = bridge::Interface<Impl, Handle::type>;
        using ctor = bridge::Unconstructable<Impl>;

        struct Interface_ : db::Interface_
        {
            void init(JSContext *c) const override
            {
                Base::init(c);
            }
            void init(JSRuntime *r) const override
            {
                Base::init(r);
            }
            Handle::type& from(JSValue value) const override
            {
                return Base::get(value);
            }
            JSValue from(JSContext*c, Handle::type &&handle) const override
            {
                return Base::from(c, std::move(handle));
            }
        };

        BOOST_FORCEINLINE static void init(std::string const &name)
        {
            db::init(name, std::make_unique<Interface_ const>());
            Base::init();
        }
    };

    static JSValue error(JSContext *);
    static JSValue error(JSContext *, std::error_code &&);
    static JSValue error(JSContext *, std::string_view const &ec);
    static JSValue error(JSContext *, std::runtime_error const &ec);

private:
    db() = delete;
    static void init(std::string const &, std::unique_ptr<Interface_ const> &&);
};

namespace fs {

struct Path : bridge::Interface<Path>
{
    enum Flag
    {
        UNMOUNTED = 0,
        RO = 1,
        RW = 2
    };

    using Base::Base;
    struct type
    {
        std::filesystem::path path;
        Flag flag{UNMOUNTED};
    };

    struct Methods
    {
        virtual Path::type native() const = 0;
        virtual ~Methods() = default;
    };

    static JSValue make(JSContext *, std::filesystem::path &&);
    static Path::type n2v(std::filesystem::path const &);
    static Path::type v2n(std::filesystem::path const &);
};

} // namespace fs
} // namespace notojs::NOTOJS_VERSION_NS

#ifndef NOTOJS_INTERNAL_MODULE
namespace noto = notojs::NOTOJS_VERSION_NS;
#endif

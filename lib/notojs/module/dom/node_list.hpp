#pragma once
#include <notojs/module/dom/backend.hpp>

namespace notojs::dom {

struct NodeList : private std::vector<void*>
{
    NodeList(std::shared_ptr<Backend> &&doc, std::vector<void*> &&list)
    : std::vector<void*>{std::move(list)}, doc(std::move(doc)) {}

    BOOST_FORCEINLINE std::uint64_t size() const
    {
        return static_cast<std::uint64_t>(std::vector<void*>::size());
    }

    BOOST_FORCEINLINE JSValue at(std::uint64_t i, JSValue def) const
    {
        return doc->collection_item(*this, i, def);
    }

    BOOST_FORCEINLINE operator JSValue() const
    {
        return doc->collection(*this);
    }

private:
    std::shared_ptr<Backend> doc;
};

} // namespace notojs:dom

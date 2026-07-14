#include <notojs/module/dom/html_collection.hpp>
#include <utility>

namespace notojs::dom {

void HTMLCollection::init()
{
    dynamic_cast<dom::HTMLBackend *>(this->doc.get())->live.reg(*this);
    update();
}

void HTMLCollection::update() const
{
    if(dynamic_cast<dom::HTMLBackend *>(this->doc.get())->live.upd(*this, gen))
    {
        std::optional<std::string> e;
        doc->querySelectorAll(*this, sel, e).swap(out);
        if(ns != LXB_NS__UNDEF) out.erase(std::remove_if(std::begin(out), std::end(out), [ns=ns](void *node){
            return ns != static_cast<lxb_dom_node_t *>(node)->ns;
        }), std::end(out));
    }
}

void HTMLCollection::free()
{
    if(!sel.empty()) dynamic_cast<dom::HTMLBackend *>(this->doc.get())->live.erase(*this);
}

} // namespace notojs:dom

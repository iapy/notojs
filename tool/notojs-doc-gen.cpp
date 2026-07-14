#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>
#include <cstdlib>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/raw_ostream.h>
#include <notojs.hpp>

using namespace clang;
using namespace clang::tooling;
using namespace clang::ast_matchers;

static llvm::cl::OptionCategory ToolCategory("notojs-doc-gen options");

static llvm::cl::opt<std::string> OutputFile(
    "output",
    llvm::cl::desc("Output file"),
    llvm::cl::value_desc("filename"),
    llvm::cl::cat(ToolCategory));

template<typename T, typename ...Ts>
class Matcher : public MatchFinder::MatchCallback
{
protected:
    Matcher(T *model, std::array<char const *, sizeof...(Ts)> &&names)
    : model{model}, names(std::move(names)) {}

    void run(MatchFinder::MatchResult const &result) override
    {
        invoke<Ts...>(result);
    }

    virtual void reg(MatchFinder &finder) = 0;

protected:
    T *model;
    std::array<char const *, sizeof...(Ts)> names;
    virtual void run(MatchFinder::MatchResult const &result, Ts const*...) = 0;

private:
    template<typename U, typename ...Us, typename ...Vs>
    void invoke(MatchFinder::MatchResult const &result, Vs const *... args)
    {
        if(const auto *node = result.Nodes.getNodeAs<U>(names[sizeof ...(args)]))
        {
            if constexpr (0 == sizeof ...(args))
            {
                if(auto loc = node->getLocation(); loc.isInvalid()
                    || !(
                        result.SourceManager->isWrittenInMainFile(loc)
                        || std::invoke([&loc, &result]{
                            auto floc = result.SourceManager->getFileLoc(loc);
                            return result.SourceManager->getFilename(floc).ends_with(".hxx");
                        })
                    )) return;
            }
            if constexpr (sizeof ...(Us))
                invoke<Us...>(result, args..., node);
            else
                run(result, args..., node);
        }
    }
};

class ClassMatcher : public Matcher<notojs::Module, CXXRecordDecl, ClassTemplateSpecializationDecl>
{
public:
    ClassMatcher(notojs::Module *def)
    : Matcher(def, {"type", "base"}) {}

    virtual void reg(MatchFinder &finder) override
    {
        finder.addMatcher(cxxRecordDecl(
            isDefinition(),
            hasDirectBase(
                cxxBaseSpecifier(hasType(
                    classTemplateSpecializationDecl(
                        hasName("bridge::Interface")
                    ).bind(names[1])))
                )
        ).bind(names[0]), this);
    }

    virtual void run(MatchFinder::MatchResult const &result,
        CXXRecordDecl const *type, ClassTemplateSpecializationDecl const *base) override
    {
        auto [c, d] = model->make<notojs::Class>(type->getQualifiedNameAsString());
        c->name = type->getNameAsString();
        d->ctor.resize(1);

        if(const auto& args = base->getTemplateInstantiationArgs();
            args.size() == 3 && TemplateArgument::Type == args[2].getKind() && !args[2].getAsType()->isVoidType())
            if (const auto* rec = args[2].getAsType()->getAsCXXRecordDecl())
                d->base = model->find<>(rec->getQualifiedNameAsString()).first;
    }
};

class ExceptionMatcher : public Matcher<notojs::Module, CXXRecordDecl>
{
public:
    ExceptionMatcher(notojs::Module *def)
    : Matcher(def, {"type"}) {}

    virtual void reg(MatchFinder &finder) override
    {
        finder.addMatcher(cxxRecordDecl(
            isDefinition(),
            hasDirectBase(
                cxxBaseSpecifier(hasType(
                    classTemplateSpecializationDecl(
                        hasName("bridge::Exception")
                    )))
                )
        ).bind(names[0]), this);
    }

    virtual void run(MatchFinder::MatchResult const &result,
        CXXRecordDecl const *type) override
    {
        auto [c, _] = model->make<notojs::Exception>(type->getQualifiedNameAsString());
        c->name = type->getNameAsString();
    }
};

#define VERIFY_NOT(n, c, e) auto n = e; if(c) {  \
    llvm::errs() << __LINE__ << ": " #e << '\n'; \
    return;                                      \
}

class FieldMatcher : public Matcher<notojs::Module, CXXRecordDecl, CXXRecordDecl, CallExpr, StringLiteral>
{
public:
    FieldMatcher(notojs::Module *mod)
    : Matcher(mod, {"type", "innr", "func", "name"}) {}

    virtual void reg(MatchFinder &finder) override
    {
        finder.addMatcher(cxxRecordDecl(
            isDefinition(),
            hasDirectBase(
                cxxBaseSpecifier(hasType(
                    classTemplateSpecializationDecl(
                        hasName("bridge::Interface"))
                    )
                )),
            forEachDescendant(cxxRecordDecl(
                isDefinition(),
                hasDirectBase(
                    cxxBaseSpecifier(hasType(
                        classTemplateSpecializationDecl(
                            hasName("bridge::Struct"))
                        )
                    )),
                forEachDescendant(varDecl(
                    hasName("fields"),
                    isConstexpr(),
                    hasStaticStorageDuration(),
                    hasInitializer(ignoringParenImpCasts(callExpr(
                        callee(functionDecl(hasName("bridge::fields"))),
                        forEachDescendant(callExpr(
                            callee(functionDecl(hasName("bridge::field"))),
                            hasArgument(0, stringLiteral().bind(names[3]))
                        ).bind(names[2]))
                    )))
                ))
            ).bind(names[1]))
        ).bind(names[0]), this);
    }

    virtual void run(MatchFinder::MatchResult const &result,
        CXXRecordDecl const *type, CXXRecordDecl const *innr, CallExpr const *func, StringLiteral const *name) override
    {
        VERIFY_NOT(c, !c.first || !c.second, model->find<notojs::Class>(type->getQualifiedNameAsString()));
        VERIFY_NOT(*callee, !callee, func->getCallee());
        VERIFY_NOT(*decref, !decref, dyn_cast<DeclRefExpr>(callee->IgnoreParenImpCasts()));
        VERIFY_NOT(*args,   !args,   decref->getTemplateArgs());

        auto const &arg = args[0].getArgument();

        auto [s, d] = model->make<notojs::Struct>(innr->getQualifiedNameAsString(), c.second);
        s->name = innr->getNameAsString();
        s->scope = c.first->name;
        d->fields.emplace_back(name->getString(), model->make(arg.getAsType(), *result.Context));
    }
};

class ConstructorMatcher : public Matcher<notojs::Module, CXXRecordDecl, TypeAliasDecl>
{
public:
    ConstructorMatcher(notojs::Module *mod)
    : Matcher(mod, {"type", "ctor"}) {}

    virtual void reg(MatchFinder &finder) override
    {
        finder.addMatcher(cxxRecordDecl(
            isDefinition(),
            hasDirectBase(
                cxxBaseSpecifier(hasType(
                    classTemplateSpecializationDecl(
                        hasName("bridge::Interface"))
                    )
                )),
            forEachDescendant(
                typeAliasDecl(
                    hasName("ctor"),
                    hasTypeLoc(
                        loc(templateSpecializationType(
                            hasDeclaration(
                                namedDecl(hasName("bridge::Constructor"))
                            ))))
                ).bind(names[1]))
        ).bind(names[0]), this);
    }

    virtual void run(MatchFinder::MatchResult const &result,
        CXXRecordDecl const *type, TypeAliasDecl const *ctor) override
    {
        VERIFY_NOT(c, !c.first || !c.second, model->find<notojs::Class>(type->getQualifiedNameAsString()));
        VERIFY_NOT(qt, qt.isNull(), ctor->getUnderlyingType());
        VERIFY_NOT(tp, !tp,         qt.getTypePtrOrNull());
        VERIFY_NOT(tt, !tt,         tp->getAs<TemplateSpecializationType>());

        auto const &args = tt->template_arguments();
        c.second->ctor.resize(args.size());

        for(decltype(args.size()) i = 0; i < args.size(); ++i)
        {
            VERIFY_NOT(fn, !fn, args[i].getAsType()->getAs<clang::FunctionProtoType>());
            for(decltype(fn->getNumParams()) j = 0; j < fn->getNumParams(); ++j) {
                c.second->ctor[i].push_back(model->make(fn->getParamType(j), *result.Context));
            }

            // special case - self-aware constructor
            if(3 == fn->getNumParams()
                && "JSContext*"     == c.second->ctor[i][0].lock()->name
                && "JSValue"        == c.second->ctor[i][1].lock()->name
                && "std::monostate" == c.second->ctor[i][2].lock()->name
            ) c.second->ctor[i].clear();
        }
    }
};

class UnconstructableMatcher : public Matcher<notojs::Module, CXXRecordDecl>
{
public:
    UnconstructableMatcher(notojs::Module *mod)
    : Matcher(mod, {"type"}) {}

    virtual void reg(MatchFinder &finder) override
    {
        finder.addMatcher(cxxRecordDecl(
            isDefinition(),
            hasDirectBase(
                cxxBaseSpecifier(hasType(
                    classTemplateSpecializationDecl(
                        hasName("bridge::Interface"))
                    )
                )),
            forEachDescendant(
                typeAliasDecl(
                    hasName("ctor"),
                    hasTypeLoc(
                        loc(templateSpecializationType(
                            hasDeclaration(
                                namedDecl(hasName("bridge::Unconstructable"))
                            ))))
                ))
        ).bind(names[0]), this);
    }

    virtual void run(MatchFinder::MatchResult const &result,
        CXXRecordDecl const *type) override
    {
        VERIFY_NOT(c, !c.first || !c.second, model->find<notojs::Class>(type->getQualifiedNameAsString()));
        c.second->ctor.clear();
    }
};

class ValidatorMatcher : public Matcher<notojs::Module, CXXRecordDecl, CXXRecordDecl, CXXRecordDecl>
{
public:
    ValidatorMatcher(notojs::Module *mod)
    : Matcher(mod, {"type", "innr", "base"}) {}

    virtual void reg(MatchFinder &finder) override
    {
        finder.addMatcher(cxxRecordDecl(
            isDefinition(),
            hasDirectBase(
                cxxBaseSpecifier(hasType(
                    classTemplateSpecializationDecl(
                        hasName("bridge::Interface"))
                    )
                )),
            forEachDescendant(cxxRecordDecl(
                isDefinition(),
                hasDirectBase(cxxBaseSpecifier(
                    hasType(cxxRecordDecl().bind(names[2]))
                )),
                hasMethod(cxxMethodDecl(
                    isPublic(),
                    hasName("valid")
                ))
            ).bind(names[1]))
        ).bind(names[0]), this);
    }

    virtual void run(MatchFinder::MatchResult const &result,
        CXXRecordDecl const *type, CXXRecordDecl const *innr, CXXRecordDecl const *base) override
    {
        VERIFY_NOT(c, !c.first || !c.second, model->find<notojs::Class>(type->getQualifiedNameAsString()));

        auto [ptr, det] = model->make<notojs::Validator>(innr->getQualifiedNameAsString(), c.second);
        det->base = model->make<>(base->getQualifiedNameAsString()).first;
        ptr->name = innr->getNameAsString();
        ptr->scope = c.first->name;
    }
};

template<bool Static>
class MethodMatcher : public Matcher<notojs::Module, CXXRecordDecl, StringLiteral, CXXMethodDecl>
{
public:
    MethodMatcher(notojs::Module *mod)
    : Matcher(mod, {"type", "name", "func"}) {}

    virtual void reg(MatchFinder &finder) override
    {
        finder.addMatcher(varDecl(
            isDefinition(),
            hasName(Static ? "sfunc" : "funcs"),
            hasDeclContext(
                cxxRecordDecl().bind(names[0])
            ),
            hasInitializer(initListExpr(
                forEach(initListExpr(
                    hasInit(0, stringLiteral().bind(names[1])),
                    hasDescendant(designatedInitExpr(hasDescendant(
                        declRefExpr(
                            to(cxxMethodDecl(
                                hasName("invoke"),
                                ofClass(hasName("bridge::Function"))
                            ).bind(names[2]))
                        )
                    )))
                ))
            ))
        ), this);
    }

    virtual void run(MatchFinder::MatchResult const &result,
        CXXRecordDecl const *type, StringLiteral const *name, CXXMethodDecl const *func) override
    {
        VERIFY_NOT(c, !c.first || !c.second, model->find<notojs::Class>(type->getQualifiedNameAsString()));
        VERIFY_NOT(f, !f, func->getParent());
        VERIFY_NOT(s, !s, llvm::dyn_cast<ClassTemplateSpecializationDecl>(f));

        auto const visit = [&](auto const &fn) {
            notojs::Args args;
            for(unsigned i = 0; i < fn->getNumParams(); ++i)
                args.push_back(model->make(fn->getParamDecl(i)->getType(), *result.Context));

            if constexpr (!Static)
                if(!args.empty() && args[0].lock()->name == "JSValue") args.erase(args.begin());
            if(!args.empty() && args[0].lock()->name == "JSContext*") args.erase(args.begin());
            if constexpr (Static)
                c.second->statics[std::string(name->getString().data(), name->getString().size())].emplace_back(std::move(args));
            else
                c.second->methods[std::string(name->getString().data(), name->getString().size())].emplace_back(std::move(args));
        };

        auto const &args = s->getTemplateInstantiationArgs();
        VERIFY_NOT(head, !head, llvm::dyn_cast<FunctionDecl>(args[0].getAsDecl()));
        visit(head);

        for(auto const &tail: args[1].pack_elements())
        {
            VERIFY_NOT(t, !t, llvm::dyn_cast<FunctionDecl>(tail.getAsDecl()));
            visit(t);
        }
    }
};

class GetterMatcher : public Matcher<notojs::Module, CXXRecordDecl, StringLiteral>
{
public:
    GetterMatcher(notojs::Module *mod)
    : Matcher(mod, {"type", "name"}) {}

    virtual void reg(MatchFinder &finder) override
    {
        finder.addMatcher(varDecl(
            isDefinition(),
            hasName("funcs"),
            hasDeclContext(
                cxxRecordDecl().bind(names[0])
            ),
            hasInitializer(initListExpr(
                forEach(initListExpr(
                    hasInit(0, stringLiteral().bind(names[1])),
                    hasDescendant(designatedInitExpr(hasDescendant(
                        declRefExpr(
                            to(functionDecl(
                                hasName("bridge::Getter")
                            ))
                        )
                    )))
                ))
            ))
        ), this);
    }

    virtual void run(MatchFinder::MatchResult const &result,
        CXXRecordDecl const *type, StringLiteral const *name) override
    {
        VERIFY_NOT(c, !c.first || !c.second, model->find<notojs::Class>(type->getQualifiedNameAsString()));
        c.second->fields[std::string(name->getString().data(), name->getString().size())].first = true;
    }
};

class SetterMatcher : public Matcher<notojs::Module, CXXRecordDecl, StringLiteral, FunctionDecl>
{
public:
    SetterMatcher(notojs::Module *mod)
    : Matcher(mod, {"type", "name", "func"}) {}

    virtual void reg(MatchFinder &finder) override
    {
        finder.addMatcher(varDecl(
            isDefinition(),
            hasName("funcs"),
            hasDeclContext(
                cxxRecordDecl().bind(names[0])
            ),
            hasInitializer(initListExpr(
                forEach(initListExpr(
                    hasInit(0, stringLiteral().bind(names[1])),
                    hasDescendant(designatedInitExpr(hasDescendant(
                        declRefExpr(
                            to(functionDecl(
                                hasName("bridge::Setter")
                            ).bind(names[2]))
                        )
                    )))
                ))
            ))
        ), this);
    }

    virtual void run(MatchFinder::MatchResult const &result,
        CXXRecordDecl const *type, StringLiteral const *name, FunctionDecl const *func) override
    {
        VERIFY_NOT(c, !c.first || !c.second, model->find<notojs::Class>(type->getQualifiedNameAsString()));
        VERIFY_NOT(a, !a, func->getTemplateSpecializationArgs());
        VERIFY_NOT(d, !d, a->get(0).getAsDecl());
        VERIFY_NOT(f, !f, llvm::dyn_cast<clang::FunctionDecl>(d));

        c.second->fields[std::string(name->getString().data(), name->getString().size())].second.push_back(
            model->make(f->parameters().back()->getType(), *result.Context));
    }
};

class SettersMatcher : public Matcher<notojs::Module, CXXRecordDecl, StringLiteral, CXXMethodDecl>
{
public:
    SettersMatcher(notojs::Module *mod)
    : Matcher(mod, {"type", "name", "func"}) {}

    virtual void reg(MatchFinder &finder) override
    {
        finder.addMatcher(varDecl(
            isDefinition(),
            hasName("funcs"),
            hasDeclContext(
                cxxRecordDecl().bind(names[0])
            ),
            hasInitializer(initListExpr(
                forEach(initListExpr(
                    hasInit(0, stringLiteral().bind(names[1])),
                    hasDescendant(designatedInitExpr(hasDescendant(
                        declRefExpr(
                            to(cxxMethodDecl(
                                hasName("invoke"),
                                ofClass(hasName("bridge::Setters"))
                            ).bind(names[2]))
                        )
                    )))
                ))
            ))
        ), this);
    }

    virtual void run(MatchFinder::MatchResult const &result,
        CXXRecordDecl const *type, StringLiteral const *name, CXXMethodDecl const *func) override
    {
        VERIFY_NOT(c, !c.first || !c.second, model->find<notojs::Class>(type->getQualifiedNameAsString()));
        VERIFY_NOT(f, !f, func->getParent());
        VERIFY_NOT(s, !s, llvm::dyn_cast<ClassTemplateSpecializationDecl>(f));

        auto const visit = [&](auto const &fn) {
            notojs::Args args;
            for(unsigned i = 0; i < fn->getNumParams(); ++i)
                args.push_back(model->make(fn->getParamDecl(i)->getType(), *result.Context));

            c.second->fields[std::string(name->getString().data(), name->getString().size())].second.push_back(args.back());
        };

        auto const &args = s->getTemplateInstantiationArgs();
        VERIFY_NOT(head, !head, llvm::dyn_cast<FunctionDecl>(args[0].getAsDecl()));
        visit(head);

        for(auto const &tail: args[1].pack_elements())
        {
            VERIFY_NOT(t, !t, llvm::dyn_cast<FunctionDecl>(tail.getAsDecl()));
            visit(t);
        }
    }
};

class ImplementsMatcher : public Matcher<notojs::Module, CXXRecordDecl, TypeAliasDecl>
{
public:
    ImplementsMatcher(notojs::Module *mod)
    : Matcher(mod, {"type", "impl"}) {}

    virtual void reg(MatchFinder &finder) override
    {
        finder.addMatcher(cxxRecordDecl(
            isDefinition(),
            hasDirectBase(
                cxxBaseSpecifier(hasType(
                    classTemplateSpecializationDecl(
                        hasName("bridge::Interface"))
                    )
                )),
            forEachDescendant(
                typeAliasDecl(
                    hasName("impl"),
                    hasTypeLoc(
                        loc(templateSpecializationType(
                            hasDeclaration(
                                namedDecl(hasName("bridge::Implements"))
                            ))))
                ).bind(names[1]))
        ).bind(names[0]), this);
    }

    virtual void run(MatchFinder::MatchResult const &result,
        CXXRecordDecl const *type, TypeAliasDecl const *impl) override
    {
        VERIFY_NOT(c, !c.first || !c.second, model->find<notojs::Class>(type->getQualifiedNameAsString()));
        VERIFY_NOT(qt, qt.isNull(), impl->getUnderlyingType());
        VERIFY_NOT(tp, !tp,         qt.getTypePtrOrNull());
        VERIFY_NOT(tt, !tt,         tp->getAs<TemplateSpecializationType>());

        auto const &args = tt->template_arguments();
        c.second->impls.resize(args.size());

        for(decltype(args.size()) i = 0; i < args.size(); ++i)
        {
            auto const type = args[i].getAsType();
            VERIFY_NOT(d, !d, type->getAsCXXRecordDecl());

            for(const CXXBaseSpecifier &base : d->bases())
            {
                VERIFY_NOT(tu, !tu, base.getType()->getAs<TemplateSpecializationType>());
                VERIFY_NOT(ar, ar.size() != 2, tu->template_arguments());
                c.second->impls[i] = model->make(ar[1].getAsType(), *result.Context);
            }
        }
    }
};

class FunctionMatcher : Matcher<notojs::Module, VarDecl, CXXMethodDecl, StringLiteral>
{
public:
    FunctionMatcher(notojs::Module *mod)
    : Matcher(mod, {"decl", "func", "name"}) {}

    virtual void reg(MatchFinder &finder) override
    {
        finder.addMatcher(varDecl(
            isDefinition(),
            hasName("func"),
            hasInitializer(initListExpr(
                forEach(initListExpr(
                    hasInit(0, stringLiteral().bind(names[2])),
                    hasDescendant(designatedInitExpr(hasDescendant(
                        declRefExpr(
                            to(cxxMethodDecl(
                                hasName("invoke"),
                                ofClass(hasName("bridge::Function"))
                            ).bind(names[1]))
                        )
                    )))
                ))
            )),
            hasDeclContext(namespaceDecl(isAnonymous()))
        ).bind(names[0]), this);
    }

    virtual void run(MatchFinder::MatchResult const &result,
        VarDecl const *, CXXMethodDecl const *func, StringLiteral const *name) override
    {
        VERIFY_NOT(f, !f, func->getParent());
        VERIFY_NOT(s, !s, llvm::dyn_cast<ClassTemplateSpecializationDecl>(f));

        auto const visit = [&](auto const &fn) {
            notojs::Args args;
            for(unsigned i = 0; i < fn->getNumParams(); ++i)
                args.push_back(model->make(fn->getParamDecl(i)->getType(), *result.Context));

            if(!args.empty() && args[0].lock()->name == "JSContext*") args.erase(args.begin());
            model->fns[std::string(name->getString().data(), name->getString().size())].emplace_back(std::move(args));
        };

        auto const &args = s->getTemplateInstantiationArgs();
        VERIFY_NOT(head, !head, llvm::dyn_cast<FunctionDecl>(args[0].getAsDecl()));
        visit(head);

        for(auto const &tail: args[1].pack_elements())
        {
            VERIFY_NOT(t, !t, llvm::dyn_cast<FunctionDecl>(tail.getAsDecl()));
            visit(t);
        }
    }
};

class ToJSONMatcher : Matcher<notojs::Module, CXXRecordDecl>
{
public:
    ToJSONMatcher(notojs::Module *mod)
    : Matcher(mod, {"type"}) {}

    virtual void reg(MatchFinder &finder) override
    {
        finder.addMatcher(varDecl(
            isDefinition(),
            hasName("funcs"),
            hasDeclContext(
                cxxRecordDecl().bind(names[0])
            ),
            hasInitializer(initListExpr(
                forEach(initListExpr(
                    hasInit(0, stringLiteral().bind(names[1])),
                    hasDescendant(designatedInitExpr(hasDescendant(
                        declRefExpr(
                            to(cxxMethodDecl(
                                hasName("toJSON"),
                                ofClass(hasName("bridge::JSON"))
                            ))
                        )
                    )))
                ))
            ))
        ), this);
    }

    virtual void run(MatchFinder::MatchResult const &result,
        CXXRecordDecl const *type) override
    {
        VERIFY_NOT(c, !c.first || !c.second, model->find<notojs::Class>(type->getQualifiedNameAsString()));
        c.second->methods["toJSON"].emplace_back();
    }
};

int module_(ClangTool &Tool)
{
    notojs::Module mod;
    MatchFinder finder;

    ClassMatcher classMatcher{&mod};
    FieldMatcher fieldMatcher{&mod};
    ExceptionMatcher exceptionMatcher{&mod};
    MethodMatcher<0> methodMatcher{&mod};
    MethodMatcher<1> staticMatcher{&mod};
    GetterMatcher getterMatcher{&mod};
    SetterMatcher setterMatcher{&mod};
    SettersMatcher settersMatcher{&mod};
    ConstructorMatcher constructorMatcher{&mod};
    UnconstructableMatcher unconstructableMatcher{&mod};
    ValidatorMatcher validatorMatcher{&mod};
    ImplementsMatcher implementsMatcher{&mod};
    FunctionMatcher functionMatcher{&mod};
    ToJSONMatcher toJSONMatcher{&mod};

    classMatcher.reg(finder);
    fieldMatcher.reg(finder);
    exceptionMatcher.reg(finder);
    methodMatcher.reg(finder);
    staticMatcher.reg(finder);
    getterMatcher.reg(finder);
    setterMatcher.reg(finder);
    settersMatcher.reg(finder);
    constructorMatcher.reg(finder);
    unconstructableMatcher.reg(finder);
    validatorMatcher.reg(finder);
    implementsMatcher.reg(finder);
    functionMatcher.reg(finder);
    toJSONMatcher.reg(finder);

    if(EXIT_SUCCESS == Tool.run(newFrontendActionFactory(&finder).get()))
    {
        mod.dump(OutputFile.empty()
            ? std::optional<std::filesystem::path>{}
            : std::optional<std::filesystem::path>{static_cast<std::string>(OutputFile)}
        );
        return EXIT_SUCCESS;
    }
    return EXIT_FAILURE;
}

class APIInterfaceMatcher : Matcher<notojs::Header, TypeAliasDecl, CXXRecordDecl>
{
public:
    APIInterfaceMatcher(notojs::Header *hpp)
    : Matcher(hpp, {"decl", "type"}) {}

    virtual void reg(MatchFinder &finder) override
    {
        finder.addMatcher(typeAliasDecl(
            hasAncestor(namespaceDecl(anyOf(
                hasName("notojs::facade"),
                matchesName("^::?notojs::[^:]+::facade$")
            ))),
            hasTypeLoc(loc(qualType(hasDeclaration(
                cxxRecordDecl(
                    hasName("Impl"),
                    hasDeclContext(classTemplateSpecializationDecl(
                        hasName("bridge::Interface"),
                        hasTemplateArgument(0, refersToType(qualType(hasDeclaration(
                            cxxRecordDecl().bind(names[1])
                        ))))
                    ))
                )
            ))))
        ).bind(names[0]), this);
    }

    virtual void run(MatchFinder::MatchResult const &result,
        TypeAliasDecl const *decl, CXXRecordDecl const *type) override
    {
        std::string name = decl->getQualifiedNameAsString();
        name.replace(name.find("notojs::"), 8, "noto::");
        name.replace(name.find("facade::"), 8, "");

        VERIFY_NOT(c, !c.first || !c.second, model->make<notojs::Interface>(name));

        if(auto const *def = type->getDefinition()) type = def;
        for(auto const *method : type->methods())
        {
            if(!method->isVirtual() || isa<CXXDestructorDecl>(method)) continue;
            auto &sign = c.second->methods[method->getNameAsString()].emplace_back();
            sign.type = method->getReturnType().getAsString();
            for(auto const *param : method->parameters())
                sign.args.push_back(param->getType().getAsString());
        }

        for(auto const *decl : type->decls())
        {
            auto const *inner = dyn_cast<CXXRecordDecl>(decl);
            if(!inner || inner->getName() != "Static") continue;
            if(auto const *def = inner->getDefinition()) inner = def;

            for(auto const *method : inner->methods())
            {
                if(!method->isStatic()) continue;
                auto &sign = c.second->statics[method->getNameAsString()].emplace_back();
                sign.type = method->getReturnType().getAsString();
                for(auto const *param : method->parameters())
                    sign.args.push_back(param->getType().getAsString());
            }
        }
    }
};

class APIFunctionMatcher : Matcher<notojs::Header, FunctionDecl>
{
public:
    APIFunctionMatcher(notojs::Header *hpp)
    : Matcher(hpp, {"decl"}) {}

    virtual void reg(MatchFinder &finder) override
    {
        finder.addMatcher(functionDecl(
            hasAncestor(namespaceDecl(anyOf(
                hasName("notojs::facade"),
                matchesName("^::?notojs::[^:]+::facade$")
            ))),
            returns(qualType(asString("JSValue")))
        ).bind(names[0]), this);
    }

    virtual void run(MatchFinder::MatchResult const &result,
        FunctionDecl const *decl) override
    {
        std::string name = decl->getQualifiedNameAsString();
        name.replace(name.find("notojs::"), 8, "noto::");
        name.replace(name.find("facade::"), 8, "");

        auto &sign = model->fns[name].emplace_back();
        sign.type = decl->getReturnType().getAsString();
        for(auto const *param : decl->parameters())
            sign.args.push_back(param->getType().getAsString());
    }
};

int header_(ClangTool &Tool)
{
    notojs::Header hpp;
    MatchFinder finder;

    APIInterfaceMatcher apiInterfaceMatcher{&hpp};
    APIFunctionMatcher apiFunctionMatcher{&hpp};

    apiInterfaceMatcher.reg(finder);
    apiFunctionMatcher.reg(finder);

    if(EXIT_SUCCESS == Tool.run(newFrontendActionFactory(&finder).get()))
    {
        hpp.dump(OutputFile.empty()
            ? std::optional<std::filesystem::path>{}
            : std::optional<std::filesystem::path>{static_cast<std::string>(OutputFile)}
        );
        return EXIT_SUCCESS;
    }
    return EXIT_FAILURE;
}

int main(int argc, const char** argv)
{
    auto ExpectedParser = CommonOptionsParser::create(argc, argv, ToolCategory);
    if(!ExpectedParser)
    {
        llvm::errs() << ExpectedParser.takeError();
        return 1;
    }

    CommonOptionsParser& OptionsParser = ExpectedParser.get();
    ClangTool Tool(OptionsParser.getCompilations(),
                   OptionsParser.getSourcePathList());

    Tool.appendArgumentsAdjuster(
        clang::tooling::getInsertArgumentAdjuster("-Wno-everything"));

    Tool.appendArgumentsAdjuster(
        clang::tooling::getInsertArgumentAdjuster("-stdlib=libc++"));

    const auto& sources = OptionsParser.getSourcePathList();
    if(1 != sources.size())
    {
       llvm::errs() << "Wrong number of inputs provided\n";
       return EXIT_FAILURE;
    }

    if(auto e = std::filesystem::path{sources[0]}.extension(); ".cpp" == e)
        return module_(Tool);
    else if(".hpp" == e)
        return header_(Tool);

    llvm::errs() << "Unrecognized input provided\n";
    return EXIT_FAILURE;
}

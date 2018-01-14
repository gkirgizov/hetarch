#pragma once

#include "llvm/IR/Module.h"
#include "llvm/Object/ObjectFile.h"


namespace hetarch {

template<typename RetT, typename... Args>
class ISignature {

public:
    explicit ISignature() = default;
    explicit ISignature(std::string symbolName)
            : symbol{std::move(symbolName)} {}

    const std::string symbol;

    const RetT (*m_signature)(Args...) = nullptr; // only types matter
};


template<typename RetT, typename... Args>
class ObjCode : public ISignature<RetT, Args...> {

    using m_payload_t = llvm::object::OwningBinary<llvm::object::ObjectFile>;

    m_payload_t m_payload;

    llvm::object::SymbolRef m_sym;

public:
    explicit ObjCode() = default;
    ObjCode(m_payload_t payload, const std::string &mainSymbol)
            : ISignature<RetT, Args...>{mainSymbol}, m_payload{std::move(payload)}, m_sym{}
    {
        for (const auto &sym : m_payload.getBinary()->symbols()) {
            if (sym.getName().get().str() == mainSymbol) {
                m_sym = sym;
            }
        }
        // todo: avoid assert and instead kind of produce an error? because it is incorrect state.
//        assert(!(m_sym == llvm::object::SymbolRef()) && "provided symbol not found");
    };

    // Move-only type
    ObjCode(ObjCode &&) = default;
    ObjCode &operator=(ObjCode &&) = default;

    explicit operator bool() const { return bool(m_payload.getBinary()); }

    const llvm::object::ObjectFile *getBinary() const {
        return m_payload.getBinary();
    }
    const llvm::object::SymbolRef &getSymbol() const {
        return m_sym;
    }
};


class IIRModule {

    friend class CodeGen; // no need to expose getters for this->m because they're needed only for CodeGen

//protected:
    std::unique_ptr<llvm::Module> m;
    std::vector<IIRModule> m_dependencies;

public:
    explicit IIRModule(std::unique_ptr<llvm::Module> m) : m{std::move(m)} {}
    virtual ~IIRModule() = default;

    // Move-only type
    IIRModule(IIRModule &&) = default;
    IIRModule &operator=(IIRModule &&) = default;

    explicit operator bool() const { return bool(m); }
};


template<typename RetT, typename... Args>
class IRModule : public IIRModule, public ISignature<RetT, Args...> {

public:
    IRModule(std::unique_ptr<llvm::Module> m, const std::string &mainSymbol)
    : IIRModule{std::move(m)}, ISignature<RetT, Args...>{mainSymbol}
    {};

};


}

#pragma once

#include "llvm/IR/Module.h"
#include "llvm/Object/ObjectFile.h"


namespace hetarch {

template<typename RetT, typename... Args>
class ISignature {

public:
    explicit ISignature() = default;
    explicit ISignature(const std::string &symbolName)
            : symbol(symbolName) {}

    const std::string symbol;

    const RetT (*m_signature)(Args...) = nullptr; // only types matter
};


template<typename RetT, typename... Args>
class ObjCode : public ISignature<RetT, Args...> {

    // temporary
    using m_payload_t = llvm::object::OwningBinary<llvm::object::ObjectFile>;

    m_payload_t m_payload;

public:
    explicit ObjCode() = default;
    ObjCode(m_payload_t payload, const std::string &mainSymbol)
            : ISignature<RetT, Args...>{mainSymbol}, m_payload{std::move(payload)}
    {};

    // Move-only type
    ObjCode(ObjCode&&) = default;
    ObjCode &operator=(ObjCode &&) = default;

    explicit operator bool() const { return bool(m_payload.getBinary()); }

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




//class ObjCode2;
//class CodeGen2;
//
//class IRModule2 {
//
//    struct IRModuleTypedBase {
//        virtual ~IRModuleTypedBase() {};
//
////         there goes common (to what?) interface?
////        virtual ?type? getSignature();
//
//        virtual ObjCode2 compile(CodeGen2 *) const = 0;
//    };
//
//    template<typename RetT, typename... Args>
//    struct IRModuleTyped : IRModuleTypedBase {
//
////        const ISignature<RetT, Args...> m_signature;
//
////        explicit IRModuleTyped(const ISignature<RetT, Args...> &signature)
////        : m_signature(signature)
////        {}
//
//        llvm::Module m;
//
//        explicit IRModuleTyped(llvm::Module &m)
//        : m(std::move(m))
//        {};
//
//        ObjCode2 compile(CodeGen2 *codeGen) const override {
//            return codeGen.compile()
//        }
//    };
//
//
//    friend class CodeGen; // no need to expose getters for this->m because they're needed only for CodeGen
//
//    IRModuleTypedBase m_signatureHolder;
//
//    llvm::Module m;
//    std::vector <IRModule2> m_dependencies;
//
//public:
//
//    template<typename RetT, typename... Args>
//    explicit IRModule2(llvm::Module &m, const ISignature<RetT, Args...> &signature)
//            : m(std::move(m)), m_dependencies(), m_signatureHolder(IRModuleTyped(signature))
//    {};
//
////    auto getSignature() const {
////        return m_signatureHolder.
////    }
//};


}

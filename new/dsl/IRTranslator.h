#pragma once


#include <iostream>
#include <string>
#include <string_view>
#include <tuple>
#include <stack>
#include <unordered_map>
#include <memory>

//#include <boost/fusion/include/at_key.hpp>

#include "llvm/IR/Module.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/IRBuilder.h"

#include "IDSLCallable.h"
#include "../utils.h"
#include "types_map.h"


namespace hetarch {
namespace dsl {


/*class IIRTranslator {
public:
// out-of-dsl constructs

    template<typename T, bool is_const, bool is_volatile>
    virtual void accept(const Var<T, is_const, is_volatile> &var) const = 0;

// dsl function and related constructs (function is 'entry' point of ir generation)

    template<typename RetT, typename ...Args>
    virtual void accept(const DSLFunction<RetT, Args...> &func) const = 0;

// helper classes

    template<typename T>
    virtual void accept(const Seq<T> &seq) const = 0;

    // maybe i doesn't even need this if i'll put handling of Return<RetT> in DSLFunction<RetT, Args...>
    template<typename RetT>
    virtual void accept(const Return<RetT> &returnSt) const = 0;

    template<typename T>
    virtual void accept(const EAssign<T> &e) const = 0;

    template<typename RetT, typename ...Args>
    virtual void accept(const ECall<RetT, Args...> &e) const = 0;
}*/;


// temporary! just to ensure interface
//class IRTranslator : public IIRTranslator {
class IRTranslator {
    llvm::LLVMContext context{}; // todo: how to store/constrcut context?? is it completely internal thing?

    std::unique_ptr<llvm::Module> cur_module{};
    llvm::BasicBlock* cur_block{nullptr};

//    llvm::IRBuilderBase* cur_builder{nullptr};
    // todo: remove raw pointer
    llvm::IRBuilder<>* cur_builder{nullptr};

    // todo: clear it after end of translation? definetly.
//    std::unordered_map<std::string, llvm::Function*> defined_funcs{};
    std::unordered_map<std::string_view, llvm::Function*> defined_funcs{};

    const utils::map_t type_map;

    template<typename Key>
    inline auto get_type() { return utils::at_key<Key>(type_map); }
//    template<typename ...Keys>
//    inline auto get_types() { return { utils::at_key<Keys>(type_map)... }; }


public:
    explicit IRTranslator() : context{}, type_map{utils::get_map(context)} {}
//    explicit IRTranslator(const llvm::LLVMContext &context)
//    : context{context} {}

//    template<typename T>
//    void accept(const Expr<T> &expr) {} // dummy overload

public: // out-of-dsl constructs
    template<typename T, bool is_const, bool is_volatile>
    void accept(const Var<T, is_const, is_volatile> &var) {
        std::cout << "accept " << typeid(var).name() << std::endl;
    }

public: // dsl function and related constructs (function is 'entry' point of ir generation)
    // todo: return IRModule
    template<typename RetT, typename ...Args>
    void translate(const DSLFunction<RetT, Args...> &func) {
        std::cout << "accept " << typeid(func).name() << std::endl;


        // todo: can func params be handled same as Var?
        std::apply([this](const auto& ...params){
            // need zeros as toIR returns void; preceding zeros because params can be empty tuple
            std::make_tuple(0, (params.toIR(*this), 0)...); // dummy tuple construction
        }, func.params);

        func.body.toIR(*this);
        func.returnSt.toIR(*this);

        /////////////////////////////////////////////////////////
//        accept(func);

//        return IRModule<RetT, Args...>{std::move(module), funcName};
    }

//    template<typename RetT, typename ...Args>
//    void accept(const ResidentOCode<RetT, Args...> &func) {
    // check that func is really loaded?
//    }

    template<typename RetT, typename ...Args>
    void accept(const DSLFunction<RetT, Args...> &dslFun) {
        // todo: handle absence of function name; get temp name from IRBuilder???
        std::string funcName{dslFun.name};
        const auto moduleName = "module_" + funcName;

        cur_module = std::make_unique<llvm::Module>(moduleName, context);

        // Define function

        llvm::FunctionType *f_type = llvm::FunctionType::get(
                get_type<RetT>(), { get_type<Args>()... }, false
        );
        // ExternalLinkage to be able to call the func outside the module it resides in
        llvm::Function *fun = llvm::Function::Create(
                f_type, llvm::Function::ExternalLinkage, funcName, cur_module.get()
        );

        // Handle arguments
        // todo: need to iterate through tuple at runtime
//        unsigned int argNo = 0;
//        for(auto& arg : fun->args()) {
//            arg.setName(std::get<argNo++>(dslFun.params).name.data());
            // todo: parameter attributes can be added through fun.addParamAttr(unsigned ArgNo, llvm::Attribute)
//        }

        const auto blockName = moduleName + "_block" + "_entry";
        llvm::BasicBlock *entry_bb = llvm::BasicBlock::Create(context, blockName, fun);
        cur_block = entry_bb;
        cur_builder = new llvm::IRBuilder<>{entry_bb}; // todo: remove raw ref

        // that's it. now we have builder and func and module
        dslFun.body.toIR(*this); // todo: is it enough to just call it?

        // todo: check for conflicts; not just override accidentally!
        bool ok = defined_funcs.insert({dslFun.name, fun}).second;
    }

public: // helper classes
    template<typename T>
    void accept(const Seq<T> &seq) {
        std::cout << "accept " << typeid(seq).name() << std::endl;

        seq.lhs.toIR(*this);
        seq.rhs.toIR(*this);
    }

    // maybe i doesn't even need this if i'll put handling of Return<RetT> in DSLFunction<RetT, Args...>
    template<typename RetT>
    void accept(const Return<RetT> &returnSt) {
        std::cout << "accept " << typeid(returnSt).name() << std::endl;
    }

    template<typename T>
    void accept(const EAssign<T> &e) {
        std::cout << "accept " << typeid(e).name() << std::endl;
    }

    template<typename RetT, typename ...Args>
    void accept(const ECall<RetT, Args...> &e) {
        std::cout << "accept " << typeid(e).name() << std::endl;

        // todo: based on some ID (name, presumably) check is it a call to yet undefined func.
//        if (defined_funcs.find(e.callee.name) == std::end(defined_funcs)) {
//            e.callee.toIR(*this);
//        }
        // now dsl func should be defined anyway.
        // ResidentOCode presumably does just nothing when accepted (it is already loaded...)
        // so, generate Call instruction (using function name?)
//        auto func = defined_funcs[e.callee.name];
//        cur_builder->CreateCall(func,);
    }

    template<typename RetT, typename ...Args>
    void accept(const ECallLoaded<RetT, Args...> &e) {
        std::cout << "accept " << typeid(e).name() << std::endl;
    }

private:
};

template<>
void IRTranslator::accept(const Return<void> &returnSt) {
    std::cout << "accept " << typeid(returnSt).name() << std::endl;

//    auto retInst = cur_builder.CreateRetVoid();
}



template<typename T>
inline void toIRImpl(const T &irTranslatable, IRTranslator &irTranslator) {
    irTranslator.accept(irTranslatable);
}

/// Allows static polymorphism.
/// Despite allowing use of Visitor pattern (convenient there), avoids run-time overhead for virtual calls.
/// \tparam T subclass of DSLBase
//template<typename T, std::enable_if<std::is_base_of<DSLBase, T>::value>::type>
/*template<typename T>
struct IRTranslatable : virtual public DSLBase {
    void toIR(const IRTranslator &gen) const override {
        gen.accept(*static_cast<const T*>(this));
    }
};*/

//template<template<typename T> class Tdsl, typename T>
//struct IRTranslatable1 : public Expr<T> {
//    inline void toIR(const IRTranslator &gen) const override {
//        gen.accept(*static_cast<const Tdsl<T>*>(this));
//    }
//};


}
}

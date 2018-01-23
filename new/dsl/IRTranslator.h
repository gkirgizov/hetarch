#pragma once


#include <iostream>
#include <string>
#include <string_view>
#include <tuple>
#include <stack>
#include <unordered_map>
#include <memory>
#include <utility>

#include "llvm/IR/Module.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/IRBuilder.h"

//#include "../supportingClasses.h"
#include "IDSLCallable.h"
#include "ResidentObjCode.h"
#include "sequence.h"
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
}*/


// temporary! just to ensure interface
//class IRTranslator : public IIRTranslator {
class IRTranslator {
    llvm::LLVMContext context{}; // todo: how to store/constrcut context?? is it completely internal thing?

    std::unique_ptr<llvm::Module> cur_module{};
    llvm::BasicBlock* cur_block{nullptr};

//    llvm::IRBuilderBase* cur_builder{nullptr};
    // todo: remove raw pointer
    llvm::IRBuilder<>* cur_builder{nullptr};

    std::stack<llvm::Value*> vals{};

    // todo: clear it after end of translation? definetly.
//    std::unordered_map<std::string, llvm::Function*> defined_funcs{};
    std::unordered_map<std::string_view, llvm::Function*> defined_funcs{};

//    using expr_ref_t = std::reference_wrapper<ExprBase>;
//    std::unordered_map<expr_ref_t, llvm::Value*> evaluated{};
    std::unordered_map<const ExprBase*, llvm::Value*> evaluated{};
    std::stack<llvm::Value*> val_stack{};

    const utils::map_t type_map;

    template<typename Key>
    inline auto get_type() { return utils::at_key<Key>(type_map); }
//    template<typename ...Keys>
//    inline auto get_types() { return { utils::at_key<Keys>(type_map)... }; }

    template<typename RetT, typename... Args>
    inline llvm::FunctionType* get_func_type(bool isVarArg = false) {
        return llvm::FunctionType::get(
                get_type<RetT>(), { get_type<Args>()... }, isVarArg
        );
    }


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

        llvm::Value* val = nullptr;
        auto known = evaluated.find(static_cast<const ExprBase*>(&var));
//        auto known = evaluated.find(&static_cast<ExprBase>(var));
        if (known == std::end(evaluated)) {
            // how can happen that var is used for the first time; in what contexts -- handle all of them
            // todo: CreateWhat?? Alloca, Store, Load??
            // local var -- alloca + store(init)
            // arguments -- alloca + store(actual argument value)

            // todo: how to create/use globals and constants
            // global var -- load
            // const var -- ??

            auto init_val_t = get_type<T>();
            // default empty name is handled automatically by LLVM (when var.name.data() == "")
            val = cur_builder->CreateAlloca(init_val_t, nullptr, var.name.data());
            auto init_val = val2llvm(var.initial_val());
            auto initInst = cur_builder->CreateStore(init_val, val, is_volatile);

            evaluated[&var] = val;
        } else {
            val = known->second;
        }
        val_stack.push(val);
    }

private:
//     todo: overload these (also overload for signed/unsigned ints)
//            llvm::ConstantInt* init_val = llvm::ConstantInt::get(init_val_t, static_cast<uint64_t>(var.initial_val()));
//            llvm::ConstantInt* init_val = llvm::ConstantInt::getSigned(init_val_t, static_cast<int64_t>(var.initial_val()));
//            llvm::Constant* init_val = llvm::ConstantFP::get(init_val_t, static_cast<double>(var.initial_val()));
    template<typename T, typename = std::enable_if<std::is_convertible<T, uint64_t>::value>>
    auto val2llvm(T val, uint64_t type_tag = 0) {
        return llvm::ConstantInt::get(get_type<T>(), static_cast<uint64_t>(val));
    }
//    template<typename T, typename = std::enable_if<std::is_convertible<T, int64_t>::value>>
//    auto val2llvm(const VarBase<T> &var, int64_t type_tag = 0) {
//        return llvm::ConstantInt::getSigned(get_type<T>(), static_cast<int64_t>(var.initial_val()));
//    }
//    template<typename T, typename = std::enable_if<std::is_convertible<T, double>::value>>
//    auto val2llvm(const VarBase<T> &var, double type_tag = 0) {
//        return llvm::ConstantFP::get(get_type<T>(), static_cast<double>(var.initial_val()));
//    }


public: // dsl function and related constructs (function is 'entry' point of ir generation)
    // todo: return IRModule
    template<typename RetT, typename ...Args>
    auto translate(const DSLFunction<RetT, Args...> &func) {
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
        // ExternalLinkage to be able to call the func outside the module it resides in
        llvm::Function *fun = llvm::Function::Create(
                get_func_type<RetT, Args...>(), llvm::Function::ExternalLinkage, funcName, cur_module.get()
        );

        // Handle arguments
        modifyArgs(dslFun, fun);

        const auto blockName = moduleName + "_block" + "_entry";
        llvm::BasicBlock *entry_bb = llvm::BasicBlock::Create(context, blockName, fun);
        cur_block = entry_bb;
        cur_builder = new llvm::IRBuilder<>{entry_bb}; // todo: remove raw ref

        // that's it. now we have builder and func and module
        dslFun.body.toIR(*this); // todo: is it enough to just call it?

        bool no_name_conflict = defined_funcs.insert({dslFun.name, fun}).second;
        // btw: recursive functions ain't handled here
        assert(no_name_conflict && (funcName + ": dsl function name conflict!").c_str());
    }

private:
    // todo: test modifyArgs
    template<typename RetT, typename ...Args, typename Inds = std::index_sequence_for<Args...>>
    void modifyArgs(const DSLFunction<RetT, Args...> &dslFun, llvm::Function *fun) {
        std::array<llvm::Argument*, sizeof...(Args)> args;
        for(llvm::Argument* arg_ptr = fun->arg_begin(); arg_ptr != fun->arg_end(); ++arg_ptr) {
            args[arg_ptr->getArgNo()] = arg_ptr;
        }
        modifyArgsImpl(dslFun, args, Inds{});
    }

    template<typename RetT, typename ...Args, std::size_t ...I>
    void modifyArgsImpl(const DSLFunction<RetT, Args...> &dslFun,
                        std::array<llvm::Argument*, sizeof...(Args)> &args,
                        std::index_sequence<I...>) {
        auto dummy = std::make_tuple( 0, (modifyArg(std::get<I>(dslFun.params), *args[I]), 0)... );
    }

    template<typename T>
    void modifyArg(const ParamBase<T> &param, llvm::Argument &arg) {
        arg.setName(param.name.data());
        // todo: parameter attributes can be added through fun.addParamAttr(unsigned ArgNo, llvm::Attribute)
    }

public:
    template<typename T>
    void accept(const Seq<T> &seq) {
        std::cout << "accept " << typeid(seq).name() << std::endl;

        const auto n_vals = val_stack.size();
        // evaluate left expr
        seq.lhs.toIR(*this);
        assert(val_stack.size() == n_vals + 1 && "val_stack probably isn't used correctly!"); // hard-testing...
        // Sequence discards result of evaluation of left expression
        val_stack.pop();
        // evaluate right expr
        seq.rhs.toIR(*this);
    }

    template<typename RetT>
    void accept(const Return<RetT> &returnSt) {
        std::cout << "accept " << typeid(returnSt).name() << std::endl;

        returnSt.returnee.toIR(*this);
        auto retInst = cur_builder->CreateRet(vals.top());
        vals.pop();
    }

    template<typename T>
    void accept(const EAssign<T> &e) {
        std::cout << "accept " << typeid(e).name() << std::endl;

        e.rhs.toIR(*this);
        llvm::Value* rhs = val_stack.top();
        val_stack.pop();

        e.var.toIR(*this);
        llvm::Value* dest = val_stack.top();
        val_stack.pop();

        // TODO: somehow access is_volatile
        llvm::StoreInst* inst = cur_builder->CreateStore(rhs, dest);
    }

    template<typename RetT, typename ...Args>
    void accept(const ECall<RetT, Args...> &e) {
        std::cout << "accept " << typeid(e).name() << std::endl;

        // If dsl func is not defined (i.e. translated)
        if (defined_funcs.find(e.callee.name) == std::end(defined_funcs)) {
            e.callee.toIR(*this);
        }

        llvm::Function* func = defined_funcs.at(e.callee.name);
        llvm::CallInst* inst = cur_builder->CreateCall(func, eval_func_args(e.args));
    }

    template<typename AddrT, typename RetT, typename ...Args>
    void accept(const ECallLoaded<AddrT, RetT, Args...> &e) {
        std::cout << "accept " << typeid(e).name() << std::endl;

        llvm::CallInst* inst = cur_builder->CreateCall(
                get_func_type<RetT, Args...>(),
                val2llvm(e.callee.callAddr),
                eval_func_args(e.args)
        );
    }

private:
    template<typename ...Args>
    llvm::ArrayRef<llvm::Value*> eval_func_args(expr_tuple<Args...> args) {

        // Evaluate arguments left-to-right
        auto evaluated_args = std::apply([this](const Expr<Args>&... args){
            return llvm::ArrayRef({ (args.toIR(*this), val_stack.top())... });
        }, args);

        // Clear stack from evaluated arguments
        for (auto i = 0; i < sizeof...(Args); ++i) { val_stack.pop(); }

        return evaluated_args;
    }
};

template<>
void IRTranslator::accept(const Return<void> &returnSt) {
    std::cout << "accept " << typeid(returnSt).name() << std::endl;

    auto retInst = cur_builder->CreateRetVoid();
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

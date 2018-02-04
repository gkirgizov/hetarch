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

#include "../supportingClasses.h"
#include "IDSLCallable.h"
#include "ResidentObjCode.h"
#include "sequence.h"
#include "if_else.h"
#include "op.h"

#include "../utils.h"
#include "types_map.h"


namespace hetarch {
namespace dsl {


class IRTranslator {
    template<typename T>
    friend void toIRImpl(const T &irTranslatable, IRTranslator &irTranslator);

    llvm::LLVMContext context{}; // todo: how to store/constrcut context?? is it completely internal thing?

    std::unique_ptr<llvm::Module> cur_module{};

    // todo: remove pointer
//    llvm::IRBuilder<>* cur_builder{nullptr};
    std::unique_ptr<llvm::IRBuilder<>> cur_builder{};

    // todo: use const DSLBase* as key for funcs (analogous to evaluated map)
    std::unordered_map<std::string_view, llvm::Function*> defined_funcs{};
    std::stack<llvm::Function*> fun_stack{};

    // todo: One global variable context for all functions and scopes allows harsh misuse
    std::unordered_map<const ExprBase*, llvm::Value*> evaluated{};
    std::stack<llvm::Value*> val_stack{};

    const utils::map_t type_map;

    template<typename Key>
    inline auto get_type() { return utils::at_key<Key>(type_map); }

    template<typename RetT, typename... Args>
    inline llvm::FunctionType* get_func_type(bool isVarArg = false) {
        return llvm::FunctionType::get(
                get_type<RetT>(), { get_type<Args>()... }, isVarArg
        );
    }

public:
    explicit IRTranslator()
            : context{}, cur_builder{new llvm::IRBuilder<>{context}}, type_map{utils::get_map(context)} {}
//    explicit IRTranslator(llvm::LLVMContext &context)
//            : context{context}, cur_builder{new llvm::IRBuilder<>{context}}, type_map{utils::get_map(context)} {}

    void accept(const EmptyExpr &) {}

public: // out-of-dsl constructs

    // todo: test Const handling
    // todo: how to create/use globals
    template<typename T, bool is_const, bool is_volatile>
    void accept(const Var<T, is_const, is_volatile> &var) {
        llvm::Value* val_ptr = nullptr;

        auto known = evaluated.find(static_cast<const ExprBase*>(&var));
        if (known == std::end(evaluated)) {
            // Default empty name is handled automatically by LLVM (when var.name.data() == "")
            val_ptr = cur_builder->CreateAlloca(get_type<T>(), nullptr, var.name.data());
            // When var is used for the first time (this branch) its actual value is its initial value
            auto init_val = get_llvm_const(var.initial_val());
            auto initInst = cur_builder->CreateStore(init_val, val_ptr, is_volatile);
            evaluated[&var] = val_ptr;
        } else { // Variable usage
            val_ptr = known->second;
        }

        // todo: maybe return address instead and let users decide what they need: Load or Store?
        //  this way they must be aware that on the stack is the loaded value -- not val_ptr and pop it if they don't need it
        // Using SSA: each time we load value again
        llvm::Value* val = cur_builder->CreateLoad(val_ptr, is_volatile, var.name.data());
        val_stack.push(val);
    }

    // Actually, this specialisation is optional, because non-volatile const case is handled above
    template<typename T>
    void accept(const Const<T, false> &var) {
        // Non-volatile const variables can be directly substitued by its value
        val_stack.push(get_llvm_const(var.initial_val()));
    }


private:
//     todo: overload these (also overload for signed/unsigned ints)
//            llvm::ConstantInt* init_val = llvm::ConstantInt::get(init_val_t, static_cast<uint64_t>(var.initial_val()));
//            llvm::ConstantInt* init_val = llvm::ConstantInt::getSigned(init_val_t, static_cast<int64_t>(var.initial_val()));
//            llvm::Constant* init_val = llvm::ConstantFP::get(init_val_t, static_cast<double>(var.initial_val()));
    template<typename T, typename = std::enable_if<std::is_convertible<T, uint64_t>::value>>
    auto get_llvm_const(T val, uint64_t type_tag = 0) {
        return llvm::ConstantInt::get(get_type<T>(), static_cast<uint64_t>(val));
    }
//    template<typename T, typename = std::enable_if<std::is_convertible<T, int64_t>::value>>
//    auto get_llvm_const(const VarBase<T> &var, int64_t type_tag = 0) {
//        return llvm::ConstantInt::getSigned(get_type<T>(), static_cast<int64_t>(var.initial_val()));
//    }
//    template<typename T, typename = std::enable_if<std::is_convertible<T, double>::value>>
//    auto get_llvm_const(const VarBase<T> &var, double type_tag = 0) {
//        return llvm::ConstantFP::get(get_type<T>(), static_cast<double>(var.initial_val()));
//    }


public: // dsl function and related constructs (function is 'entry' point of ir generation)
    template<typename RetT, typename ...Args>
    auto translate(const DSLFunction<RetT, Args...> &fun) {

        const auto moduleName = "module_" + std::string(fun.name);
        cur_module = std::make_unique<llvm::Module>(moduleName, context);
        fun.toIR(*this);

        // todo: some cleanup?
        return IRModule<RetT, Args...>{std::move(cur_module), fun.name.data()};
    }

    template<typename AddrT, typename RetT, typename ...Args>
    void accept(const ResidentObjCode<AddrT, RetT, Args...> &func) {
        // check that func is really loaded?
    }

    template<typename RetT, typename ...Args>
    void accept(const DSLFunction<RetT, Args...> &dslFun) {
        /* todo: handle absence of function name; get temp name from IRBuilder???
         * actually,
         * store functions not by name but by ptr (referential equality)
         * use names for checks on symbol name conflicts (actually, linker later will anyway do it)
         */
        std::string funcName{dslFun.name};

        // Define function
        // ExternalLinkage is to be able to call the func outside the module it resides in
        llvm::Function *fun = llvm::Function::Create(
                get_func_type<RetT, Args...>(), llvm::Function::ExternalLinkage, funcName, cur_module.get()
        );

        // Handle arguments
        modifyArgs(dslFun, fun);

        const auto blockName = funcName + "_block" + "_entry";
        llvm::BasicBlock *entry_bb = llvm::BasicBlock::Create(context, blockName, fun);
        llvm::IRBuilderBase::InsertPoint ip = cur_builder->saveIP();

        cur_builder->SetInsertPoint(entry_bb);
        fun_stack.push(fun);
        dslFun.body.toIR(*this);
        dslFun.returnSt.toIR(*this);
        fun_stack.pop();
        cur_builder->restoreIP(ip);

        // todo: fail before translation
        bool no_name_conflict = defined_funcs.insert({dslFun.name, fun}).second;
        // Recursive functions ain't handled here
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
        seq.lhs.toIR(*this);
        // Sequence discards result of evaluation of left expression
        val_stack.pop();
        seq.rhs.toIR(*this);
        // And returns result of evaluation of right expression
    }

    template<typename T>
    void accept(const IfExpr<T> &e) {
        // Evaluate condition
        e.cond.toIR(*this);
        llvm::Value* cond = val_stack.top();
        val_stack.pop();

        // get ?unique name (or null i think is simpler. why do i even need these names?)
        llvm::BasicBlock* true_block = llvm::BasicBlock::Create(context, "", fun_stack.top());
        llvm::BasicBlock* false_block = llvm::BasicBlock::Create(context, "", fun_stack.top());

        llvm::BranchInst* inst = cur_builder->CreateCondBr(cond, true_block, false_block);

        // Create temp value to hold result of the whole expr depending on branch
        llvm::Value* result = cur_builder->CreateAlloca(get_type<T>());
        llvm::IRBuilderBase::InsertPoint ip = cur_builder->saveIP();

        cur_builder->SetInsertPoint(true_block);
        e.then_body.toIR(*this);
        cur_builder->CreateStore(val_stack.top(), result);
        val_stack.pop();

        cur_builder->SetInsertPoint(false_block);
        e.else_body.toIR(*this);
        cur_builder->CreateStore(val_stack.top(), result);
        val_stack.pop();

        cur_builder->restoreIP(ip);
        val_stack.push(result);
    }

    void accept(const IfElse &e) {
        // todo: seems, difference betwenn IfExpr and If statement is that with the latter resulted value popped from the val_stack
        // .... but IfStatement contains not Expr. that is, there is no guarantee that stack will be changed
        // .... need checks on stack size? ensure it to remaing the same at the end of evaluation of If statement?
    }

    template<typename RetT>
    void accept(const Return<RetT> &returnSt) {
        returnSt.returnee.toIR(*this);
        cur_builder->CreateRet(val_stack.top());
        val_stack.pop();
    }

    template<typename T>
    void accept(const EAssign<T> &e) {
        e.var.toIR(*this);
        // TODO: but accept(Var) leaves on the stack Loaded value, not ptr to dest!!
//        llvm::Value* dest = val_stack.top();
        val_stack.pop();
        llvm::Value* dest = evaluated.at(&e.var);

        e.rhs.toIR(*this);
        llvm::Value* rhs = val_stack.top();
        // assignment returns result of the evaluation of rhs; so don't pop
        // val_stack.pop();

        // TODO: somehow access is_volatile (refine EAssign for Var (not VarBase))
//        cur_builder->CreateStore(rhs, dest, is_volatile);
        cur_builder->CreateStore(rhs, dest);
    }

    template<typename RetT, typename ...Args>
    void accept(const ECall<RetT, Args...> &e) {
        // If dsl func is not defined (i.e. not translated)
        if (defined_funcs.find(e.callee.name) == std::end(defined_funcs)) {
            e.callee.toIR(*this);
        }

        llvm::CallInst* inst = cur_builder->CreateCall(
                defined_funcs.at(e.callee.name),
                eval_func_args(e.args)
        );
        val_stack.push(inst);
    }

    template<typename AddrT, typename RetT, typename ...Args>
    void accept(const ECallLoaded<AddrT, RetT, Args...> &e) {
        llvm::CallInst* inst = cur_builder->CreateCall(
                get_func_type<RetT, Args...>(),
                get_llvm_const(e.callee.callAddr),
                eval_func_args(e.args)
        );
        val_stack.push(inst);
    }

private:
    // todo: test: empty args; empty init list
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

public: // Operations and Casts
    template<typename To, typename From>
    void accept(const ECast<To, From> &e) {
        e.src.toIR(*this);
        llvm::Value* src = val_stack.top();
        val_stack.pop();

        llvm::Value* casted = cur_builder->CreateCast(e.op, src, get_type<To>());
        val_stack.push(casted);
    }
    template<typename To>
    void accept(const ECast<To, To> &e) {
        e.src.toIR(*this);
    }

    template<BinOps bOp, typename T1, typename T2>
    void accept(const EBinOp<bOp, T1, T2> &e) {
        e.lhs.toIR(*this);
        llvm::Value* lhs = val_stack.top();
        val_stack.pop();

        e.rhs.toIR(*this);
        llvm::Value* rhs = val_stack.top();
        val_stack.pop();

        llvm::Value* res = cur_builder->CreateBinOp(bOp, lhs, rhs);
        val_stack.push(res);
    }

    // todo: possibly specialise this template for logical ops to evaluate lazily
//    template<BinOps bOp>
//    void accept(const EBinOpLogical<bOp> &e) {
//    }
};

template<>
void IRTranslator::accept(const Return<void> &returnSt) {
    llvm::ReturnInst* inst = cur_builder->CreateRetVoid();
}


// debuggin version
template<typename T>
void toIRImpl(const T &irTranslatable, IRTranslator &irt) {
    std::cout << "accept " << typeid(irTranslatable).name() << std::endl;

    // Checking some invariants
    const auto n_vals = irt.val_stack.size();
    const auto n_funs = irt.fun_stack.size();

    irt.accept(irTranslatable);

    // hard-testing...
//    assert(irt.val_stack.size() == n_vals && "val_stack probably isn't used correctly!");
//    assert(irt.fun_stack.size() == n_funs && "fun_stack probably isn't used correctly!");
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

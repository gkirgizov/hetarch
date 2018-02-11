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
#include "dsl_base.h"
#include "ResidentObjCode.h"
#include "sequence.h"
#include "if_else.h"
#include "op.h"
#include "var.h"
#include "fun.h"

#include "../utils.h"
#include "types_map.h"


namespace hetarch {
namespace dsl {


class IRTranslator {
    template<typename T>
    friend void toIRImpl(const T &irTranslatable, IRTranslator &irTranslator);

    struct Frame {
        llvm::Function* fun;
        std::stack<llvm::Value*> val_stack{};
        std::unordered_map<const ValueBase*, llvm::Value*> evaluated{};

        explicit Frame(llvm::Function* fun) : fun{fun} {}
    };

    std::unordered_map<const CallableBase*, llvm::Function*> defined_funcs{};
    std::stack<Frame> frame_stack{};
    Frame* frame{nullptr}; // current frame
    // todo: One global variable context for all functions and scopes allows harsh misuse
//    std::stack<llvm::Function*> fun_stack{};
//    std::unordered_map<const ValueBase*, llvm::Value*> evaluated{};
//    std::stack<llvm::Value*> val_stack{};

    llvm::LLVMContext context{}; // todo: how to store/constrcut context?? is it completely internal thing?
    std::unique_ptr<llvm::Module> cur_module{};
    std::unique_ptr<llvm::IRBuilder<>> cur_builder{};

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

    void accept(const VoidExpr &) {}

    void clear() {
        cur_module.release();

        defined_funcs.clear();
        assert(frame_stack.size() == 0);
    }

public: // out-of-dsl constructs

    // todo: test Const handling
    // todo: how to create/use globals
    template<typename T, bool is_const, bool is_volatile>
    void accept(const Var<T, is_const, is_volatile> &var) {
        llvm::Value* val_ptr = nullptr;

        auto known = frame->evaluated.find(&var);
        if (known == std::end(frame->evaluated)) {
            // Default empty name is handled automatically by LLVM (when var.name().data() == "")
            val_ptr = cur_builder->CreateAlloca(get_type<T>(), nullptr, var.name().data());
            // When var is used for the first time (this branch) its actual value is its initial value
            auto init_val = get_llvm_const(var.initial_val());
            auto initInst = cur_builder->CreateStore(init_val, val_ptr, is_volatile);
            frame->evaluated[&var] = val_ptr;
        } else { // Variable usage
            val_ptr = known->second;
        }

        // todo: maybe return address instead and let users decide what they need: Load or Store?
        //  this way they must be aware that on the stack is the loaded value -- not val_ptr and pop it if they don't need it
        // Using SSA: each time we load value again
        llvm::Value* val = cur_builder->CreateLoad(val_ptr, is_volatile, var.name().data());
        frame->val_stack.push(val);
    }

    // Actually, this specialisation is optional, because non-volatile const case is handled above
    template<typename T>
    void accept(const Const<T, false> &var) {
        // Non-volatile const variables can be directly substitued by its value
        frame->val_stack.push(get_llvm_const(var.initial_val()));
    }


private:
    template<typename T, typename = typename std::enable_if_t<std::is_arithmetic_v<T>>>
    auto get_llvm_const(T val) {
        if constexpr (std::is_floating_point_v<T>) {
            return llvm::ConstantFP::get(get_type<T>(), static_cast<double>(val));
        } else if constexpr (std::is_unsigned_v<T>) {
            return llvm::ConstantInt::get(get_type<T>(), static_cast<uint64_t>(val));
        } else {
            static_assert(std::is_signed_v<T>);
            return llvm::ConstantInt::getSigned(get_type<T>(), static_cast<int64_t>(val));
        }
    }


public: // dsl function and related constructs (function is 'entry' point of ir generation)
    template<typename TdRet, typename TdBody, typename... Args>
    auto translate(const DSLFunction<TdRet, TdBody, Args...> &fun) {

        const auto moduleName = "module_" + std::string(fun.name());
        cur_module = std::make_unique<llvm::Module>(moduleName, context);
        fun.toIR(*this);

        using RetT = typename DSLFunction<TdRet, TdBody, Args...>::ret_t;
        return clear(), IRModule<RetT, Args...>{std::move(cur_module), fun.name().data()};
    }

    template<typename AddrT, typename RetT, typename ...Args>
    void accept(const ResidentObjCode<AddrT, RetT, Args...> &func) {
        // check that func is really loaded?
    }

    template<typename TdRet, typename TdBody, typename... Args>
    void accept(const DSLFunction<TdRet, TdBody, Args...> &dslFun) {
        using RetT = typename DSLFunction<TdRet, TdBody, Args...>::ret_t;
        /* todo: handle absence of function name; get temp name from IRBuilder???
         * actually,
         * store functions not by name but by ptr (referential equality)
         * use names for checks on symbol name conflicts (actually, linker later will anyway do it)
         */
        std::string funcName{dslFun.name()};

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
        frame_stack.push(Frame{fun});
        frame = &frame_stack.top();

        dslFun.body.toIR(*this);
        dslFun.returnSt.toIR(*this);

        frame_stack.pop();
        frame = &frame_stack.top();
        cur_builder->restoreIP(ip);
        bool no_name_conflict = defined_funcs.insert({&dslFun, fun}).second;
        // todo: fail before translation, not after
        // Recursive functions ain't handled here
        assert(no_name_conflict && (funcName + ": dsl function name conflict!").c_str());
    }

private:
    // todo: test modifyArgs
    template<typename TdRet, typename TdBody, typename... Args
            , typename Inds = std::index_sequence_for<Args...>>
    void modifyArgs(const DSLFunction<TdRet, TdBody, Args...> &dslFun, llvm::Function *fun) {
        std::array<llvm::Argument*, sizeof...(Args)> args;
        for(llvm::Argument* arg_ptr = fun->arg_begin(); arg_ptr != fun->arg_end(); ++arg_ptr) {
            args[arg_ptr->getArgNo()] = arg_ptr;
        }
        modifyArgsImpl(dslFun, args, Inds{});
    }

    template<typename TdRet, typename TdBody, typename ...Args
            , std::size_t ...I>
    void modifyArgsImpl(const DSLFunction<TdRet, TdBody, Args...> &dslFun,
                        std::array<llvm::Argument*, sizeof...(Args)> &args,
                        std::index_sequence<I...>) {
        auto dummy = std::make_tuple( 0, (modifyArg(std::get<I>(dslFun.args), *args[I]), 0)... );
    }

    template<typename T>
    void modifyArg(const FunArg<T> &param, llvm::Argument &arg) {
        arg.setName(param.name().data());
        // todo: parameter attributes can be added through fun.addParamAttr(unsigned ArgNo, llvm::Attribute)
    }

public:
    template<typename Td1, typename Td2>
    void accept(const Seq<Td1, Td2> &seq) {
        seq.e1.toIR(*this);
        // Sequence discards result of evaluation of left expression
        frame->val_stack.pop();
        seq.e2.toIR(*this);
        // And returns result of evaluation of right expression
    }

    template<typename TdCond, typename TdThen, typename TdElse>
    void accept(const IfExpr<TdCond, TdThen, TdElse> &e) {
        // Evaluate condition
        e.cond_expr.toIR(*this);
        llvm::Value* cond = frame->val_stack.top();
        frame->val_stack.pop();

        // get ?unique name (or null i think is simpler. why do i even need these names?)
        llvm::BasicBlock* true_block = llvm::BasicBlock::Create(context, "", frame->fun);
        llvm::BasicBlock* false_block = llvm::BasicBlock::Create(context, "", frame->fun);

        llvm::BranchInst* inst = cur_builder->CreateCondBr(cond, true_block, false_block);

        // Create temp value to hold result of the whole expr depending on branch
        using T = typename IfExpr<TdCond, TdThen, TdElse>::type;
        llvm::Value* result = cur_builder->CreateAlloca(get_type<T>());
        llvm::IRBuilderBase::InsertPoint ip = cur_builder->saveIP();

        cur_builder->SetInsertPoint(true_block);
        e.then_expr.toIR(*this);
        cur_builder->CreateStore(frame->val_stack.top(), result);
        frame->val_stack.pop();

        cur_builder->SetInsertPoint(false_block);
        e.else_expr.toIR(*this);
        cur_builder->CreateStore(frame->val_stack.top(), result);
        frame->val_stack.pop();

        cur_builder->restoreIP(ip);
        frame->val_stack.push(result);
    }

//    void accept(const IfElse &e) {
        // todo: seems, difference betwenn IfExpr and If statement is that with the latter resulted value popped from the frame->val_stack
        // .... but IfStatement contains not Expr. that is, there is no guarantee that stack will be changed
        // .... need checks on stack size? ensure it to remaing the same at the end of evaluation of If statement?
//    }

    template<typename Td>
    void accept(const ReturnImpl<Td> &returnSt) {
        if constexpr (std::is_void_v<i_t<Td>>) {
            cur_builder->CreateRetVoid();
        } else {
            returnSt.returnee.toIR(*this);
            cur_builder->CreateRet(frame->val_stack.top());
            frame->val_stack.pop();
        }
    }

    template<typename TdLhs, typename TdRhs>
    void accept(const EAssign<TdLhs, TdRhs> &e) {
        e.lhs.toIR(*this);
        // TODO: but accept(Var) leaves on the stack Loaded value, not ptr to dest!!
//        llvm::Value* dest = frame->val_stack.top();
        frame->val_stack.pop();
        llvm::Value* dest = frame->evaluated.at(&e.lhs);

        e.rhs.toIR(*this);
        llvm::Value* rhs = frame->val_stack.top();
        // assignment returns result of the evaluation of rhs; so don't pop
        // frame->val_stack.pop();

        cur_builder->CreateStore(rhs, dest, e.lhs.isVolatile());
    }

    template<typename TdCallable, typename... ArgExprs>
    void accept(const ECall<TdCallable, ArgExprs...> &e) {
        // If dsl func is not defined (i.e. not translated)
        if (defined_funcs.find(&e.callee) == std::end(defined_funcs)) {
            e.callee.toIR(*this);
        }

        llvm::CallInst* inst = cur_builder->CreateCall(
                defined_funcs.at(&e.callee),
                eval_func_args(e.args)
        );
        frame->val_stack.push(inst);
    }

    template<typename AddrT, typename RetT, typename... ArgExprs>
    void accept(const ECallLoaded<AddrT, RetT, ArgExprs...> &e) {
        using call_t = ECallLoaded<AddrT, RetT, ArgExprs...>;
        llvm::CallInst* inst = cur_builder->CreateCall(
                get_func_type<typename call_t::ret_t, i_t<ArgExprs>...>(),
                get_llvm_const(e.callee.callAddr),
                eval_func_args(e.args)
        );
        frame->val_stack.push(inst);
    }

private:
    // todo: test: empty args; empty init list
    template<typename ...ArgExprs>
//    llvm::ArrayRef<llvm::Value*> eval_func_args(const ArgExprs&... args) {
    llvm::ArrayRef<llvm::Value*> eval_func_args(std::tuple<const ArgExprs&...> args) {

        // Evaluate arguments in left-to-right order
        auto evaluated_args = std::apply([this](const ArgExprs&... args){
            return llvm::ArrayRef({ (args.toIR(*this), frame->val_stack.top())... });
        }, args);

        // Clear stack from evaluated arguments
        for (auto i = 0; i < sizeof...(ArgExprs); ++i) { frame->val_stack.pop(); }

        return evaluated_args;
    }

public: // Operations and Casts
    template<typename To, typename TdFrom>
    void accept(const ECast<To, TdFrom> &e) {
        e.src.toIR(*this);
        llvm::Value* src = frame->val_stack.top();
        frame->val_stack.pop();

        llvm::Value* casted = cur_builder->CreateCast(e.op, src, get_type<To>());
        frame->val_stack.push(casted);
    }

    template<BinOps bOp, typename TdLhs, typename TdRhs = TdLhs>
    void accept(const EBinOp<bOp, TdLhs, TdRhs> &e) {
        e.lhs.toIR(*this);
        llvm::Value* lhs = frame->val_stack.top();
        frame->val_stack.pop();

        e.rhs.toIR(*this);
        llvm::Value* rhs = frame->val_stack.top();
        frame->val_stack.pop();

        llvm::Value* res = cur_builder->CreateBinOp(bOp, lhs, rhs);
        frame->val_stack.push(res);
    }

    // todo: possibly specialise this template for logical ops to evaluate lazily
//    template<BinOps bOp>
//    void accept(const EBinOpLogical<bOp> &e) {
//    }
};


// debuggin version
template<typename T>
void toIRImpl(const T &irTranslatable, IRTranslator &irt) {
    std::cout << "accept " << typeid(irTranslatable).name() << std::endl;

    // Checking some invariants
//    const auto n_vals = irt.frame->val_stack.size();
//    const auto n_funs = irt.fun_stack.size();

    irt.accept(irTranslatable);

    // hard-testing...
//    assert(irt.frame->val_stack.size() == n_vals && "frame->val_stack probably isn't used correctly!");
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

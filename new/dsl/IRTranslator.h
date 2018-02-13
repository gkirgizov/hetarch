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
#include "array.h"
#include "fun.h"

#include "../utils.h"
#include "types_map.h"


namespace hetarch {
namespace dsl {


class IRTranslator {
    template<typename T>
    friend void toIRImpl(const T &irTranslatable, IRTranslator &irTranslator);


    enum class ValSpec {
        Value,
        AddrNonVol,
        AddrVol
    };

    struct State {};

    struct Frame {
        llvm::Function* fun;
        std::stack<llvm::Value*> val_stack{};
        std::unordered_map<const ValueBase*, llvm::Value*> allocated{};

        std::stack<ValSpec> valspec_stack{};

        explicit Frame(llvm::Function* fun) : fun{fun} {}
    };

    std::unordered_map<const CallableBase*, llvm::Function*> defined_funcs{};
    std::stack<Frame> frame_stack{};
    Frame* frame{nullptr}; // current frame

    void push_addr(llvm::Value* val, bool is_volatile = false) {
        frame->val_stack.push(val);
        frame->valspec_stack.push(is_volatile ? ValSpec::AddrVol : ValSpec::AddrNonVol);
    }
    void push_val(llvm::Value* val) {
        frame->val_stack.push(val);
        frame->valspec_stack.push(ValSpec::Value);
    }
    void push_x(llvm::Value* x, ValSpec sp) {
        frame->val_stack.push(x);
        frame->valspec_stack.push(sp);
    }

    std::pair<llvm::Value*, bool> pop_addr() {
        ValSpec sp = frame->valspec_stack.top();
        frame->valspec_stack.pop();
        assert((sp != ValSpec::Value) && "trying to pop usual value as address!");

        llvm::Value* addr = frame->val_stack.top();
        frame->val_stack.pop();
        bool is_volatile = sp == ValSpec::AddrVol;
        return {addr, is_volatile};
    }

    llvm::Value* pop_val() {
        ValSpec sp = frame->valspec_stack.top();
        frame->valspec_stack.pop();
        if (sp != ValSpec::Value) {
            llvm::Value* addr = frame->val_stack.top();
            frame->val_stack.pop();
            return cur_builder->CreateLoad(addr, sp == ValSpec::AddrVol);
        }
        llvm::Value* val = frame->val_stack.top();
        frame->val_stack.pop();
        return val;
    }


    llvm::LLVMContext context{}; // todo: how to store/constrcut context?? is it completely internal thing?
    std::unique_ptr<llvm::Module> cur_module{};
    std::unique_ptr<llvm::IRBuilder<>> cur_builder{};


    const utils::map_t type_map;

    // LLVM make no distinction between signed and unsigned (https://stackoverflow.com/a/14723945)
    template<typename T, typename = typename std::enable_if_t<std::is_arithmetic_v<T>>>
    inline auto get_type() { return utils::at_key<std::make_signed_t<T>>(type_map); }

    // multidimensional arrays should be handled
//    template<typename Array, typename = typename std::enable_if_t<std::is_array_v<Array>>>
//    inline llvm::ArrayType* get_type() {
//        std::size_t N = std::extent_v<Array>;
//        using X = std::remove_extent_t<Array>;
//        return llvm::ArrayType::get(get_type<X>(), N);
//    };

    template<typename Array, typename = typename std::enable_if_t< is_std_array_v<Array>> >
//            std::is_same_v<Array, std::array<typename Array::value_type, std::tuple_size<Array>::value>> >>
    inline llvm::ArrayType* get_type() {
        return llvm::ArrayType::get(get_type<typename Array::value_type>(), std::tuple_size<Array>::value);
    };

    template<typename T, std::size_t N>
    inline llvm::ArrayType* get_type() { return llvm::ArrayType::get(get_type<T>(), N); };

    // todo: arrays , structs as arguments?
    template<typename RetT, typename... Args>
    inline llvm::FunctionType* get_func_type(bool isVarArg = false) {
        return llvm::FunctionType::get(
                get_type<RetT>(), { get_type<Args>()... }, isVarArg
        );
    }


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

    template<typename T, std::size_t N, typename = typename std::enable_if_t<std::is_arithmetic_v<T>>>
//    auto get_llvm_const(const T (&arr)[N]) {
    auto get_llvm_const(const std::array<T, N> &arr) {
        // todo ensure it works when vals are out of scope after return
        std::array<llvm::Constant*, N> vals;
        for (auto i = 0; i < N; ++i) {
            vals[i] = get_llvm_const(arr[i]);
        }
        return llvm::ConstantArray::get(get_type<std::array<T, N>>(), llvm::ArrayRef<llvm::Constant*>{vals});
    };

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
    // TODO: allocate everything only in the entry block of a func
    template<typename T, bool is_const, bool is_volatile
//            , typename = typename std::enable_if_t<std::is_arithmetic_v<T>>
    >
    void accept(const Var<T, is_const, is_volatile> &var) {
        llvm::Value* val_addr = nullptr;

        auto known = frame->allocated.find(&var);
        if (known == std::end(frame->allocated)) {

            llvm::Value* array_size = nullptr;
            if constexpr (is_std_array_v<T>) { array_size = get_llvm_const(std::tuple_size<T>::value); }
//            if constexpr (std::is_array_v<T>) { array_size = get_llvm_const(std::extent_v<T>); }
            // Default empty name is handled automatically by LLVM (when var.name().data() == "")
            val_addr = cur_builder->CreateAlloca(get_type<T>(), array_size, var.name().data());

            // When var is used for the first time (this branch) its actual value is its initial value
            auto init_val = get_llvm_const(var.initial_val());
            auto initInst = cur_builder->CreateStore(init_val, val_addr, is_volatile);
            frame->allocated[&var] = val_addr;
        } else { // Variable usage
            val_addr = known->second;
        }

        push_addr(val_addr, is_volatile);
    }

    // Actually, this specialisation is optional, because non-volatile const case can be handled above
    template<typename T>
    void accept(const Const<T, false> &var) {
        // Non-volatile const variables can be directly substitued by its value
        push_val(get_llvm_const(var.initial_val()));
    }

    // todo: try EArrayAccess and understand.
    //  todo: Ptr, Deref, ?TakeAddr
    //  todo: think how Struct

/*    template<typename T, std::size_t N, bool is_const, bool is_volatile>
    void accept(const Array<T, N, is_const, is_volatile>& a) {
        llvm::Value* val_addr = nullptr;

        auto known = frame->allocated.find(&a);
        if (known == std::end(frame->allocated)) {
            // Default empty name is handled automatically by LLVM (when var.name().data() == "")
            val_addr = cur_builder->CreateAlloca(get_type<T[N]>(), get_llvm_const(N), a.name().data());

            // When var is used for the first time (this branch) its actual value is its initial value
            auto init_val = get_llvm_const(a.initial_val());
            auto initInst = cur_builder->CreateStore(init_val, val_addr, is_volatile);
            frame->allocated[&a] = val_addr;
        } else { // Variable usage
            val_addr = known->second;
        }

        push_addr(val_addr, is_volatile); // todo: how the addr of the whole array can be volatile?
    };*/

    template<typename TdInd, typename T, std::size_t N, bool is_const, bool is_volatile>
    void accept(const EArrayAccess<TdInd, T, N, is_const, is_volatile>& e) {
        e.arr.toIR(*this);
        auto arr_addr = pop_addr().first;

        // todo: GEP-concerned: do i need extra 0 index at beginning? to access array by ptr...
        //      hardly. i have not the ptr, but addr (i.e. ref)

        // todo: specialise for constexpr values
//        llvm::Constant* ind = get_llvm_const(e.ind);
//        auto elt_addr = cur_builder->CreateInBoundsGEP(arr_addr, {ind});

        e.ind.toIR(*this);
        llvm::Value* ind = pop_val();
        auto elt_addr = cur_builder->CreateGEP(arr_addr, {ind});

        push_addr(elt_addr, is_volatile);
    };


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
        pop_val();
        // And returns result of evaluation of right expression
        seq.e2.toIR(*this);
    }

    template<typename TdCond, typename TdThen, typename TdElse>
    void accept(const IfExpr<TdCond, TdThen, TdElse> &e) {
        // Evaluate condition
        e.cond_expr.toIR(*this);
        llvm::Value* cond = pop_val();

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
        cur_builder->CreateStore(pop_val(), result);

        cur_builder->SetInsertPoint(false_block);
        e.else_expr.toIR(*this);
        cur_builder->CreateStore(pop_val(), result);

        cur_builder->restoreIP(ip);
        push_val(result);
    }

//    void accept(const IfElse &e) {
        // todo: seems, difference between IfExpr and If statement is that with the latter resulted value popped from the frame->val_stack
        // .... but IfStatement contains not Expr. that is, there is no guarantee that stack will be changed
        // .... need checks on stack size? ensure it to remaing the same at the end of evaluation of If statement?
//    }

    template<typename Td>
    void accept(const ReturnImpl<Td> &returnSt) {
        if constexpr (std::is_void_v<i_t<Td>>) {
            cur_builder->CreateRetVoid();
        } else {
            returnSt.returnee.toIR(*this);
            cur_builder->CreateRet(pop_val());
        }
    }

    template<typename TdLhs, typename TdRhs>
    void accept(const EAssign<TdLhs, TdRhs> &e) {
        e.lhs.toIR(*this);
        std::pair<llvm::Value*, bool> dest = pop_addr();

        e.rhs.toIR(*this);
        llvm::Value* rhs = pop_val();

        cur_builder->CreateStore(rhs, dest.first, dest.second);
        // assignment returns result of the evaluation of rhs
        push_val(rhs);
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
        push_val(inst);
    }

    template<typename AddrT, typename RetT, typename... ArgExprs>
    void accept(const ECallLoaded<AddrT, RetT, ArgExprs...> &e) {
        using call_t = ECallLoaded<AddrT, RetT, ArgExprs...>;
        llvm::CallInst* inst = cur_builder->CreateCall(
                get_func_type<typename call_t::ret_t, i_t<ArgExprs>...>(),
                get_llvm_const(e.callee.callAddr),
                eval_func_args(e.args)
        );
        push_val(inst);
    }

private:
    // todo: test: empty args; empty init list
    template<typename ...ArgExprs>
//    llvm::ArrayRef<llvm::Value*> eval_func_args(const ArgExprs&... args) {
    llvm::ArrayRef<llvm::Value*> eval_func_args(std::tuple<const ArgExprs&...> args) {

        // Evaluate arguments in left-to-right order
        auto evaluated_args = std::apply([this](const ArgExprs&... args){
            return llvm::ArrayRef({ (args.toIR(*this), pop_val())... });
        }, args);

        // Clear stack from evaluated arguments
//        for (auto i = 0; i < sizeof...(ArgExprs); ++i) { frame->val_stack.pop(); }

        return evaluated_args;
    }

public: // Operations and Casts
    template<typename To, typename TdFrom>
    void accept(const ECast<To, TdFrom> &e) {
        e.src.toIR(*this);
        llvm::Value* src = pop_val();

        llvm::Value* casted = cur_builder->CreateCast(e.op, src, get_type<To>());
        push_val(casted);
    }

    template<BinOps bOp, typename TdLhs, typename TdRhs = TdLhs>
    void accept(const EBinOp<bOp, TdLhs, TdRhs> &e) {
        e.lhs.toIR(*this);
        llvm::Value* lhs = pop_val();

        e.rhs.toIR(*this);
        llvm::Value* rhs = pop_val();

        llvm::Value* res = cur_builder->CreateBinOp(bOp, lhs, rhs);
        push_val(res);
    }

    // todo: possibly specialise this template for logical ops to evaluate lazily
//    template<BinOps bOp>
//    void accept(const EBinOpLogical<bOp> &e) {
//    }
};


// debuggin version
template<typename T>
void toIRImpl(const T &irTranslatable, IRTranslator &irt) {
//    std::cout << "accept " << typeid(irTranslatable).name() << std::endl;
    std::cout << "accept: " << utils::type_name<decltype(irTranslatable)>() << std::endl;

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

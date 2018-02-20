#pragma once


#include <iostream>
#include <string>
#include <string_view>
#include <tuple>
#include <stack>
#include <unordered_map>
#include <memory>
#include <utility>

#include <llvm/IR/Module.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/GlobalVariable.h>

#include "./types_map.h"
#include "../utils.h"

#include "../supportingClasses.h"
#include "dsl_base.h"
#include "sequence.h"
#include "if_else.h"
#include "loops.h"
#include "op.h"
#include "var.h"
#include "ptr.h"
#include "array.h"
#include "fun.h"
#include "ResidentObjCode.h"


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
        assert(addr->getType()->isPointerTy() && "llvm:Type of this llvm::Value is not a llvm::PointerType!");

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

    utils::LLVMMapper llvm_map;

public:
    explicit IRTranslator()
            : context{}, cur_builder{new llvm::IRBuilder<>{context}}, llvm_map{context} {}
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
            if constexpr (is_std_array_v<T>) { array_size = llvm_map.get_const(std::tuple_size<T>::value); }
            // Default empty name is handled automatically by LLVM (when var.name().data() == "")
            val_addr = cur_builder->CreateAlloca(llvm_map.get_type<T>(), array_size, var.name().data());

            // When var is used for the first time (this branch) its actual value is its initial value
            auto init_val = llvm_map.get_const(var.initial_val());
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
        push_val(llvm_map.get_const(var.initial_val()));
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
            val_addr = cur_builder->CreateAlloca(llvm_map.get_type<T[N]>(), llvm_map.get_const(N), a.name().data());

            // When var is used for the first time (this branch) its actual value is its initial value
            auto init_val = llvm_map.get_const(a.initial_val());
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

        // todo: maybe specialise for constexpr values
//        llvm::Constant* ind = llvm_map.get_const(e.ind);

        e.ind.toIR(*this);
        llvm::Value* ind = pop_val();
        auto zero = llvm_map.get_const(0); // need extra 0 to access elements of arrays
        auto elt_addr = cur_builder->CreateInBoundsGEP(arr_addr, {zero, ind});

        push_addr(elt_addr, is_volatile);
    };

    template<typename Td, bool is_const>
    void accept(const Ptr<Td, is_const>& ptr) {
        ptr.pointee.toIR(*this);

        /*
        auto addr = pop_addr();
        llvm::Value* pointee_addr = addr.first;
        bool is_volatile = addr.second; // todo: do I need it for anything?

        auto ptr_type = pointee_addr->getType()->getPointerTo();
        auto ptr_var = cur_builder->CreateAlloca(ptr_type, nullptr, "ptr");
        cur_builder->CreateStore(pointee_addr, ptr_var, false);

        push_addr(ptr_var, false);
         */

        /*
         * ? if <anythin (array, global)> is accessed through ptr then I need to do zero-GEP first
         */
    };

    template<typename Td>
    void accept(const ETakeAddr<Td>& e) {
        // it is enough to just do that -- then on the stack should be addr
        e.pointee.toIR(*this);

    }

    template<typename Td>
    void accept(const EDeref<Td>& e) {
        // it is memory access, isn't it? so, it is DANGEROUS. emit warning when dereferencing unknown Ptr and not ETakeAddr
        e.x.toIR(*this);

        // ...> i can apply Deref only to PtrBase
    }


public: // dsl function and related constructs (function is 'entry' point of ir generation)
    template<typename Td>
    auto translate(const DSLGlobal<Td>& g) {
        const auto g_name = std::string(g.x.name());
        const auto moduleName = "module_for_global_" + g_name;
        auto g_module = new llvm::Module(moduleName, context);

        llvm::GlobalVariable g_var{
                *g_module,
                llvm_map.get_type<f_t<Td>>(),
                Td::const_q,
                llvm::GlobalValue::LinkageTypes::ExternalLinkage,
                g.x.initialised() ? llvm_map.get_const(g.x.initial_val()) : nullptr,
                g_name
        };

//        return IIRModule{g_module};
        return IRModule<Td>{g_module, g_name};
    }

    template<typename TdRet, typename TdBody, typename... TdArgs>
    auto translate(const DSLFunction<TdRet, TdBody, TdArgs...> &fun) {
//        std::cout << "Translating DSLFunction '" << fun.name() << "'" << std::endl;

        const auto moduleName = "module_" + std::string(fun.name());
        cur_module = std::make_unique<llvm::Module>(moduleName, context);
        fun.toIR(*this);

        llvm::Module* m = cur_module.release();
        clear();
//        using fun_t = DSLFunction<TdRet, TdBody, TdArgs...>;
//        using RetT = typename fun_t::ret_t;
//        using Args = typename fun_t::args_full_t;
//        return IRModule<RetT, f_t<TdArgs>...>{m, fun.name().data()};
        return IRModule<TdRet, TdArgs...>{m, fun.name().data()};

    }

    template<typename AddrT, typename TdRet, typename ...TdArgs>
    void accept(const ResidentObjCode<AddrT, TdRet, TdArgs...> &func) {
        // check that func is really loaded?
    }

    template<typename TdRet, typename TdBody, typename... TdArgs>
    void accept(const DSLFunction<TdRet, TdBody, TdArgs...> &dslFun) {
        std::string funcName{dslFun.name()};

        // Define function
        // ExternalLinkage is to be able to call the func outside the module it resides in
        using ret_t = f_t<TdRet>;
        using args_t = std::tuple<f_t<TdArgs>...>;
        llvm::Function *fun = llvm::Function::Create(
            llvm_map.get_func_type<ret_t, args_t>(), llvm::Function::ExternalLinkage, funcName, cur_module.get()
//            llvm_map.get_func_type<f_t<TdRet>, f_t<TdArgs>...>(), llvm::Function::ExternalLinkage, funcName, cur_module.get()
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
    template<typename TdRet, typename TdBody, typename... TdArgs
            , typename Inds = std::index_sequence_for<TdArgs...>>
    void modifyArgs(const DSLFunction<TdRet, TdBody, TdArgs...> &dslFun, llvm::Function *fun) {
        std::array<llvm::Argument*, sizeof...(TdArgs)> args;
        for(llvm::Argument* arg_ptr = fun->arg_begin(); arg_ptr != fun->arg_end(); ++arg_ptr) {
            args[arg_ptr->getArgNo()] = arg_ptr;
        }
        modifyArgsImpl(dslFun, args, Inds{});
    }

    template<typename TdRet, typename TdBody, typename ...TdArgs
            , std::size_t ...I>
    void modifyArgsImpl(const DSLFunction<TdRet, TdBody, TdArgs...> &dslFun,
                        std::array<llvm::Argument*, sizeof...(TdArgs)> &args,
                        std::index_sequence<I...>) {
        auto dummy = std::make_tuple( 0, ( modifyArg(std::get<I>(dslFun.args), *args[I]) , 0)... );
    }

    template<typename TdArg>
    void modifyArg(const TdArg& param, llvm::Argument &arg) {
        arg.setName(param.name().data());
        if constexpr (is_byref_v<TdArg>) {
            // todo: get sizeof in target platform
            arg.addAttr(llvm::Attribute::getWithDereferenceableBytes(context, sizeof(i_t<TdArg>)));
        }
    }

public:
    template<typename Td1, typename Td2>
    void accept(const SeqExpr<Td1, Td2> &seq) {
        seq.e1.toIR(*this);
        // Sequence discards result of evaluation of left expression
        pop_val();
        // And returns result of evaluation of right expression
        seq.e2.toIR(*this);
    }

    template<typename TdCond, typename TdThen, typename TdElse>
    void accept(const IfExpr<TdCond, TdThen, TdElse> &e) {
        llvm::BasicBlock* true_block = llvm::BasicBlock::Create(context, "true_block", frame->fun);
        llvm::BasicBlock* false_block = llvm::BasicBlock::Create(context, "false_block", frame->fun);
        llvm::BasicBlock* after_block = llvm::BasicBlock::Create(context, "", frame->fun);

        // Create temp value to hold result of the whole expr depending on branch
        using T = typename IfExpr<TdCond, TdThen, TdElse>::type;
        auto result_var = cur_builder->CreateAlloca(llvm_map.get_type<T>());

        // Evaluate condition
        e.cond_expr.toIR(*this);
        llvm::Value* cond = pop_val();
        llvm::BranchInst* inst = cur_builder->CreateCondBr(cond, true_block, false_block);

        cur_builder->SetInsertPoint(true_block);
        e.then_expr.toIR(*this);
        cur_builder->CreateStore(pop_val(), result_var);
        cur_builder->CreateBr(after_block);

        cur_builder->SetInsertPoint(false_block);
        e.else_expr.toIR(*this);
        cur_builder->CreateStore(pop_val(), result_var);
        cur_builder->CreateBr(after_block);

        cur_builder->SetInsertPoint(after_block);
        llvm::Value* result = cur_builder->CreateLoad(result_var);
        push_val(result);
    }

    template<typename TdCond, typename TdBody>
    void accept(const WhileExpr<TdCond, TdBody> &e) {
        llvm::BasicBlock* cond_block = llvm::BasicBlock::Create(context, "cond_block", frame->fun);
        llvm::BasicBlock* loop_block = llvm::BasicBlock::Create(context, "loop_block", frame->fun);
        llvm::BasicBlock* after_block = llvm::BasicBlock::Create(context, "", frame->fun);

        cur_builder->CreateBr(cond_block);
        cur_builder->SetInsertPoint(cond_block);
        e.cond_expr.toIR(*this);
        llvm::Value* cond = pop_val();
        llvm::BranchInst* inst = cur_builder->CreateCondBr(cond, loop_block, after_block);

        cur_builder->SetInsertPoint(loop_block);
        e.body_expr.toIR(*this);
        llvm::Value* res = pop_val();
        cur_builder->CreateBr(cond_block);

        cur_builder->SetInsertPoint(after_block);
        push_val(res);
    }

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
                eval_func_args<TdCallable>(e.args)
        );
        push_val(inst);
    }


    template<typename TdCallable, typename... ArgExprs
            , typename = typename std::enable_if_t< is_dsl_loaded_callable_v<TdCallable> >>
    void accept(const ECall<TdCallable, ArgExprs...>& e) {
        using ret_t = typename TdCallable::ret_t;
        using args_t = typename TdCallable::args_t;

        llvm::CallInst* inst = cur_builder->CreateCall(
                llvm_map.get_func_type<ret_t, args_t>(),
                llvm_map.get_const(e.callee.callAddr),
                eval_func_args<TdCallable>(e.args)
        );
        push_val(inst);
    }

/*    template<typename AddrT, typename TdRet, typename ...ArgExprs>
    void accept(const ECallLoaded<AddrT, TdRet, ArgExprs...> &e) {
        llvm::CallInst* inst = cur_builder->CreateCall(
                llvm_map.get_func_type<f_t<TdRet>, f_t<TdArgs>...>(),
                llvm_map.get_const(e.callee.callAddr),
                eval_func_args(e.args)
        );
        push_val(inst);
    }*/

private:
    template<typename TdCallable, typename ArgExprsTuple
            , typename Inds = std::make_index_sequence<std::tuple_size_v<ArgExprsTuple>>>
    auto eval_func_args(const ArgExprsTuple& args) {
        return eval_func_args_impl<TdCallable>(args, Inds{});
    }
    template<typename TdCallable, typename ArgExprsTuple, std::size_t ...I>
    std::array<llvm::Value*, sizeof...(I)> eval_func_args_impl(
            const ArgExprsTuple& args,
            std::index_sequence<I...>) {
        return { eval_func_arg< std::tuple_element_t<I, typename TdCallable::dsl_args_t> >( std::get<I>(args) )... };
    }

    template<typename TdArg, typename ArgExpr>
    llvm::Value* eval_func_arg(const ArgExpr& e) {
        e.toIR(*this);
        if constexpr (is_byval_v<TdArg>) {
            return pop_val();
        } else if constexpr (is_val_v<ArgExpr>) {
            return pop_addr().first;
        } else { // ArgExpr is Expr
            // ArgExpr represents temporary value; allocate space for it
            llvm::Value* val = pop_val();
            llvm::AllocaInst* val_addr = cur_builder->CreateAlloca(val->getType());
            cur_builder->CreateStore(val, val_addr);
            return val_addr;
        }
    };

public: // Operations and Casts
    template<typename TdTo, typename TdFrom, Casts Op>
    void accept(const ECast<TdTo, TdFrom, Op> &e) {
        e.src.toIR(*this);

        using To = typename ECast<TdTo, TdFrom>::To;
        using From = typename ECast<TdTo, TdFrom>::From;
        // Cast only if necessary
        if constexpr (!std::is_same_v<To, From>) {
            llvm::Value* src = pop_val();
            llvm::Value* casted = cur_builder->CreateCast(Op, src, llvm_map.get_type<To>());
            push_val(casted);
        }
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

    template<OtherOps Op, Predicate P, typename TdLhs, typename TdRhs>
    void accept(const EBinCmp<Op, P, TdLhs, TdRhs>& e) {
        e.lhs.toIR(*this);
        llvm::Value* lhs = pop_val();
        e.rhs.toIR(*this);
        llvm::Value* rhs = pop_val();

        llvm::Value* res = nullptr;
        if constexpr (Op == OtherOps::FCmp) {
            res = cur_builder->CreateFCmp(P, lhs, rhs);
        } else if constexpr (Op == OtherOps::ICmp) {
            res = cur_builder->CreateICmp(P, lhs, rhs);
        } else {
            std::cout << "FCmp==" << OtherOps::FCmp << std::endl;
            std::cout << "ICmp==" << OtherOps::ICmp << std::endl;
            std::cout << "Op  ==" << Op << std::endl;
//            static_assert(false, "Unknown CmpInst kind!");
        }
        push_val(res);
    };
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



/// \tparam T subclass of DSLBase
//template<typename T, std::enable_if<std::is_base_of<DSLBase, T>::value>::type>
/*template<typename T>
struct IRTranslatable : virtual public DSLBase {
    void toIR(const IRTranslator &gen) const override {
        gen.accept(*static_cast<const T*>(this));
    }
};*/


}
}

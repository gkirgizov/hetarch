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
#include "llvm/IR/GlobalVariable.h"

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
//            : context{context}, cur_builder{new llvm::IRBuilder<>{context}}, llvm_map{context} {}

    void clear() {
        cur_module.release();

        defined_funcs.clear();
        assert(frame_stack.size() == 0);
    }

    llvm::LLVMContext& getContext() { return context; }

    // todo: push dummy Value to avoid accidental pop in empty stack?
    void accept(const VoidExpr &) {}

public: // Values
    template<typename TdVal
            , typename = typename std::enable_if_t<is_val_v<TdVal>>>
    void accept_value(const TdVal& val, llvm::Value* init_val = nullptr, bool is_volatile = false) {
        llvm::Value* val_addr = nullptr;

        auto known = frame->allocated.find(&val);
        if (known == std::end(frame->allocated)) {
            if constexpr (utils::is_debug)
                std::cerr << "--alloca for " << std::hex << &val << "; T=" << utils::type_name<decltype(val)>() << std::endl;

            auto ty = llvm_map.get_type<f_t<TdVal>>();
            val_addr = allocate(ty, init_val, is_volatile, val.name());

            frame->allocated[&val] = val_addr;
        } else { // Variable usage
            if constexpr (utils::is_debug)
                std::cerr << "--use of" << std::hex << &val << "; T=" << utils::type_name<decltype(val)>() << std::endl;

            val_addr = known->second;
        }

        push_addr(val_addr, is_volatile);
    }

    template<typename T, bool is_const, bool is_volatile>
    void accept(const Var<T, is_const, is_volatile> &var) {
        auto init_val = var.initialised() ? llvm_map.get_const(var.initial_val()) : nullptr;
        accept_value(var, init_val, is_volatile);
    }

    // Actually, this specialisation is optional, because non-volatile const case can be handled above
    template<typename T>
    void accept(const Const<T, false> &var) {
        // Non-volatile const variables can be directly substitued by its value
        push_val(llvm_map.get_const(var.initial_val()));
    }
    template<typename T>
    void accept(const DSLConst<T> &var) {
        push_val(llvm_map.get_const(var.val));
    }


    template<typename TdElem, std::size_t N, bool is_const = false>
    void accept(const Array<TdElem, N, is_const>& arr) {
        auto init_val = arr.initialised() ? llvm_map.get_const(arr.initial_val()) : nullptr;
        accept_value(arr, init_val);
    };

//    template<typename TdInd, typename TdElem, std::size_t N, bool is_const>
    template<typename TdArr, typename TdInd>
    void accept(const EArrayAccess<TdArr, TdInd>& e) {
        e.arr.toIR(*this);
        auto arr_addr = pop_addr().first;

        // todo: maybe specialise for constexpr indices
//        llvm::Constant* ind = llvm_map.get_const(e.ind);

        e.ind.toIR(*this);
        llvm::Value* ind = pop_val();
        auto zero = llvm_map.get_const(0); // need extra 0 to access elements of arrays
        auto elt_addr = cur_builder->CreateInBoundsGEP(arr_addr, {zero, ind});

        push_addr(elt_addr, TdArr::elt_volatile_q);
    };

    template<typename Td, bool is_const>
    void accept(const RawPtr<Td, is_const>& ptr) {
        llvm::Value* ptr_val = nullptr;
        if (ptr.initialised()) {
            using addr_t = HETARCH_TARGET_ADDRT;
            using ptr_t = f_t<RawPtr<Td, is_const>>;

//            auto ptr_val_raw = reinterpret_cast<ptr_t>(ptr.initial_val());
//            auto ptr_val = llvm_map.get_const(ptr_val_raw);
            auto addr_val_raw = static_cast<addr_t>(ptr.initial_val());
            auto addr_val = llvm_map.get_const(addr_val_raw);
            ptr_val = cur_builder->CreateIntToPtr(addr_val, llvm_map.get_type<ptr_t>());

            if constexpr (utils::is_debug) {
                std::cerr << "--init ptr of type='" << utils::type_name<ptr_t>()
                          << "' with addr 0x" << std::hex << ptr.initial_val() << std::dec << std::endl;
            }
        }
        bool is_volatile = false;
        accept_value(ptr, ptr_val, is_volatile);
    }

/*    template<typename Td, bool is_const>
    void accept(const Ptr<Td, is_const>& ptr) {
        ptr.pointee.toIR(*this);
        auto addr_pair = pop_addr();
//        bool is_volatile = addr_pair.second; // todo: do I need it for anything?
        accept_value(ptr, addr_pair.first, false);
    };*/

    template<typename Td>
    void accept(const ETakeAddr<Td>& e) {
        e.pointee.toIR(*this); // addr is on top of the stack
        push_val(pop_addr().first); // re-push addr as value
    }

    template<typename Td>
    void accept(const EDeref<Td>& e) {
        // it is memory access, isn't it? so, it is DANGEROUS. emit warning when dereferencing unknown Ptr and not ETakeAddr

        // avoid allocating ptr in e.ptr.toIR(*this) -- works only for const Ptr
//        e.ptr.pointee.toIR(*this);

        // another, more explicit way
        e.ptr.toIR(*this);
        auto pointee_addr = pop_val();
        push_addr(pointee_addr, e.ptr.elt_volatile_q);
    }

private:
//    template<typename T>
//    inline llvm::AllocaInst* allocate(const std::string_view& name = "") {
//        return allocate(llvm_map.get_type<T>(), name);
//    }

    inline llvm::AllocaInst* allocate(llvm::Type* ty, const std::string_view& name = "") {
        return allocate(ty, nullptr, false, name);
    }

    llvm::AllocaInst* allocate(llvm::Type* ty, llvm::Value* init_val = nullptr, bool is_volatile = false, const std::string_view& name = "") {
        // Allocate all local data in the entry block of func (as clang does)
        auto ip = cur_builder->saveIP();
        auto& entry_block = frame->fun->getEntryBlock();
        cur_builder->SetInsertPoint(&entry_block, --entry_block.end());

        auto allocated = cur_builder->CreateAlloca(ty, nullptr, name.data());
        if (init_val != nullptr) {
            cur_builder->CreateStore(init_val, allocated, is_volatile);
        }

        cur_builder->restoreIP(ip);
        return allocated;
    }

public: // dsl functions & residents

    // todo: remove
    template<typename Td>
    auto translate(const DSLGlobal<Td>& g) {
        const auto g_name = std::string(g.x.name());
        const auto moduleName = "module_for_global_" + g_name;
        auto g_module = new llvm::Module(moduleName, context);

//        llvm::GlobalVariable g_var{
        // todo: is it owned by Module?
        llvm::GlobalVariable* g_var = new llvm::GlobalVariable{
                *g_module,
                llvm_map.get_type<f_t<Td>>(),
                Td::const_q,
                llvm::GlobalValue::LinkageTypes::ExternalLinkage,
//                g.x.initialised() ? llvm_map.get_const(g.x.initial_val()) : nullptr,
                llvm_map.get_const(g.x.initial_val()), // init even if not g.x.initialised()
                g_name
        };

//        return IIRModule{g_module};
        return IRModule<Td>{g_module, g_name};
    }

    template<typename TdResident>
    void accept_resident(const TdResident& g, bool is_const = false, bool is_volatile = false) {
        using T = f_t<TdResident>;

//        if (TdResident::const_q) {
        if (is_const && !is_volatile) {
            // we can't change init_val for Const value after loading, can we? so, just return it.
            push_val(llvm_map.get_const(g.initial_val()));
            return;
        }

        llvm::Value* g_ptr = nullptr;
        auto known = frame->allocated.find(&g);
        if (known == std::end(frame->allocated)) {
            llvm::Type* element_type = llvm_map.get_type<T>();
            llvm::PointerType* ptr_type = element_type->getPointerTo();

            auto g_addr = llvm_map.get_const(g.addr);
            g_ptr = cur_builder->CreateIntToPtr(g_addr, ptr_type, g.name().data());
            frame->allocated[&g] = g_ptr;
        } else {
            g_ptr = known->second;
        }

//        push_addr(g_ptr, TdResident::volatile_q);
        push_addr(g_ptr, is_volatile);
    };

    template<typename AddrT, typename T, bool is_const, bool is_volatile>
    inline void accept(const ResidentVar<AddrT, T, is_const, is_volatile>& g) {
        accept_resident(g, is_const, is_volatile);
    };

    template<typename AddrT, typename TdElem, std::size_t N, bool is_const>
    inline void accept(const ResidentArray<AddrT, TdElem, N, is_const>& g) {
        accept_resident(g, is_const);
    };

    template<typename AddrT, typename Td, bool is_const>
    inline void accept(const ResidentPtr<AddrT, Td, is_const>& g) {
        accept_resident(g, is_const);
    };

    template<typename TdBody, typename... TdArgs>
    auto translate(const DSLFunction<TdBody, TdArgs...> &fun) {
        const auto moduleName = "module_" + std::string(fun.name());
        cur_module = std::make_unique<llvm::Module>(moduleName, context);
        fun.toIR(*this);

        llvm::Module* m = cur_module.release();
        clear();
        using fun_t = DSLFunction<TdBody, TdArgs...>;
        using TdRet = typename fun_t::dsl_ret_t;
        return IRModule<TdRet, TdArgs...>{m, fun.name().data()};
    }

    template<typename AddrT, typename TdRet, typename ...TdArgs>
    void accept(const ResidentObjCode<AddrT, TdRet, TdArgs...> &func) {
        // check that func is really loaded?
    }

    template<typename TdBody, typename... TdArgs>
    void accept(const DSLFunction<TdBody, TdArgs...> &dslFun) {
        std::string funcName{dslFun.name()};

        // Define function
        // ExternalLinkage is to be able to call the func outside the module it resides in
        using fun_t = DSLFunction<TdBody, TdArgs...>;
        using ret_t = typename fun_t::ret_t;
        using args_t = typename fun_t::args_t;
        llvm::Function *fun = llvm::Function::Create(
            llvm_map.get_func_type<ret_t, args_t>(), llvm::Function::ExternalLinkage, funcName, cur_module.get()
//            llvm_map.get_func_type<ret_t, f_t<TdArgs>...>(), llvm::Function::ExternalLinkage, funcName, cur_module.get()
        );

        const auto entry_bb_name = funcName + "_block" + "_entry";
        llvm::BasicBlock *entry_bb = llvm::BasicBlock::Create(context, entry_bb_name, fun);
        const auto main_bb_name = funcName + "_block" + "_main";
        llvm::BasicBlock* main_bb = llvm::BasicBlock::Create(context, main_bb_name, fun);

        // Save ip in case it's another func translation while previous func translation hasn't finished
        llvm::IRBuilderBase::InsertPoint ip = cur_builder->saveIP();
        frame_stack.push(Frame{fun});
        frame = &frame_stack.top();

        cur_builder->SetInsertPoint(entry_bb);
        cur_builder->CreateBr(main_bb);
        cur_builder->SetInsertPoint(main_bb);
        handle_args(dslFun, fun);
        dslFun.body.toIR(*this);

        frame_stack.pop();
        frame = &frame_stack.top();
        cur_builder->restoreIP(ip);
        bool not_repeated_translation = defined_funcs.insert({&dslFun, fun}).second;
        // todo: fail before translation, not after (it's meaningless)
        // Recursive functions ain't handled here
        assert(not_repeated_translation && (funcName + ": attempt to translate dsl function twice!").c_str());
    }

private:
    template<typename TdBody, typename... TdArgs
            , typename Inds = std::index_sequence_for<TdArgs...>>
    void handle_args(const DSLFunction<TdBody, TdArgs...> &dslFun, llvm::Function *fun) {
        // Build mapping from llvm::Argument to dslFun.args using indices
        std::array<llvm::Argument*, sizeof...(TdArgs)> args;
        for(llvm::Argument* arg_ptr = fun->arg_begin(); arg_ptr != fun->arg_end(); ++arg_ptr) {
            args[arg_ptr->getArgNo()] = arg_ptr;
        }
        handle_args_impl(dslFun, args, Inds{});
    }

    template<typename TdBody, typename ...TdArgs, std::size_t ...I>
    void handle_args_impl(const DSLFunction<TdBody, TdArgs...> &dslFun,
                        std::array<llvm::Argument*, sizeof...(TdArgs)> &args,
                        std::index_sequence<I...>) {
        // Call handle_arg for each pair of llvm::Argument and arg from dslFun.args
        auto dummy = std::make_tuple( 0, ( handle_arg(std::get<I>(dslFun.args), args[I]) , 0)... );
    }

    template<typename TdArg>
    void handle_arg(const TdArg& dsl_arg, llvm::Argument *arg) {
//        const std::string name{dsl_arg.name()};
        // todo: failing at assert(NameRef.find_first_of(0) == StringRef::npos && "Null bytes are not allowed in names");
//        arg->setName(name);

        if constexpr (is_byref_v<TdArg>) {
            // todo: get sizeof in target platform
            arg->addAttr(llvm::Attribute::getWithDereferenceableBytes(context, sizeof(i_t<TdArg>)));
        }
        // Now allocate space on stack for argument and store actual Value arg represents (as Clang does)
        auto arg_addr = allocate(arg->getType(), dsl_arg.name());
        cur_builder->CreateStore(arg, arg_addr, dsl_arg.volatile_q);
        frame->allocated[static_cast<const ValueBase*>(&dsl_arg)] = arg_addr;
    }

public:
    template<typename Td1, typename Td2>
    void accept(const SeqExpr<Td1, Td2> &seq) {
        seq.e1.toIR(*this);
        // Sequence discards result of evaluation of left expression
        frame->val_stack.pop();
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
        auto result_var = allocate(llvm_map.get_type<T>(), "if_result");

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
        returnSt.returnee.toIR(*this);
        if constexpr (std::is_void_v<i_t<Td>>) {
            cur_builder->CreateRetVoid();
        } else {
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
//        push_val(rhs);
        // assignment returns reference to assignee
        push_addr(dest.first, dest.second);
    }

    template<typename TdCallable, typename... ArgExprs>
    void accept(const ECall<TdCallable, ArgExprs...> &e
            , std::enable_if_t< !is_dsl_loaded_callable_v<TdCallable>, std::false_type> = std::false_type{}) {
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

    // TODO: these things with using ... types (e..g in eval_func_args) ain't a good sign.

    template<typename TdCallable, typename... ArgExprs>
    void accept(const ECall<TdCallable, ArgExprs...>& e
        , std::enable_if_t< is_dsl_loaded_callable_v<TdCallable>, std::true_type> = std::true_type{}) {
        using fun_t = std::remove_reference_t<TdCallable>;
        using ret_t = typename fun_t::ret_t;
        using args_t = typename fun_t::args_t;

        llvm::FunctionType* fty = llvm_map.get_func_type<ret_t, args_t>();
        auto addr_value = llvm_map.get_const(e.callee.callAddr);
        auto fun_ptr = cur_builder->CreateIntToPtr(addr_value, fty->getPointerTo(), "fun_ptr");

        llvm::CallInst* inst = cur_builder->CreateCall(
                fty,
                fun_ptr,
                eval_func_args<fun_t>(e.args)
        );
        push_val(inst);
    }

    template<typename TdRet, typename ...TdArgs>
    void accept(const Recurse<TdRet, TdArgs...>& e) {
        std::cerr << "not implemented." << std::endl;

/*        using fun_t = decltype(frame->dsl_fun); // todo: no way to get full DSLFunction type now
        static_assert(std::is_invocable_v<fun_t, TdArgs...>,
                      "Arguments type mismatch between DSLFunction and Recurse!");
        static_assert(std::is_same_v< f_t<std::invoke_result_t<fun_t, TdArgs...>>, f_t<TdRet> >,
                      "Return type mismatch between DSLFunction and Recurse!");

        llvm::CallInst* inst = cur_builder->CreateCall(
                frame->fun,
                eval_func_args<fun_t>(e.args)
        );
        push_val(inst);*/
    };


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
        return { eval_func_arg
                         < std::tuple_element_t<I, typename std::remove_reference_t<TdCallable>::dsl_args_t> >
                         ( std::get<I>(args) )...
        };
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
            llvm::AllocaInst* val_addr = allocate(val->getType(), val);
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

    template<BinOps bOp, typename TdPtr, typename Td>
    void accept(const EBinPtrOp<bOp, TdPtr, Td>& e) {
        using target_size_t = HETARCH_TARGET_ADDRT;
        auto ty = llvm_map.get_type<target_size_t>();

        e.operand.toIR(*this);
        auto n = pop_val();
        e.ptr.toIR(*this);
        auto addr = pop_val();

        auto pointee_size = llvm_map.get_const<target_size_t>(sizeof(typename std::remove_reference_t<TdPtr>::element_t));
        auto n_casted = cur_builder->CreateSExt(n, ty);
        auto val = cur_builder->CreateMul(n_casted, pointee_size);

        auto addr_val = cur_builder->CreatePtrToInt(addr, ty);
        auto new_addr_val = cur_builder->CreateBinOp(bOp, addr_val, val);
        auto new_addr = cur_builder->CreateIntToPtr(new_addr_val, addr->getType());

        push_val(new_addr); // it is manipulation with Values of PointerType, not with pointers (i.e. memory)
//        push_addr(new_addr, false);
    };

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
            std::cerr << "FCmp==" << OtherOps::FCmp << std::endl;
            std::cerr << "ICmp==" << OtherOps::ICmp << std::endl;
            std::cerr << "Op  ==" << Op << std::endl;
//            static_assert(false, "Unknown CmpInst kind!");
        }
        push_val(res);
    };
};


// debuggin version
template<typename T>
void toIRImpl(const T &irTranslatable, IRTranslator &irt) {
//    std::cout << "accept " << typeid(irTranslatable).name() << std::endl;
    std::cerr << "accept: " << utils::type_name<decltype(irTranslatable)>() << std::endl;

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

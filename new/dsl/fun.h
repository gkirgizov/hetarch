#pragma once

#include <string_view>

#include "dsl_base.h"
#include "var.h"
#include "../utils.h"


namespace hetarch {
namespace dsl {


template<typename, typename, typename...> class DSLFunction;


template<typename RetT, typename... Args>
struct ECallEmptyBase : public Expr<RetT> {};

template<typename TdCallable, typename... ArgExprs>
struct ECall : public Expr<typename TdCallable::ret_t> {
    static_assert((... && (std::is_base_of_v<ExprBase, remove_cvref_t<ArgExprs>> ||
                           std::is_base_of_v<ValueBase, remove_cvref_t<ArgExprs>>)
    ));
//    static_assert((... && std::is_base_of_v<Expr<i_t<ArgExprs>>, ArgExprs>));
    static_assert(std::is_same_v< typename TdCallable::args_t, std::tuple<i_t<ArgExprs>...> >);

    using args_t = typename TdCallable::args_t;
    using ret_t = typename TdCallable::ret_t;

    const TdCallable& callee;
    std::tuple<const ArgExprs...> args;

    explicit constexpr ECall(const TdCallable& callee, ArgExprs&&... args)
            : callee{callee}, args{std::forward<ArgExprs>(args)...} {}

    inline void toIR(IRTranslator &irTranslator) const override { toIRImpl(*this, irTranslator); }
};


//template<typename RetT, typename ...Args> class DSLCallableBase : public DSLBase {
//    using ret_t = RetT;
//    using args_t = std::tuple<Args...>;
//};
struct CallableBase : public DSLBase {};

template<typename TdCallable>
//struct DSLCallable : public DSLCallableBase<typename TdCallable::ret_t, typename TdCallable::args_t> {
//    using ret_t = typename TdCallable::ret_t;
//    using args_t = typename TdCallable::args_t;
struct DSLCallable : public CallableBase {

    template<typename... ArgExprs, typename TdCallable_ = TdCallable
            , typename = typename std::enable_if_t< std::is_same_v<typename TdCallable_::args_t, std::tuple<i_t<ArgExprs>...>> >
    >
    inline constexpr auto call(ArgExprs&&... args) const {
        return ECall<TdCallable, ArgExprs...>{static_cast<const TdCallable&>(*this), std::forward<ArgExprs>(args)...};
    }
    template<typename... ArgExprs>
    inline constexpr auto operator()(ArgExprs&&... args) const { return this->call(std::forward<ArgExprs>(args)...); }
};


template<typename Td>
struct ReturnImpl : public ESBase {
    using type = i_t<Td>;

    const Td returnee;

    explicit constexpr ReturnImpl(Td&& r) : returnee{std::forward<Td>(r)} {}

    inline void toIR(IRTranslator &irTranslator) const override { toIRImpl(*this, irTranslator); }
};

template<typename Td>
constexpr auto Return(Td&& r) { return ReturnImpl<Td>{std::forward<Td>(r)}; }

constexpr auto Return() { return ReturnImpl<const VoidExpr&>{empty_expr}; }

//template<> struct ReturnImpl<void> : public ESBase {
//    using type = void;
//
//    inline void toIR(IRTranslator &irTranslator) const override { toIRImpl(*this, irTranslator); }
//};
//constexpr auto Return() { return ReturnImpl<void>{}; }


//template<typename T, bool isConst, bool is_volatile>
//using FunArg = Var<T, isConst, is_volatile>;
template<typename T> using FunArg = Var<T, false, false>;
template<typename... Args> using FunArgs = std::tuple<const FunArg<Args>&...>;

template<typename... Args>
struct FunArgs2 {
    std::tuple<const FunArg<Args>&...> impl;

    explicit constexpr FunArgs2(const FunArg<Args>&... args) : impl{args...} {
        static_assert((... && checkDefaultArg(args.initialised())), "arg without default value!");
    }

private:
    bool checkDefaultArg(bool defaulted) {
        default_arg_present |= defaulted;
        return default_arg_present && !defaulted;
    }
    bool default_arg_present{false};
};


// todo: handle default arguments
template<typename TdRet, typename TdBody, typename... Args>
struct DSLFunction : public DSLCallable<DSLFunction<TdRet, TdBody, Args...>>, public Named {
    using ret_t = i_t<TdRet>;
    using args_t = std::tuple<Args...>;

    const FunArgs<Args...> args;
    const TdBody body;
    const ReturnImpl<TdRet> returnSt;

    // No arguments constructors
    explicit constexpr DSLFunction(ReturnImpl<TdRet>&& returnSt)
            : args{}, body{empty_expr}, returnSt{std::move(returnSt)} {}
    // Empty body constructors
    explicit constexpr DSLFunction(FunArgs<Args...> args, ReturnImpl<TdRet>&& returnSt)
            : args{args}, body{empty_expr}, returnSt{std::move(returnSt)} {}
    // Ordinary (full) constructors
    constexpr DSLFunction(FunArgs<Args...> args, TdBody&& body, ReturnImpl<TdRet>&& returnSt)
            : args{args}, body{std::move(body)}, returnSt{std::move(returnSt)} {}

    // suppose there's another func which uses __the same__ types
//    constexpr void specialise(Args... args);

    inline void toIR(IRTranslator &irTranslator) const override { toIRImpl(*this, irTranslator); }
};

// Explicit deduction guides for some ctors
template<typename TdRet>
DSLFunction(ReturnImpl<TdRet>&& returnSt) -> DSLFunction<TdRet, const VoidExpr>;
template<typename TdRet, typename... Args>
DSLFunction(FunArgs<Args...> args, ReturnImpl<TdRet>&& returnSt) -> DSLFunction<TdRet, const VoidExpr, Args...>;




}
}

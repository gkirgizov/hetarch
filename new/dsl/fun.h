#pragma once

#include <string_view>


#include "dsl_base.h"
#include "var.h"
#include "ptr.h"
#include "dsl_type_traits.h"
#include "../utils.h"


namespace hetarch {
namespace dsl {


template<typename, typename...> class DSLFunction;


template<typename Td, typename = typename std::enable_if_t<is_val_v<Td>>>
struct byRef : public ValueBase {
    using type = f_t<Td>;
    static const bool volatile_q = Td::volatile_q;
    static const bool const_q = Td::const_q;

    const Td& arg;
    explicit constexpr byRef(const Td& arg) : arg{arg} {}
};


// Some type traits
template<typename>
struct is_dsl_ptr : public std::false_type {};
template<typename Td, bool is_const>
struct is_dsl_ptr<Ptr<Td, is_const>> : public std::true_type {};
template<typename Td>
inline constexpr bool is_dsl_ptr_v = is_dsl_ptr<Td>::value;

template<typename>
struct is_byref: public std::false_type {};
template<typename Td>
struct is_byref<byRef<Td>> : public std::true_type {};
template<typename Td>
inline constexpr bool is_byref_v = is_byref<Td>::value;

template<typename Td> struct is_byval : public std::conditional_t<
        is_dsl_ptr_v<Td> || is_byref_v<Td>,
        std::false_type,
        std::true_type
> {};
template<typename Td>
inline constexpr bool is_byval_v = is_byval<Td>::value;


// todo: align with DSL Value type system
template<typename TdExpr>
struct param_for_arg {
    using T = f_t<TdExpr>;
//    using type = Var<T>;
    using type = std::conditional_t< std::is_void_v<T>, VoidExpr, Var<T>>;
};
template<typename TdExpr>
using param_for_arg_t = typename param_for_arg<TdExpr>::type;


template<typename TdCallable, typename... ArgExprs>
struct ECall : public Expr<typename std::remove_reference_t<TdCallable>::ret_t> {
    const TdCallable callee;
    const std::tuple<ArgExprs...> args;

//    explicit constexpr ECall(const TdCallable& callee, ArgExprs&&... args)
//            : callee{callee}, args{std::forward<ArgExprs>(args)...} {}
    explicit constexpr ECall(TdCallable&& callee, ArgExprs&&... args)
            : callee{std::forward<TdCallable>(callee)}, args{std::forward<ArgExprs>(args)...} {}

    inline void toIR(IRTranslator &irTranslator) const override { toIRImpl(*this, irTranslator); }
};

template<typename TdCallable, typename... ArgExprs>
constexpr auto makeCall(TdCallable&& callable, ArgExprs&&... args) {
    return ECall<TdCallable, ArgExprs...>{std::forward<TdCallable>(callable), std::forward<ArgExprs>(args)...};
};


template<typename TdCallable, typename TdRet, typename ...TdArgs>
struct DSLCallable : public CallableBase {
    using dsl_ret_t = TdRet;
    using dsl_args_t = std::tuple<TdArgs...>;
    using ret_t = f_t<TdRet>;
    using args_t = std::tuple<f_t<TdArgs>...>;

    template<typename ...ArgExprs>
    constexpr static bool validateArgs() {
//        return ( ... && validateArg< TdArgs, ArgExprs, std::index_sequence_for<TdArgs...>{} >() );
        return ( ... && validateArg<TdArgs, ArgExprs>() );
    }

    template<typename TdArg, typename ArgExpr, std::size_t arg_i = 0>
    constexpr static bool validateArg() {
        static_assert(std::is_same_v<i_t<TdArg>, i_t<ArgExpr>>,
                      "Type mismatch in arguments");
        // Makes sense to check cv-qualifiers when passing NOT by value
        // Only Value-like dsl types have cv-qualifiers
        if constexpr (is_val_v<ArgExpr> && !is_byval_v<TdArg>) {
            static_assert(!(ArgExpr::const_q && !TdArg::const_q),
                          "Passing const value as non-const (discards qualifier)!");
            static_assert(!(ArgExpr::volatile_q && !TdArg::volatile_q),
                          "Passing volatile value as non-const (discards qualifier)!");
            // separate rule for non-const ptr to const
            static_assert(!(is_dsl_ptr_v<TdArg> && ArgExpr::const_pointee_q && !TdArg::const_pointee_q),
                          "Passing ptr-to-const as ptr-to-non-const (discards qualifier)!");
        }
        return true;
    }

    template<typename... ArgExprs
            , typename = typename std::enable_if_t<
//                    is_ev_v<ArgExprs...> &&
                    validateArgs<ArgExprs...>()
            >
    >
    inline constexpr auto call(ArgExprs&&... args) const {
//        return ECall<TdCallable, ArgExprs...>(
        return makeCall(
                static_cast<const TdCallable&>(*this),
                std::forward<ArgExprs>(args)...
        );
    }
    template<typename... ArgExprs>
    inline constexpr auto operator()(ArgExprs&&... args) const { return this->call(std::forward<ArgExprs>(args)...); }

};


template<typename Td>
struct ReturnImpl : public Expr<f_t<Td>> {
    const Td returnee;
    explicit constexpr ReturnImpl(Td&& r) : returnee{std::forward<Td>(r)} {}

    inline void toIR(IRTranslator &irTranslator) const override { toIRImpl(*this, irTranslator); }
};

template<typename Td>
constexpr auto Return(Td&& r) { return ReturnImpl<Td>{std::forward<Td>(r)}; }
constexpr auto Return() { return ReturnImpl<const VoidExpr&>{empty_expr}; }


//template<typename T, bool isConst, bool is_volatile>
//using FunArg = Var<T, isConst, is_volatile>;
template<typename T> using FunArg = Var<T, false, false>;
//template<typename... Args> using FunArgs = std::tuple<const FunArg<Args>&...>;

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


//template<typename ...Tds> using FunArgs = FunArgs3<Tds...>;
//template<typename ...Tds> using FunArgs = std::tuple<const Tds&...>;

template<typename ...Tds>
auto MakeFunArgs(const Tds&... args) {
    static_assert(is_val_v<Tds...>, "Expected subclasses of ValueBase! (invalid args in function declaration)");
    return std::tuple<const Tds&...>{args...};
//    return FunArgs<Tds...>{args...};
}


template<typename TdBody, typename... TdArgs>
struct DSLFunction : public Named, public DSLCallable<DSLFunction<TdBody, TdArgs...>, param_for_arg_t<TdBody>, TdArgs...> {
    static_assert((is_val_v<TdArgs> && ...), "Formal parameters must be Values (derived from ValueBase)!");

//    using funargs_t = FunArgs<TdArgs...>;
    using funargs_t = std::tuple<TdArgs...>;

    funargs_t args;
//    const funargs_t args;
    const TdBody body;

    constexpr DSLFunction(std::string_view name, TdBody&& body)
            : Named{name}, args{}, body{std::move(body)} {}
    constexpr DSLFunction(std::string_view name, funargs_t&& args, TdBody&& body)
            : Named{name}, args{std::forward<funargs_t>(args)}, body{std::move(body)} {}

    template<typename BodyGenerator
            , typename = typename std::enable_if_t<
                    std::is_invocable_v<BodyGenerator, TdArgs...>
            >
    >
    constexpr DSLFunction(std::string_view name, funargs_t&& args, BodyGenerator&& body_builder)
//            : Named{name}, args{std::forward<funargs_t>(args)}, body{std::apply(body_builder, this->args)}
            : Named{name}, args{std::forward<funargs_t>(args)}, body{Return(std::apply(body_builder, this->args))}
    {
        // makes sense when generators are pure: always return their body as expression; don't use side-effects
        static_assert(!std::is_void_v<TdBody>, "DSL Generator must generate (return) some DSL code!");
        //static_assert(is_expr_v<TdBody>, "DSL Generator must generate DSL code, not something else!");
        static_assert(is_dsl_v<TdBody>, "DSL Generator must generate DSL code, not something else!");
    }

//    constexpr void specialise(Args... args);

    inline void toIR(IRTranslator &irTranslator) const override { toIRImpl(*this, irTranslator); }
};

// Explicit deduction guide
template<typename BodyGenerator, typename ...TdArgs>
DSLFunction(std::string_view, std::tuple<TdArgs...>&&, BodyGenerator&&) ->
//DSLFunction<std::invoke_result_t<BodyGenerator, TdArgs...>, TdArgs...>;
DSLFunction<ReturnImpl< std::invoke_result_t<BodyGenerator, TdArgs...> >, TdArgs...>;

}
}

#pragma once

#include <string_view>

#include "dsl_base.h"
#include "var.h"
#include "ptr.h"
#include "dsl_type_traits.h"
#include "../utils.h"


namespace hetarch {
namespace dsl {


template<typename, typename, typename...> class DSLFunction;


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


template<typename TdCallable, typename... ArgExprs>
struct ECall : public Expr<typename TdCallable::ret_t> {
    const TdCallable& callee;
    std::tuple<const ArgExprs...> args;

    explicit constexpr ECall(const TdCallable& callee, ArgExprs&&... args)
            : callee{callee}, args{std::forward<ArgExprs>(args)...} {}

    inline void toIR(IRTranslator &irTranslator) const override { toIRImpl(*this, irTranslator); }
};


template<typename TdCallable, typename TdRet, typename ...TdArgs>
struct DSLCallable : public CallableBase {
    using ret_t = f_t<TdRet>;
    using args_t = std::tuple<f_t<TdArgs>...>;
    using dsl_ret_t = TdRet;
    using dsl_args_t = std::tuple<TdArgs...>;

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
        return ECall<TdCallable, ArgExprs...>{static_cast<const TdCallable&>(*this), std::forward<ArgExprs>(args)...};
    }
    template<typename... ArgExprs>
    inline constexpr auto operator()(ArgExprs&&... args) const { return this->call(std::forward<ArgExprs>(args)...); }

};


template<typename Td>
struct ReturnImpl : public ESBase {
    using type = f_t<Td>;

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



//template<typename ...Tds>
//struct FunArgs3 {
//    std::tuple<const Tds&...> args;
//    explicit constexpr FunArgs3(const Tds&... args) : args{args...} {}
//};

//template<typename ...Tds> using FunArgs = FunArgs3<Tds...>;
template<typename ...Tds> using FunArgs = std::tuple<const Tds&...>;

template<typename ...Tds>
auto MakeFunArgs(const Tds&... args) {
    static_assert(is_val_v<Tds...>, "Expected subclasses of ValueBase! (invalid args in function declaration)");
    return std::tuple<const Tds&...>{args...};
//    return FunArgs<Tds...>{args...};
}


template<typename TdRet, typename TdBody, typename... TdArgs>
struct DSLFunction : public DSLCallable<DSLFunction<TdRet, TdBody, TdArgs...>, TdRet, TdArgs...>, public Named {
//    using funargs_t = FunArgs<TdArgs...>;
    using funargs_t = std::tuple<const TdArgs&...>;

    const funargs_t args;
    const TdBody body;
    const ReturnImpl<TdRet> returnSt;

    // No arguments constructors
    explicit constexpr DSLFunction(const char* name, ReturnImpl<TdRet>&& returnSt)
            : Named{name}, args{}, body{empty_expr}, returnSt{std::move(returnSt)} {}
    // Empty body constructors
    explicit constexpr DSLFunction(const char* name, funargs_t args, ReturnImpl<TdRet>&& returnSt)
            : Named{name}, args{args}, body{empty_expr}, returnSt{std::move(returnSt)} {}
    // Ordinary (full) constructors
    constexpr DSLFunction(const char* name, funargs_t args, TdBody&& body, ReturnImpl<TdRet>&& returnSt)
            : Named{name}, args{args}, body{std::move(body)}, returnSt{std::move(returnSt)} {}

//    constexpr void specialise(Args... args);

    inline void toIR(IRTranslator &irTranslator) const override { toIRImpl(*this, irTranslator); }
};

// Explicit deduction guides for some ctors
template<typename TdRet>
DSLFunction(const char*, ReturnImpl<TdRet>&&) -> DSLFunction<TdRet, const VoidExpr>;
template<typename TdRet, typename... TdArgs>
DSLFunction(const char*, FunArgs<TdArgs...>, ReturnImpl<TdRet>&&) -> DSLFunction<TdRet, const VoidExpr, TdArgs...>;




}
}

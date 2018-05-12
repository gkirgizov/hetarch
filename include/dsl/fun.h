#pragma once

#include <string_view>

#include "dsl_base.h"
#include "dsl_type_traits.h"
#include "to_dsl_trait.h"
#include "utils.h"


namespace hetarch {
namespace dsl {

using dsl::f_t; // by some reasons CLion can't resolve it automatically.


template<typename, typename...> class Function;

template<typename TdCallable, typename... ArgExprs>
struct ECall : Expr<typename std::remove_reference_t<TdCallable>::ret_t> {
    using fun_t = std::remove_reference_t<TdCallable>;

//    const TdCallable& callee;
    TdCallable callee;
    const std::tuple<ArgExprs...> args;

//    explicit constexpr ECall(const TdCallable& callee, ArgExprs... args)
    explicit constexpr ECall(TdCallable&& callee, ArgExprs... args)
//            : callee{callee}
            : callee{std::forward<TdCallable>(callee)}
            , args{args...}
    {}

    IR_TRANSLATABLE
};

// todo: Almost anyway need to explicitly specify return type TdRet
//  otherwise only some kind of return type deduction magic depending on exit condition (if there is some...)
template<typename TdRet, typename ...TdArgs>
struct Recurse : Expr<f_t<TdRet>> {
    const std::tuple<TdArgs...> args;
    explicit constexpr Recurse(TdArgs... args) : args{args...} {}
    IR_TRANSLATABLE
};


template<typename TdCallable, typename TdRet, typename ...TdArgs>
struct Callable : CallableBase {
    static_assert(is_val_v<TdRet, TdArgs...>);

    using dsl_ret_t = TdRet;
    using dsl_args_t = std::tuple<TdArgs...>;
    using ret_t = f_t<TdRet>;
    using args_t = std::tuple<f_t<TdArgs>...>;

    template<typename ...Args>
    constexpr static bool validateArgs() {
//        using fun_t = void( f_t<TdArgs>... );
//        return std::is_invocable_t< fun_t, f_t<ArgExprs> >;
        // Fail at first inappropriate argument to provide more information
        return ( ... && validateArg<f_t<TdArgs>, Args>() );
    }

    template<typename Param, typename Arg>
    constexpr static bool validateArg() {
        using fun_t = void(Param);
        static_assert(std::is_invocable_v< fun_t, Arg >, "Passing argument of invalid type!");
        return true;
    }

    template< typename... ArgExprs >
    inline constexpr auto call(ArgExprs... args) const {
        static_assert(is_ev_v<ArgExprs...>, "Expected DSL types for DSL function call!");
        static_assert(validateArgs<f_t<ArgExprs>...>(), "Invalid argument types for function call!");

        return ECall<const TdCallable&, ArgExprs...>{
                static_cast<const TdCallable&>(*this),
                args...
        };
    }
    template<typename... ArgExprs>
    inline constexpr auto operator()(ArgExprs... args) const { return this->call(args...); }

};


namespace {

template<typename Td>
struct ReturnImpl : Expr<f_t<Td>> {
    const Td returnee;
    explicit constexpr ReturnImpl(Td r) : returnee{r} {}
    IR_TRANSLATABLE
};

template<typename Td>
constexpr auto Return(Td r) { return ReturnImpl<Td>{r}; }
constexpr auto Return() { return ReturnImpl<const VoidExpr&>{Unit}; }

}



template<typename ...Tds>
auto MakeFunArgs(const Tds&... args) {
    static_assert(is_val_v<Tds...>, "Expected subclasses of ValueBase! (invalid args in function declaration)");
    return std::tuple<const Tds&...>{args...};
}


template<typename TdBody, typename... TdArgs>
struct Function : Named
                , Callable<Function<TdBody, TdArgs...>, param_for_arg_t<TdBody>, TdArgs...>
{
    static_assert((is_val_v<TdArgs> && ...), "Formal parameters must be Values (derived from ValueBase)!");

//    using funargs_t = FunArgs<TdArgs...>;
    using funargs_t = std::tuple<TdArgs...>;

    funargs_t args;
//    const funargs_t args;
    const ReturnImpl<TdBody> body;

    constexpr Function(std::string_view name, TdBody body)
            : Named{name}, args{}, body{body} {}
    constexpr Function(std::string_view name, funargs_t args, TdBody body)
            : Named{name}, args{args}, body{body} {}

    template<typename BodyGenerator
            , typename = typename std::enable_if_t<
                    std::is_invocable_v<BodyGenerator, TdArgs...>
            >
    >
    constexpr Function(std::string_view name, funargs_t args, BodyGenerator&& body_builder)
            : Named{name}
            , args{args}
            , body(std::apply(std::forward<BodyGenerator>(body_builder), this->args))
    {
        // makes sense when generators are pure: always return their body as expression; don't use side-effects
        static_assert(!std::is_void_v<TdBody>, "DSL Generator must generate (return) some DSL code!");
        static_assert(is_dsl_v<TdBody>, "DSL Generator must generate DSL code, not something else!");
    }

    IR_TRANSLATABLE
};

template<typename BodyGenerator, typename ...TdArgs>
struct build_dsl_fun_type {
    using type = Function< std::invoke_result_t<BodyGenerator, TdArgs...>, TdArgs...>;
};
template<typename BodyGenerator, typename ...ArgExprs>
struct build_dsl_fun_type_from_args {
    using type = typename build_dsl_fun_type<BodyGenerator, param_for_arg_t<ArgExprs>...>::type;
};

// Explicit deduction guide
template<typename BodyGenerator, typename ...TdArgs>
Function(std::string_view, std::tuple<TdArgs...>, BodyGenerator&&) ->
//typename build_dsl_fun_type<BodyGenerator, TdArgs...>::type;
Function< std::invoke_result_t<BodyGenerator, TdArgs...>, TdArgs...>;

}
}

#pragma once


#include <tuple>
//#include <

#include "dsl_type_traits.h"
#include "fun.h"
#include "../utils.h"


namespace hetarch {
namespace dsl {


using dsl::f_t; // by some reasons CLion can't resolve it automatically.


// For generic types, directly use the result of the signature of its 'operator()'
template <typename T>
struct function_traits
    : public function_traits<decltype(&T::operator())>
{};

// we specialize for pointers to member function
template <typename ClassType, typename ReturnType, typename... Args>
struct function_traits<ReturnType(ClassType::*)(Args...) const>
{
    enum { arity = sizeof...(Args) };

    using result_t = ReturnType;
    using args_t = std::tuple<Args...>;

    template <std::size_t i>
    struct arg {
        using type = std::tuple_element_t<i, std::tuple<Args...>>;
    };
    template <std::size_t i>
    using arg_t = typename arg<i>::type;
};


//template<typename Tuple, typename Inds = std::make_index_sequence<std::tuple_size_v<Tuple>>>
//struct unpack {};
//template<std::size_t I, typename Tuple>
//struct unpack_impl {
//    using type = std::tuple_element_t<I, Tuple>;
//};


template<typename GLambda>
auto make_dsl_callable_test(GLambda&& dsl_generator) {
    std::cerr << utils::type_name<GLambda>() << std::endl;
    static_assert(std::is_invocable_v<GLambda, int, int>); // ok
    static_assert(std::is_invocable_v<GLambda, std::string, std::string>); // ok
    // can't be even called! failing when trying to instantiate generic lambda
//    static_assert(!std::is_invocable_v<GLambda, int, std::string>); // can be called; but ill-formed (no addition for int + std::string)

//    using ft = function_traits<decltype(dsl_generator)>;
//    static_assert(std::is_same_v<int, ft::result_t>);
//    std::cerr << utils::type_name< ft::result_type >() << std::endl;
//    std::cerr << utils::type_name< decltype(&GLambda::operator()) >() << std::endl;

    // X   i can check for arity
    // XX  likely i can check for parameter types;
    // XXX and if they are template/non-template parameters
}


// Overload with type deduction for ArgExprs
template<typename ...ArgExprs, typename GLambda>
constexpr auto make_dsl_fun_from_arg_types(GLambda&& dsl_gen, ArgExprs&&...){
//    return make_dsl_fun_from_arg_types<GLambda, ArgExprs...>(std::forward<GLambda>(dsl_gen));
    return make_dsl_fun_from_arg_types<ArgExprs...>(dsl_gen);
};

template<typename ...ArgExprs, typename GLambda>
constexpr auto make_dsl_fun_from_arg_types(const GLambda &dsl_gen) {
    constexpr auto fun_name = "generic_name";

    static_assert(std::is_invocable_v<GLambda, ArgExprs...>);

//    auto&& formal_params = std::tuple<param_for_arg_t<ArgExprs>...>{};
//    return DSLFunction{fun_name, std::move(formal_params), dsl_gen};
    return DSLFunction{make_bsv(fun_name), std::tuple<param_for_arg_t<ArgExprs>...>{}, dsl_gen};
};


template<typename GLambda>
auto make_generic_dsl_fun(GLambda &&dsl_generator) {
    // Return generic lambda which will return ECallGeneric when invoked
    return [&](auto&&... args){
        // what about by-ref, by-val?
        //   ???--> allow user use explicit <CallSomething>(ByVal(arg1), arg2) (ByVal is Copy)
        return makeCall(
                make_dsl_fun_from_arg_types<decltype(args)...>(dsl_generator),
                std::forward<decltype(args)>(args)...
        );
    };
}



}
}

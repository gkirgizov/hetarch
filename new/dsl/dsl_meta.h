#pragma once


#include <unordered_map>
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


template<typename ...ArgExprs, typename GLambda>
constexpr auto make_dsl_fun_from_arg_types(GLambda&& dsl_gen) {

    static_assert(std::is_invocable_v<GLambda, ArgExprs...>, "DSL Generator cannot be invoked with provided arguments!");
    constexpr auto fun_name = "generic_name";

    auto formal_params = std::tuple<param_for_arg_t<ArgExprs>...>{};
    return DSLFunction{make_bsv(fun_name), formal_params, std::forward<GLambda>(dsl_gen)};
//    return DSLFunction{make_bsv(fun_name), std::tuple<param_for_arg_t<ArgExprs>...>{}, std::forward<GLambda>(dsl_gen)};
};

// Overload with type deduction for ArgExprs
template<typename ...ArgExprs, typename GLambda>
constexpr auto make_dsl_fun_from_arg_types(GLambda&& dsl_gen, ArgExprs...){
    return make_dsl_fun_from_arg_types<ArgExprs...>(std::forward<GLambda>(dsl_gen));
};


namespace {

using callable_ptr = std::unique_ptr<CallableBase>;
static std::unordered_map<std::string_view, callable_ptr> dsl_fun_instantiated{};


template<typename ...ArgExprs, typename GLambda>
const auto& get_or_alloc_dsl_fun_from_arg_types(GLambda&& dsl_gen) {

    static_assert(std::is_invocable_v<GLambda, ArgExprs...>, "DSL Generator cannot be invoked with provided arguments!");
    constexpr auto fun_name = "generic_name";

//        using TdCallable = decltype(make_dsl_fun_from_arg_types<decltype(args)...>( std::declval<GLambda>() ));
    using TdCallable = typename build_dsl_fun_type_from_args<GLambda, ArgExprs...>::type;
    auto type_str = utils::type_name<TdCallable>(); // todo: make constexpr

    auto instantiated = dsl_fun_instantiated.find(type_str);
    if (instantiated == std::end(dsl_fun_instantiated)) {
        CallableBase* fun_ptr = new DSLFunction{
                        make_bsv(fun_name),
                        std::tuple<param_for_arg_t<ArgExprs>...>{},
                        std::forward<GLambda>(dsl_gen)
        };
        auto inserted = dsl_fun_instantiated.emplace(type_str, fun_ptr);
        assert(inserted.second && "Error when storing instantiated Generic DSLFunction!");
        return static_cast<const TdCallable &>(*fun_ptr);
    }
    return static_cast<const TdCallable &>(*instantiated->second);
};

}


template<typename GLambda>
auto make_generic_dsl_fun(GLambda &&dsl_generator) {
    // Return generic lambda which will return ECallGeneric when invoked
    return [&](auto&&... args){
        // what about by-ref, by-val?
        //   ???--> allow user use explicit <CallSomething>(ByVal(arg1), arg2) (ByVal is Copy)

        return makeCall(
                get_or_alloc_dsl_fun_from_arg_types<decltype(args)...>(dsl_generator),
                std::forward<decltype(args)>(args)...
        );
    };
}


}
}

#pragma once


#include <unordered_map>
#include <tuple>

#include "dsl_type_traits.h"
#include "fun.h"
#include "../utils.h"


namespace hetarch {
namespace dsl {


using dsl::f_t; // by some reasons CLion can't resolve it automatically.


template< typename ...ArgExprs, typename DSLGen >
auto make_dsl_fun(DSLGen &&dsl_gen, const std::string &name = "dsl_generic") {
    static_assert(is_ev_v<ArgExprs...>, "Expected DSL types as argument types!");
    static_assert(std::is_invocable_v<DSLGen, ArgExprs...>, "DSL Generator cannot be invoked with provided arguments!");

    using formal_params_t = std::tuple< param_for_arg_t<ArgExprs>... >;
    return Function{
            name,
            formal_params_t{},
            std::forward<DSLGen>(dsl_gen)
    };
};

// Overload with type deduction for ArgExprs
template<typename ...ArgExprs, typename DSLGen>
auto make_dsl_fun(DSLGen &&dsl_gen, ArgExprs...){
    return make_dsl_fun<ArgExprs...>(std::forward<DSLGen>(dsl_gen));
};


// DSL function template repository
namespace {

using fun_id = std::string_view;

template<typename TdCallable>
constexpr inline auto get_fun_id() {
    return utils::type_name<TdCallable>();
}

using callable_ptr = std::unique_ptr<CallableBase>;
static std::unordered_map<fun_id, callable_ptr> dsl_fun_instantiated{};

}

template<typename TdBody, typename... TdArgs>
inline auto free_repo(const Function<TdBody, TdArgs...> &f) {
    using TdCallable = Function<TdBody, TdArgs...>;
    constexpr auto fun_id = get_fun_id<TdCallable>();
    return dsl_fun_instantiated.erase(fun_id);
}

template<typename ...ArgExprs, typename DSLGen>
const auto& get_or_make_dsl_fun(DSLGen &&dsl_gen, const std::string &name = "dsl_generic") {

    static_assert(is_ev_v<ArgExprs...>, "Expected DSL types as argument types!");
    static_assert(std::is_invocable_v<DSLGen, ArgExprs...>, "DSL Generator cannot be invoked with provided arguments!");

//        using TdCallable = decltype(make_dsl_fun<decltype(args)...>( std::declval<DSLGen>() ));
    using TdCallable = typename build_dsl_fun_type_from_args<DSLGen, ArgExprs...>::type;
    const auto fun_id = get_fun_id<TdCallable>();

    auto instantiated = dsl_fun_instantiated.find(fun_id);
    if (instantiated == std::end(dsl_fun_instantiated)) {
        using formal_params_t = std::tuple< param_for_arg_t<ArgExprs>... >;
        CallableBase* fun_ptr = new Function{
                        name,
                        formal_params_t{},
                        std::forward<DSLGen>(dsl_gen)
        };
        auto inserted = dsl_fun_instantiated.emplace(fun_id, fun_ptr);
        assert(inserted.second && "Error when storing instantiated Generic Function!");
        return static_cast<const TdCallable &>(*fun_ptr);
    }
    return static_cast<const TdCallable &>(*instantiated->second);
};


template<typename DSLGen>
auto make_generic_dsl_fun(DSLGen &&dsl_gen, const std::string &name = "dsl_generic") {
    // Return generic lambda which returns ECall to Function instantiated in-place
    return [&](auto... args){
        const auto& fun_ref = get_or_make_dsl_fun<decltype(args)...>(dsl_gen, name);
        return ECall<decltype(fun_ref), decltype(args)...>{
                fun_ref,
                args...
        };

//        return ECall{
//                get_or_make_dsl_fun<decltype(args)...>(dsl_gen, name),
//                args...
//        };
    };
}


}
}

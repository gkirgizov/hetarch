#pragma once

#include "dsl_base.h"
#include "dsl_type_traits.h"
#include "to_dsl_trait.h"
#include "utils.h"
#include "var.h"
#include "ptr.h"
#include "fun.h"
#include "ResidentObjCode.h"


namespace hetarch {
namespace dsl {

using dsl::f_t; // by some reasons CLion can't resolve it automatically.


namespace {

using CharPtr = Ptr<Var<const char>>;

// signature of printf
template<typename AddrT, typename ...TdArgs>
struct LogFun : Callable< LogFun<AddrT, TdArgs...>
//                        , VoidExpr
                        , Var<int> // return
                        , CharPtr, TdArgs... // args
                        >
{
    const AddrT callAddr;
    explicit LogFun(AddrT callAddr) : callAddr{callAddr} {}
};

}

// Specialization of trait
template<typename AddrT, typename ...TdArgs>
struct is_resident< LogFun<AddrT, TdArgs...> > : std::true_type {};


template<typename AddrT>
auto make_log_fun(AddrT call_addr) {
    return [=](auto format, auto... args) {
        static_assert(is_ev_v<decltype(format), decltype(args)...>, "Expected DSL types as argument types!");
        using FormatT = param_for_arg_t<decltype(format)>;
        static_assert(std::is_same_v<char*, f_t<FormatT>>, "format string must represent a char*!");
        using LogFunT = LogFun< AddrT, param_for_arg_t<decltype(args)>...>;

//        return ECall<LogFunT, CharPtr, decltype(args)...>{
        return ECall{
                LogFunT{call_addr},
                format, args...
        };
    };
}


}
}

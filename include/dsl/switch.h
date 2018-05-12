#pragma once

#include "dsl_base.h"
#include "dsl_type_traits.h"
#include "sequence.h"


namespace hetarch {
namespace dsl {

namespace {

// todo: maybe better do my own type Case(cond, expr), but std::pair-like (to init from {cond, expr})???
template<typename TdCond, typename TdExpr>
using case_impl_t = std::pair<TdCond, TdExpr>;

}


// todo: what to do with "error: template parameter pack must be the last template parameter"?
template< typename TdDefault
        , typename ...TdCond
        , typename ...TdExpr
>
struct SwitchExpr : Expr<f_t<TdDefault>> {
    static_assert(is_same_raw_v<TdDefault, TdExpr...>);
    static_assert(std::is_same_v<bool, i_t<TdCond>...>, "Condition expressions must have bool type!");

    using cases_t = std::tuple< case_impl_t<TdCond, TdExpr>... >;
    cases_t cases;
    TdDefault def_case;

    explicit constexpr SwitchExpr(cases_t cases, TdDefault def_case)
            : cases{cases}
            , def_case{def_case}
    {}
};


template< typename TdDefault
        , typename ...TdCond
        , typename ...TdExpr
>
auto Switch(std::tuple< case_impl_t<TdCond, TdExpr>... > cases, TdDefault def_case) {
    return SwitchExpr<TdDefault, TdCond..., TdExpr...>{
            cases,
            def_case
    };
};


// todo: ensure it's okey in llvm to "ret; ret;"
// Switch statement
template< typename ...TdCond
        , typename ...TdExpr
>
auto Switch(std::tuple< case_impl_t<TdCond, TdExpr>... > cases) {
    auto repacked = std::apply([](const auto& ...cases){
        return std::tuple{ SeqExpr{cases.second, Unit}... };
    }, cases);
    return SwitchExpr<VoidExpr, TdCond..., TdExpr...>{
//            cases, // todo: check repack each case.second as SeqExpr{case.second, Unit}
            std::move(repacked),
            Unit
    };
};

}
}

#pragma once

#include "dsl_base.h"
#include "dsl_type_traits.h"
#include "sequence.h"


namespace hetarch {
namespace dsl {


using dsl::i_t; // by some reasons CLion can't resolve it automatically.
using dsl::f_t; // by some reasons CLion can't resolve it automatically.


template< typename TdCond, typename TdThen, typename TdElse
        , typename = typename std::enable_if_t< is_ev_v<TdCond, TdThen, TdElse> >
>
struct IfExpr : Expr<f_t<TdThen>> {
// todo: should both branches always have the same underlying ::type?
    static_assert(std::is_same_v<bool, i_t<TdCond>>, "Condition expression must have bool type!");
    static_assert(is_same_raw_v<TdThen, TdElse>, "Both branches of If expression must have the same type!");

    const TdCond cond_expr;
    const TdThen then_expr;
    const TdElse else_expr;

    constexpr IfExpr(TdCond cond_expr, TdThen then_expr, TdElse else_expr)
              : cond_expr{cond_expr}
              , then_expr{then_expr}
              , else_expr{else_expr} {}

    IR_TRANSLATABLE
};

template<typename TdCond, typename TdThen, typename TdElse>
constexpr auto If(TdCond cond_expr, TdThen then_expr, TdElse else_expr) {
        return IfExpr<TdCond, TdThen, TdElse>{
                cond_expr,
                then_expr,
                else_expr
        };
};


// If statement (expr with void type)
template<typename TdCond, typename TdThen>
constexpr auto If(TdCond cond_expr, TdThen then_expr) {
        return IfExpr<TdCond, TdThen, VoidExpr>{
                cond_expr,
                SeqExpr{then_expr, Unit},
                Unit
        };
};

}
}

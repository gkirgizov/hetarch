#pragma once

#include <type_traits>

#include "dsl_base.h"


namespace hetarch {
namespace dsl {


// 'if-else' statement
/*struct IfElse : public ESBase {
    const Expr<bool> &cond;
    const ESBase &then_body;
    const ESBase &else_body;

    constexpr IfElse(Expr<bool> &&cond, ESBase &&then_body)
            : cond{cond}, then_body{then_body}, else_body{empty_expr} {}
    constexpr IfElse(Expr<bool> &&cond, ESBase &&then_body, ESBase &&else_body)
            : cond{cond}, then_body{then_body}, else_body{else_body} {}

    inline void toIR(IRTranslator &irTranslator) const override { toIRImpl(*this, irTranslator); }
};*/


// todo: should both mranches always have the same underlying ::type?
template<typename TdCond, typename TdThen, typename TdElse
        , typename = typename std::enable_if_t<
                std::is_same_v<bool, i_t<TdCond>> && std::is_same_v<i_t<TdThen>, i_t<TdElse>>
        >
>
struct IfExpr : public Expr<i_t<TdThen>> {
    const TdCond cond_expr;
    const TdThen then_expr;
    const TdElse else_expr;

    constexpr IfExpr(TdCond&& cond_expr, TdThen&& then_expr, TdElse&& else_expr)
              : cond_expr{std::forward<TdCond>(cond_expr)}
              , then_expr{std::forward<TdThen>(then_expr)}
              , else_expr{std::forward<TdElse>(else_expr)} {}

    inline void toIR(IRTranslator &irTranslator) const override { toIRImpl(*this, irTranslator); }
};


}
}

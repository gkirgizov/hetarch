#pragma once

#include "IDSLCallable.h"

namespace hetarch {
namespace dsl {


class IRTranslator;
template<typename T> inline void toIRImpl(const T &irTranslatable, IRTranslator &irTranslator);


// 'if-else' statement
struct IfElse : public ESBase {
    const Expr<bool> &cond;
    const ESBase &then_body;
    const ESBase &else_body;

    constexpr IfElse(Expr<bool> &&cond, ESBase &&then_body)
            : cond{cond}, then_body{then_body}, else_body{empty_expr} {}
    constexpr IfElse(Expr<bool> &&cond, ESBase &&then_body, ESBase &&else_body)
            : cond{cond}, then_body{then_body}, else_body{else_body} {}

    inline void toIR(IRTranslator &irTranslator) const override { toIRImpl(*this, irTranslator); }
};

// 'if-else' expression or ternary operator
template<typename T>
struct IfExpr : public Expr<T> {
    const Expr<bool> &cond;
    const Expr<T> &then_body;
    const Expr<T> &else_body;

    constexpr IfExpr(Expr<bool> &&cond, Expr<T> &&then_body, Expr<T> &&else_body)
            : cond{cond}, then_body{then_body}, else_body{else_body} {}

    inline void toIR(IRTranslator &irTranslator) const override { toIRImpl(*this, irTranslator); }
};


}
}

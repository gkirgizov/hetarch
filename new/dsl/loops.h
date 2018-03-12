#pragma once


#include "dsl_base.h"
#include "dsl_type_traits.h"


namespace hetarch {
namespace dsl {

using dsl::i_t; // by some reasons CLion can't resolve it automatically.
using dsl::f_t; // by some reasons CLion can't resolve it automatically.


template<typename TdCond, typename TdBody
        , typename = typename std::enable_if_t<
                is_expr_v<TdCond, TdBody>
        >
>
struct WhileExpr: public Expr<f_t<TdBody>> {
    static_assert(std::is_same_v<bool, i_t<TdCond>>, "Loop cond_exprition must return boolean value!");

    const TdCond cond_expr;
    const TdBody body_expr;

    constexpr WhileExpr(TdCond cond_expr, TdBody body_expr)
            : cond_expr{cond_expr}
            , body_expr{body_expr}
    {}

    inline void toIR(IRTranslator &irTranslator) const { toIRImpl(*this, irTranslator); }
};


template<typename TdCond, typename TdBody>
constexpr auto While(TdCond cond_expr, TdBody body_expr) {
    return WhileExpr<TdCond, TdBody>{
            cond_expr,
            body_expr,
    };
};

}
}

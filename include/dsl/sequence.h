#pragma once

#include <utility>

#include "dsl_base.h"
#include "dsl_type_traits.h"


namespace hetarch {
namespace dsl {

/*
template<typename ...Ts> struct select_last;
template<typename T> struct select_last<T> { using type = T; };
template<typename T, typename ...Ts> struct select_last<T, Ts...> { using type = typename select_last<Ts...>::type; };
*/

using dsl::i_t; // by some reasons CLion can't resolve it automatically.
using dsl::f_t; // by some reasons CLion can't resolve it automatically.


template<typename Td1, typename Td2>
struct SeqExpr : Expr<f_t<Td2>> {
    const Td1 e1;
    const Td2 e2;
    constexpr SeqExpr(Td1 e1, Td2 e2) : e1{e1}, e2{e2} {}
    IR_TRANSLATABLE
};


template<typename Td1, typename Td2
        , typename = typename std::enable_if_t< is_ev_v<Td1, Td2> >
>
constexpr auto Seq(Td1 e1, Td2 e2) {
    return SeqExpr<Td1, Td2>{e1, e2};
}


// disallow mixing C++ constructs with DSL constructs using comma
template<typename Td1, typename Td2
        , typename = typename std::enable_if_t< is_ev_v<Td1, Td2> >
>
constexpr auto operator,(Td1 e1, Td2 e2) {
    return SeqExpr<Td1, Td2>{e1, e2};
};


}
}


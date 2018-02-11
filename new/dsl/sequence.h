#pragma once

#include <type_traits>
#include <utility>

#include "dsl_base.h"


namespace hetarch {
namespace dsl {

/*
template<typename ...Ts> struct select_last;
template<typename T> struct select_last<T> { using type = T; };
template<typename T, typename ...Ts> struct select_last<T, Ts...> { using type = typename select_last<Ts...>::type; };
*/

using dsl::i_t; // by some reasons CLion can't resolve it automatically.

template<typename Td1, typename Td2>
struct Seq : public Expr<i_t<Td2>> {
    const Td1 e1;
    const Td2 e2;

    constexpr Seq(Td1&& e1, Td2&& e2) : e1{std::forward<Td1>(e1)}, e2{std::forward<Td2>(e2)} {}

    inline void toIR(IRTranslator &irTranslator) const override { toIRImpl(*this, irTranslator); }
};

// disallow mixing C++ constructs with DSL constructs using comma
template<typename Td1, typename Td2
        , std::enable_if_t<
                std::is_base_of_v<ExprBase, Td1> && std::is_base_of_v<ExprBase, Td2>
        >
>
constexpr auto operator,(Td1&& e1, Td2&& e2) {
    return Seq<Td1, Td2>{std::forward<Td1>(e1), std::forward<Td2>(e2)};
};


}
}


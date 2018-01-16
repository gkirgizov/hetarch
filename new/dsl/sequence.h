#pragma once

//#include <type_traits>
#include <utility>

#include "IDSLCallable.h"


template<typename ...Ts> struct select_last;
template<typename T> struct select_last<T> { using type = T; };
template<typename T, typename ...Ts> struct select_last<T, Ts...> { using type = typename select_last<Ts...>::type; };

//template<typename T, typename ...Ts>
//class Seq1 : public Expr<T> {
//    std::tuple<Expr<Ts>..., Expr<T>> seq;
//public:
//    constexpr Seq1(Expr<&&e1, ExprBase &&e2, Expr<Ts>&&... es);
//};

template<typename T>
class Seq : public Expr<T>, public IRConvertible<Seq<T>> {
    const ExprBase lhs;
    const Expr<T> rhs;
public:
    template<typename T0>
    constexpr Seq(Expr<T0> &&lhs, Expr<T> &&rhs) : lhs{std::move(lhs)}, rhs{std::move(rhs)} {}

};

template<typename Tl, typename Tr>
constexpr Seq<Tr> operator,(Expr<Tl> &&lhs, Expr<Tr> &&rhs) {
    return Seq<Tr>{std::move(lhs), std::move(rhs)};
}

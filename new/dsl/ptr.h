#pragma once


#include "dsl_base.h"
#include "dsl_type_traits.h"
#include "var.h"


namespace hetarch {
namespace dsl {

using dsl::i_t; // by some reasons CLion can't resolve it automatically.


template<typename Td>
struct EDeref : public Expr<i_t<Td>> {
const Td& x;
explicit constexpr EDeref(const Td& x) : x{x} {}
};

template<typename Td, bool is_const, typename = typename std::enable_if_t<std::is_base_of_v<ValueBase, Td>>>
struct Ptr : public Value<Ptr<Td, is_const>, f_t<Td>*, is_const> {
    using type = f_t<Td>*;
    static const bool volatile_q = Td::volatile_q;
    static const bool const_q = is_const;
    static const bool const_pointee_q = Td::const_q;

    const Td& pointee;
    explicit constexpr Ptr(const Td& pointee) : pointee{pointee} {}
    // todo: is it 'taking addr of a temporary' Warning??
//    explicit constexpr Ptr(Td&& pointee) : pointee{std::forward<Td>(pointee)} {}

    constexpr auto operator*() const { return EDeref<Td>{pointee}; }
//    constexpr auto operator->() const {}
//    constexpr auto operator->*() const {}

};

template<typename Td, typename = typename std::enable_if_t<std::is_base_of_v<ValueBase, Td>>>
struct ETakeAddr: public Expr<i_t<Td>> {
using type = i_t<Td>*;
const Td& pointee;
explicit constexpr ETakeAddr(const Td& pointee) : pointee{pointee} {}
};


}
}

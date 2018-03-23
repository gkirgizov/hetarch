#pragma once


#include "dsl_base.h"
#include "dsl_type_traits.h"
#include "cast.h"


namespace hetarch {
namespace dsl {

using dsl::i_t; // by some reasons CLion can't resolve it automatically.
using dsl::f_t; // by some reasons CLion can't resolve it automatically.
using dsl::remove_cvref_t;


template<typename Tw, typename T, bool is_const> struct Value;

// todo: do implicit cast when not exact type match?
template< typename TdLhs
        , typename TdRhs
        , typename = typename std::enable_if_t< is_val_v<TdLhs> >
>
struct EAssign : get_base_t< TdLhs, EAssign<TdLhs, TdRhs> > {
    using type = f_t<TdLhs>;
    using this_t = EAssign<TdLhs, TdRhs>;
    using Value<this_t, type, TdLhs::const_q>::operator=;
    constexpr auto operator=(const this_t& rhs) const { return this->assign(rhs); };
    constexpr EAssign(this_t&&) = default;
    constexpr EAssign(const this_t&) = default;

    const TdLhs lhs;
    const TdRhs rhs;
    constexpr EAssign(TdLhs lhs, TdRhs rhs) : lhs{lhs}, rhs{rhs} {}
    IR_TRANSLATABLE
};


template< typename Td
        , typename = typename std::enable_if_t< is_val_v<Td> >
>
struct ETakeAddr: public Expr<f_t<Td>*> {
    const Td pointee;
    explicit constexpr ETakeAddr(Td pointee) : pointee{pointee} {}
    IR_TRANSLATABLE
};

template<class T1, class T2> struct CanAssign {
    static void constraints(T1 a, T2 b) { a = b; }
    CanAssign() { void(*p)(T1, T2) = constraints; }
};

//template<typename Tw, typename T, bool is_const = false, bool is_volatile = false>
template<typename Tw, typename T, bool is_const = false>
struct Value : ValueBase {
    using this_t = Value<Tw, T, is_const>;
    using type = std::conditional_t<is_const, std::add_const_t<T>, T>;
//    static const bool const_q = is_const;

    constexpr Value() = default;
    constexpr Value(this_t&&) = default;
    constexpr Value(const this_t&) = default;

    template<typename Td>
    inline constexpr auto assign(Td rhs) const {
        CanAssign<type, f_t<Td>>{};
        return EAssign{ static_cast<const Tw&>(*this), Cast<Tw>(rhs) };
    }

    template<typename Td, typename = typename std::enable_if_t<
            !std::is_convertible_v<Tw, Td>
    >>
    inline constexpr auto operator=(Td rhs) const { return this->assign(rhs); }

    // Member function templates never suppress generation of special member functions
    //  so, explicitly define copy assignment to avoid default behaviour and make it behave as assign()
    inline constexpr auto operator=(const this_t& rhs) const { return this->assign(rhs); };
//    constexpr auto operator=(const Tw& rhs) const { return this->assign(rhs); }; // didn't work

    inline constexpr auto takeAddr() const { return ETakeAddr<Tw>{ static_cast<const Tw&>(*this) }; }
};


}
}

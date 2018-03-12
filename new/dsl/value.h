#pragma once


#include "dsl_base.h"
#include "dsl_type_traits.h"
//#include "ptr.h"


namespace hetarch {
namespace dsl {


using dsl::i_t; // by some reasons CLion can't resolve it automatically.
using dsl::f_t; // by some reasons CLion can't resolve it automatically.
using dsl::remove_cvref_t;


// todo: EAssign is a Value, really. we can do (a=b)=c
// todo: do implicit cast when not exact type match?
template<typename TdLhs, typename TdRhs
        , typename = typename std::enable_if_t<
//                std::is_same_v<i_t<TdLhs>, i_t<TdRhs>> &&
                is_val_v<TdLhs>
        >
>
struct EAssign : public Expr<f_t<TdLhs>> {
    const TdLhs lhs;
    const TdRhs rhs;

    constexpr EAssign(TdLhs lhs, TdRhs rhs) : lhs{lhs}, rhs{rhs} {}

    IR_TRANSLATABLE
};


template< typename Td
        , typename = typename std::enable_if_t< is_val_v<Td> >>
struct ETakeAddr: public Expr<f_t<Td>*> {
    const Td pointee;
    explicit constexpr ETakeAddr(Td pointee) : pointee{pointee} {}
    IR_TRANSLATABLE
};


//template<typename Tw, typename T, bool is_const = false, bool is_volatile = false>
template<typename Tw, typename T, bool is_const = false>
struct Value : public ValueBase {
    using this_t = Value<Tw, T, is_const>;
    using type = std::conditional_t<is_const, std::add_const_t<T>, T>;
//    static const bool const_q = is_const;

    Value() = default;
    Value(const this_t&) = default;
    Value(this_t&&) = default;

    // todo: better & more correct check instead of the_same
    // todo: some sane casting
    template<typename Td, typename = typename std::enable_if_t<
//            std::is_same_v<remove_cvref_t<T>, i_t<Td>>
//            std::is_convertible_v<i_t<Td>, remove_cvref_t<T>> // doesn't work for volatile ptr-s
//            std::is_same_v<remove_cvref_t< std::remove_pointer_t<T> >, i_t<Td>>
            true
    >>
    constexpr auto assign(Td rhs) const {
        static_assert(!is_const, "Assigning to const!");
        return EAssign<Tw, Td>{static_cast<const Tw&>(*this), rhs};
    }

    template<typename Td, typename = typename std::enable_if_t<
            !std::is_convertible_v<Tw, Td>
//            !std::is_base_of_v<this_t, remove_cvref_t<Td>>
    >>
    constexpr auto operator=(Td rhs) const { return this->assign(rhs); }

    // Member function templates never suppress generation of special member functions
    //  so, explicitly define copy assignment to avoid default behaviour and make it behave as assign()
    constexpr auto operator=(const this_t& rhs) const { return this->assign(rhs); };
//    constexpr auto operator=(const Tw& rhs) const { return this->assign(rhs); }; // didn't work

    inline constexpr auto takeAddr() const { return ETakeAddr<Tw>{ static_cast<const Tw&>(*this) }; }
//    inline constexpr auto operator&() const { return this->takeAddr(); }
};


}
}

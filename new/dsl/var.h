#pragma once


#include "dsl_base.h"
#include "dsl_type_traits.h"


namespace hetarch {
namespace dsl {

using dsl::i_t; // by some reasons CLion can't resolve it automatically.

// todo: difference with LocalVar is that GlobalVar is Loadable. implement it.
//template<typename T, bool isConst = false, bool is_volatile = false>
//using GlobalVar<T, is_const, is_volatile> = Var<T, isConst, is_volatile>;
//struct GlobalVar : public Var<T, is_const, is_volatile>, public Assignable<T, isConst> {
//    using Var<T, isConst, is_volatile>::Var;
//};


// todo: do implicit cast when not exact type match?
template<typename TdLhs, typename TdRhs
        , typename = typename std::enable_if_t<std::is_same_v<i_t<TdLhs>, i_t<TdRhs>>>
>
struct EAssign : public Expr<i_t<TdRhs>> {
    const TdLhs& lhs;
    const TdRhs rhs;

    constexpr EAssign(const TdLhs& lhs, TdRhs&& rhs) : lhs{lhs}, rhs{std::forward<TdRhs>(rhs)} {}

    inline void toIR(IRTranslator &irTranslator) const override { toIRImpl(*this, irTranslator); }
};


template<typename Tw, bool is_const> struct Value;

/* Ptr
 * . it is not Var because no sense in 'initialiasing' Ptr. user should construct only Var.
 *
 * ? Named
 * ? use of Ptr in Expr-s
 *      --> ensure that type-safety is preserved through use of 'type = Td*'
 */

template<typename Td>
struct EDeref : public Expr<i_t<Td>> {
    const Td& x;
    explicit constexpr EDeref(const Td& x) : x{x} {}
};

template<typename Td, bool is_const, typename = typename std::enable_if_t<std::is_base_of_v<ValueBase, Td>>>
struct Ptr : public Value<Ptr<Td, is_const>, is_const> {
    using type = i_t<Td>*;

    const Td& pointee;

    explicit constexpr Ptr(const Td& pointee) : pointee{pointee} {}

    constexpr auto operator*() const { return EDeref<Td>{pointee}; }
//    constexpr auto operator->() const {}
//    constexpr auto operator->*() const {}

};

template<typename Td>
struct ETakeAddr: public Expr<i_t<Td>> {
    const Td& x;
    explicit constexpr ETakeAddr(const Td& x) : x{x} {}
};



// TODO: maybe do TW template template parameter
//   todo: if all possible values are captured by T (T, T*, T[N], ???T-struct)
template<typename Tw, bool is_const = false>
//template<typename Tw, typename T, bool is_const = false, bool is_volatile = false>
struct Value : public ValueBase {
    // TODO: impossible to see underlying type here
//    using type = typename Tw::type;

    template<typename Td, typename Tw_ = Tw
            , typename = typename std::enable_if_t< std::is_same_v<typename Tw_::type, i_t<Td>> && !is_const >
//            , typename = typename std::enable_if_t< std::is_same_v<T, i_t<Td>> && !is_const >
    >
    constexpr auto assign(Td&& rhs) const {
        return EAssign<Tw, Td>{static_cast<const Tw&>(*this), std::forward<Td>(rhs)};
    }
    template<typename Td>
    constexpr auto operator=(Td&& rhs) const { return this->assign(std::forward<Td>(rhs)); }

    template<bool const_ptr = false>
    constexpr auto takeAddr() const {
//        return ETakeAddr<Tw, const_ptr>{static_cast<const Tw&>(*this)};
        return Ptr<Tw, const_ptr>{static_cast<const Tw&>(*this)};
    }
//    ValueBase* operator&() { return this; }

};


template<typename T, bool is_const = false, bool is_volatile = false>
class Var : public Named, public Value<Var<T, is_const, is_volatile>, is_const> {
    T m_initial_val{};
    bool m_initialised{false};
public:
    using type = T;

    using Value<Var<T, is_const, is_volatile>, is_const>::operator=;

    explicit constexpr Var() = default;
    explicit constexpr Var(const std::string_view &name) : Named{name} {};
    explicit constexpr Var(T value) : m_initial_val{value}, m_initialised{true} {}
    explicit constexpr Var(T value, const std::string_view &name)
    : Named{name}, m_initial_val{value}, m_initialised{true} {}

    constexpr bool isVolatile() const { return is_volatile; }

    constexpr void initialise(T value) {
        m_initial_val = std::move(value);
        m_initialised = true;
    }
    constexpr bool initialised() const { return m_initialised; }
    constexpr const T& initial_val() const { return m_initial_val; }

    inline void toIR(IRTranslator &irTranslator) const override { toIRImpl(*this, irTranslator); }
};

template<typename T, bool is_volatile = false>
using Const = Var<T, true, is_volatile>;


}
}

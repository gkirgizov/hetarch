#pragma once


#include "dsl_base.h"


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

//template<typename T, bool, bool> struct Var;

struct ValueBase : public DSLBase {};

template<typename Tw, bool is_const = false, bool is_volatile = false>
//template<typename Tw, typename T, bool is_const = false, bool is_volatile = false>
struct Value : public ValueBase {
    // TODO: impossible to see underlying type here
//    using type = typename Tw::type;

    constexpr bool isConst() const { return is_const; }
    constexpr bool isVolatile() const { return is_volatile; }

    template<typename Td, typename Tw_ = Tw
            , typename = typename std::enable_if_t<std::is_same_v<typename Tw_::type, i_t<Td>>>
//            , typename = typename std::enable_if_t<std::is_same_v<T, i_t<Td>>>
    >
    constexpr auto assign(Td&& rhs) const {
        return EAssign<Tw, Td>{static_cast<const Tw&>(*this), std::forward<Td>(rhs)};
    }
    template<typename Td>
    constexpr auto operator=(Td&& rhs) const { return this->assign(std::forward<Td>(rhs)); }

};

template<typename Tw, bool is_volatile> struct Value<Tw, true, is_volatile> : public DSLBase { using type = typename Tw::type; };
template<typename Tw, bool is_volatile = false> using ConstValue = Value<Tw, true, is_volatile>;


template<typename T, bool is_const = false, bool is_volatile = false>
class Var : public Named, public Value<Var<T, is_const, is_volatile>, is_const, is_volatile> {
    T m_initial_val{};
    bool m_initialised{false};
public:
    using type = T;

    using Value<Var<T, is_const, is_volatile>, is_const, is_volatile>::operator=;

    explicit constexpr Var() = default;
    explicit constexpr Var(const std::string_view &name) : Named{name} {};
    explicit constexpr Var(T value) : m_initial_val{std::move(value)}, m_initialised{true} {}
    explicit constexpr Var(T value, const std::string_view &name)
    : Named{name}, m_initial_val{std::move(value)}, m_initialised{true} {}

    constexpr void initialise(T value) {
        m_initial_val = std::move(value);
        m_initialised = true;
    }
    constexpr bool initialised() const { return m_initialised; }
    constexpr T initial_val() const { return m_initial_val; }

    inline void toIR(IRTranslator &irTranslator) const override { toIRImpl(*this, irTranslator); }
};

template<typename T, bool is_volatile = false>
using Const = Var<T, true, is_volatile>;



}
}

#pragma once


#include "dsl_base.h"
#include "dsl_type_traits.h"


namespace hetarch {
namespace dsl {


using dsl::i_t; // by some reasons CLion can't resolve it automatically.
using dsl::remove_cvref_t;


// todo: temp
template<typename T, typename = typename std::enable_if_t< std::is_arithmetic_v<T> >>
struct DSLConst : public Expr<T> {
    const T val;
    constexpr DSLConst(T val) : val{val} {}
};

//template<typename T, typename = typename std::enable_if_t< std::is_arithmetic_v<T> >>
//constexpr auto operator"" _dsl (T val) { return DSLConst<T>{val}; };


template<typename Td, typename = typename std::enable_if_t< is_val_v<Td> >>
struct DSLGlobal {
    const Td x;

    // only rvalues allowed
    explicit constexpr DSLGlobal(Td&& x) : x{std::move(x)} {}

//    inline constexpr auto name() const { return x.name(); }

    inline void toIR(IRTranslator &irTranslator) const { toIRImpl(*this, irTranslator); }
};


/*// todo: do implicit cast when not exact type match?
template<typename TdLhs, typename TdRhs
        , typename = typename std::enable_if_t<
                std::is_same_v<i_t<TdLhs>, i_t<TdRhs>>
                && is_val_v<TdLhs>
        >
>
struct EAssign : public TdLhs { // Assignment returns reference to assignee, so, EAssign behaves in expressions as assignee
    const TdLhs& lhs;
    const TdRhs rhs;

    constexpr EAssign(const TdLhs& lhs, TdRhs&& rhs) : lhs{lhs}, rhs{std::forward<TdRhs>(rhs)} {}

    using this_t = EAssign<TdLhs, TdRhs>;
    using TdLhs::operator=;
    constexpr auto operator=(const this_t& rhs) const { return this->assign(rhs); };

    inline void toIR(IRTranslator &irTranslator) const { toIRImpl(*this, irTranslator); }
};*/

// todo: do implicit cast when not exact type match?
template<typename TdLhs, typename TdRhs
        , typename = typename std::enable_if_t<
                std::is_same_v<i_t<TdLhs>, i_t<TdRhs>>
                && is_val_v<TdLhs>
        >
>
struct EAssign : public Expr<f_t<TdLhs>> {
    const TdLhs& lhs;
    const TdRhs rhs;

    constexpr EAssign(const TdLhs& lhs, TdRhs&& rhs) : lhs{lhs}, rhs{std::forward<TdRhs>(rhs)} {}

    inline void toIR(IRTranslator &irTranslator) const { toIRImpl(*this, irTranslator); }
};


//template<typename Tw, typename T, bool is_const = false, bool is_volatile = false>
template<typename Tw, typename T, bool is_const = false>
struct Value : public ValueBase {
    using this_t = Value<Tw, T, is_const>;
    using type = T;
    Value() = default;
//    constexpr Value(this_t&&) = default; // allow move and implicitly delete copy ctor
//    Value(const this_t&) = delete; // delete copy ctor

    template<typename Td, typename = typename std::enable_if_t<
                    std::is_same_v<remove_cvref_t<T>, i_t<Td>>
                    && !is_const
    >>
    constexpr auto assign(Td&& rhs) const {
        return EAssign<Tw, Td>{static_cast<const Tw&>(*this), std::forward<Td>(rhs)};
    }

    template<typename Td, typename = typename std::enable_if_t<
            !std::is_convertible_v<Tw, Td>
//            !std::is_base_of_v<this_t, remove_cvref_t<Td>>
    >>
    constexpr auto operator=(Td&& rhs) const { return this->assign(std::forward<Td>(rhs)); }

    // Member function templates never suppress generation of special member functions
    //  so, explicitly define copy assignment to avoid default behaviour and make it behave as assign()
    constexpr auto operator=(const this_t& rhs) const { return this->assign(rhs); };
//    constexpr auto operator=(const Tw& rhs) const { return this->assign(rhs); };

//    template<bool const_ptr = false>
//    constexpr auto takeAddr() const {
//        return ETakeAddr<Tw, const_ptr>{static_cast<const Tw&>(*this)};
//        return Ptr<Tw, const_ptr>{static_cast<const Tw&>(*this)};
//    }
//    ValueBase* operator&() { return this; }
};


template<typename T, bool is_const = std::is_const_v<T>, bool is_volatile = std::is_volatile_v<T>
//        , typename = typename std::enable_if_t<std::is_arithmetic_v<T>>
>
class Var : public Named, public Value<Var<T, is_const, is_volatile>, T, is_const> {
    T m_initial_val{};
    bool m_initialised{false};
public:
    static const bool volatile_q = is_volatile;
    static const bool const_q = is_const;

    using this_t = Var<T, is_const, is_volatile>;
    using Value<this_t, T, is_const>::operator=;
    constexpr auto operator=(const this_t& rhs) const { return this->assign(rhs); };

//    Var(Var<T, is_const, is_volatile>&) { std::cerr << "CALLED COPY CTOR OF Var" << std::endl; };
    explicit constexpr Var() = default;
//    explicit constexpr Var(const std::string_view &name) : Named{name} {};
    explicit constexpr Var(T value) : m_initial_val{value}, m_initialised{true} {}
    explicit constexpr Var(T value, const std::string_view &name)
    : Named{name}, m_initial_val{value}, m_initialised{true} {}

    constexpr void initialise(T value) {
        m_initial_val = std::move(value);
        m_initialised = true;
    }
    constexpr bool initialised() const { return m_initialised; }
    constexpr const T& initial_val() const { return m_initial_val; }

    inline void toIR(IRTranslator &irTranslator) const { toIRImpl(*this, irTranslator); }
};

template<typename T, bool is_volatile = false>
using Const = Var<T, true, is_volatile>;


}
}

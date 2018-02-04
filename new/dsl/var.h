#pragma once


#include "IDSLCallable.h"


namespace hetarch {
namespace dsl {


template<typename T> class EAssign;


// todo: enable_if only for integral etc. simple types?
template<typename T>
class VarBase : public Expr<T> {
    T m_initial_val{};
    bool m_initialised{false};
public:
    const std::string_view name{make_bsv("")};

    constexpr VarBase(VarBase<T> &&) = default;
    constexpr VarBase(const VarBase<T> &) = default; // TODO: somehow preserve 'identity' among copies
    constexpr VarBase<T> &operator=(VarBase<T> &&) = delete; // doesn't make much sense to move-assign
    constexpr VarBase<T> &operator=(VarBase<T> &) = delete; // doesn't make much sense to copy-assign

    explicit constexpr VarBase() = default;
    explicit constexpr VarBase(const std::string_view &name) : name{name} {};
    explicit constexpr VarBase(T &&value) : m_initial_val{std::forward<T>(value)}, m_initialised{true} {}
    explicit constexpr VarBase(T &&value, const std::string_view &name)
            : name{name}, m_initial_val{std::forward<T>(value)}, m_initialised{true} {}

    constexpr void initialise(T &&value) {
        m_initial_val = std::forward(value);
        m_initialised = true;
    }
    constexpr bool initialised() const { return m_initialised; }
    constexpr T initial_val() const { return m_initial_val; }
};


// cv-qualifiers are part of type signature, so it makes sense to put them here as template parameters
template<typename T, bool is_const = false, bool is_volatile = false>
struct Var : public VarBase<T> {
    using type = T;

    using VarBase<T>::VarBase;

    inline constexpr EAssign<T> assign(Expr<T> &&rhs) const;
    inline constexpr EAssign<T> operator=(Expr<T> &&rhs) const { return this->assign(std::move(rhs)); };

    inline constexpr EAssign<T> assign(const VarBase<T> &rhs) const;
    inline constexpr EAssign<T> operator=(const VarBase<T> &rhs) const { return this->assign(rhs); };

    friend class IRTranslator;
    inline void toIR(IRTranslator &irTranslator) const override { toIRImpl(*this, irTranslator); }
};

// specialisation for const
template<typename T, bool is_volatile>
struct Var<T, true, is_volatile> : public VarBase<T> {
    using type = T;

    using VarBase<T>::VarBase;

    friend class IRTranslator;
    inline void toIR(IRTranslator &irTranslator) const override { toIRImpl(*this, irTranslator); }
};

template<typename T, bool is_volatile = false>
using Const = Var<T, true, is_volatile>;


// todo: difference with LocalVar is that GlobalVar is Loadable. implement it.
//template<typename T, bool is_const = false, bool is_volatile = false>
//using GlobalVar<T, is_const, is_volatile> = Var<T, is_const, is_volatile>;
//struct GlobalVar : public Var<T, is_const, is_volatile>, public Assignable<T, is_const> {
//    using Var<T, is_const, is_volatile>::Var;
//};



template<typename T>
class EAssign : public Expr<T> {
    const VarBase<T> &var;
    const Expr<T> &rhs;
public:
    constexpr EAssign(EAssign<T> &&) = default;
    constexpr EAssign<T> &operator=(EAssign<T> &&) = delete;

    constexpr EAssign(const VarBase<T> &var, Expr<T> &&rhs) : var{var}, rhs{rhs} {}
    constexpr EAssign(const VarBase<T> &var, const VarBase<T> &rhs) : var{var}, rhs{rhs} {}

    friend class IRTranslator;
    inline void toIR(IRTranslator &irTranslator) const override { toIRImpl(*this, irTranslator); }
};



template<typename T, bool is_const, bool is_volatile>
constexpr EAssign<T> Var<T, is_const, is_volatile>::assign(Expr<T> &&rhs) const {
    return EAssign<T>{*this, std::move(rhs)};
}

template<typename T, bool is_const, bool is_volatile>
constexpr EAssign<T> Var<T, is_const, is_volatile>::assign(const VarBase<T> &rhs) const {
    return EAssign<T>{*this, rhs};
}


}
}

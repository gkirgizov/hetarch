#pragma once


#include "dsl_base.h"
#include "dsl_type_traits.h"
#include "value.h"
#include "ResidentGlobal.h"


namespace hetarch {
namespace dsl {


using dsl::i_t; // by some reasons CLion can't resolve it automatically.
using dsl::remove_cvref_t;


template<typename Td
        , typename = typename std::enable_if_t< is_val_v<Td> >>
struct DSLGlobal : public ValueBase {
    const Td x;
    explicit DSLGlobal() = default;
    // only rvalues allowed
    explicit constexpr DSLGlobal(Td x) : x{x} {}
    IR_TRANSLATABLE
};

/*// when user wants to load global on specific addr (e.g. in case of volatile var-s)
template<typename AddrT
        , typename Td
        , typename = typename std::enable_if_t< is_val_v<Td> >>
struct DSLGlobalPreallocated : public ValueBase {
    const Td x;
    const AddrT addr;

    explicit constexpr DSLGlobalPreallocated(AddrT addr) : x{}, addr{addr} {}
    explicit constexpr DSLGlobalPreallocated(AddrT addr, Td x) : x{x}, addr{addr} {}
    IR_TRANSLATABLE
};*/



template<typename TdVar, typename T
        , bool is_const = false, bool is_volatile = false
        , typename = typename std::enable_if_t<std::is_arithmetic_v<T>>
>
class VarBase : public Value< TdVar, T, is_const >
              , public Named
{
    T m_initial_val{};
    bool m_initialised{false};
public:
    template<typename TdChild> using base_t = VarBase<TdChild, T, is_const, is_volatile>;
    
    static const bool volatile_q = is_volatile;
    static const bool const_q = is_const;

    constexpr void initialise(T value) {
        m_initial_val = value;
        m_initialised = true;
    }
    constexpr bool initialised() const { return m_initialised; }
    constexpr T initial_val() const { return m_initial_val; }

    explicit constexpr VarBase() = default;
//    explicit constexpr VarBase(const std::string_view &name = "") : Named{name} {}
    explicit constexpr VarBase(T value, const std::string_view &name = "")
            : Named{name}, m_initial_val{value}, m_initialised{true} {}

    MEMBER_ASSIGN_OP(TdVar, +)
    MEMBER_ASSIGN_OP(TdVar, -)
    MEMBER_ASSIGN_OP(TdVar, *)
    MEMBER_ASSIGN_OP(TdVar, |)
    MEMBER_ASSIGN_OP(TdVar, &)
    MEMBER_ASSIGN_OP(TdVar, ^)
    MEMBER_XX_OP(TdVar, +) // ++
    MEMBER_XX_OP(TdVar, -) // --
};


template<typename T, bool is_const = std::is_const_v<T>, bool is_volatile = std::is_volatile_v<T> >
struct Var : public VarBase< Var<T, is_const, is_volatile>, T, is_const, is_volatile >
{
    using this_t = Var<T, is_const, is_volatile>;
    using Value<this_t, T, is_const>::operator=;
    constexpr auto operator=(const this_t& rhs) const { return this->assign(rhs); };
    constexpr Var(this_t&&) = default;
    constexpr Var(const this_t&) = default;

//    using VarBase<this_t, T, is_const, is_volatile>::VarBase; // breaks template arg deduction from constructor
    explicit constexpr Var() = default;
//    explicit constexpr Var(const std::string_view &name = "")
//            : VarBase<this_t, T, is_const, is_volatile>{name} {}
    explicit constexpr Var(T value, const std::string_view &name = "")
            : VarBase<this_t, T, is_const, is_volatile>{value, name} {}

    IR_TRANSLATABLE
};

template<typename T, bool is_volatile = false>
using Const = Var<T, true, is_volatile>;


template<typename AddrT
        , typename T, bool is_const = std::is_const_v<T>, bool is_volatile = std::is_volatile_v<T>
>
struct ResidentVar
        : public VarBase< ResidentVar<AddrT, T, is_const, is_volatile>, T, is_const, is_volatile >
        , public ResidentGlobal< AddrT, T, is_const >
{
    using this_t = ResidentVar<AddrT, T, is_const, is_volatile>;
    using Value<this_t, T, is_const>::operator=;
    constexpr auto operator=(const this_t& rhs) const { return this->assign(rhs); };
    constexpr ResidentVar(this_t&&) = default;
    constexpr ResidentVar(const this_t&) = default;

    constexpr ResidentVar(
            conn::IConnection<AddrT>& conn,
            mem::MemManager<AddrT>& memMgr, mem::MemRegion<AddrT> memRegion, bool unloadable,
            T value = T{},
            const std::string_view &name = ""
    )
            : ResidentGlobal<AddrT, T, is_const>{conn, memMgr, memRegion, unloadable}
            , VarBase<this_t, T, is_const, is_volatile>{value, name} // note: always initialised
    {}

    IR_TRANSLATABLE
};


}
}

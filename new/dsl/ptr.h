#pragma once


#include "dsl_base.h"
#include "dsl_type_traits.h"
//#include "value.h"
#include "ResidentGlobal.h"


namespace hetarch {
namespace dsl {

using dsl::i_t; // by some reasons CLion can't resolve it automatically.
using dsl::f_t; // by some reasons CLion can't resolve it automatically.

template<typename Tw, typename T, bool> struct Value;


template<typename TdPtr>
struct EDeref : get_base_t< get_dsl_element_t<TdPtr>, EDeref<TdPtr> >
{
    using type = get_element_t<TdPtr>;
    using this_t = EDeref<TdPtr>;
    using Value<this_t, type, std::remove_reference_t<TdPtr>::elt_const_q>::operator=;
    constexpr auto operator=(const this_t& rhs) const { return this->assign(rhs); };
    constexpr EDeref(this_t&&) = default;
    constexpr EDeref(const this_t&) = default;

    const TdPtr ptr;
    explicit constexpr EDeref(TdPtr ptr) : ptr{ptr} {}

    IR_TRANSLATABLE
};


// todo: make volatile ptr (not just ptr to volatile, which is handled by element type)
template< typename TdPtr
        , typename TdPointee
        , bool is_const = false
        , typename = typename std::enable_if_t< is_val_v<TdPointee> >
>
class PtrBase : public Value< TdPtr, f_t<TdPointee>*, is_const >
              , public Named
{
    using val_type = size_t;
    val_type m_initial_val{};
    bool m_initialised{false};

public:
    template<typename TdChild> using base_t = PtrBase<TdChild, TdPointee, is_const>;

    using element_t = f_t<TdPointee>;
    using dsl_element_t = remove_cvref_t<TdPointee>;
    static const bool const_q = is_const;
    static const bool volatile_q = is_const; // todo: tmp
    static const bool elt_const_q = dsl_element_t::const_q;
    static const bool elt_volatile_q = dsl_element_t::volatile_q;

    explicit constexpr PtrBase() = default;
    explicit constexpr PtrBase(val_type val, std::string_view name = "")
            : m_initial_val{val}, m_initialised{true}, Named{name} {}

    constexpr void initialise(val_type value) {
        m_initial_val = value;
        m_initialised = true;
    }
    constexpr bool initialised() const { return m_initialised; }
    constexpr val_type initial_val() const { return m_initial_val; }

    constexpr auto operator*() const { return EDeref<TdPtr>{static_cast<const TdPtr&>(*this)}; }

    MEMBER_ASSIGN_OP(TdPtr, +)
    MEMBER_ASSIGN_OP(TdPtr, -)
    MEMBER_XX_OP(TdPtr, +)
    MEMBER_XX_OP(TdPtr, -)
};


template<typename Td, bool is_const = false>
struct RawPtr: PtrBase< RawPtr<Td, is_const>, Td, is_const >
{
    using type = f_t<Td>*;
    using this_t = RawPtr<Td, is_const>;
    using Value<this_t, type, is_const>::operator=;
    constexpr auto operator=(const this_t& rhs) const { return this->assign(rhs); };
    constexpr RawPtr(this_t&&) = default;
    constexpr RawPtr(const this_t&) = default;

    using PtrBase< this_t, Td, is_const >::PtrBase;

    IR_TRANSLATABLE
};

template<typename AddrT, typename Td, bool is_const = false>
struct ResidentPtr
        : PtrBase< ResidentPtr<AddrT, Td, is_const>, Td, is_const >
        , ResidentGlobal<AddrT, f_t<Td>*, is_const>
{
    using val_type = size_t;

    using type = f_t<Td>*;
    using this_t = ResidentPtr<AddrT, Td, is_const>;
    using Value<this_t, type, is_const>::operator=;
    constexpr auto operator=(const this_t& rhs) const { return this->assign(rhs); };
    constexpr ResidentPtr(this_t&&) = default;
    constexpr ResidentPtr(const this_t&) = default;

    constexpr ResidentPtr(
            conn::IConnection<AddrT>& conn,
            mem::MemManager<AddrT>& memMgr, mem::MemRegion<AddrT> memRegion, bool unloadable,
            val_type value = val_type{},
            const std::string_view &name = ""
    )
            : ResidentGlobal<AddrT, type, is_const>{conn, memMgr, memRegion, unloadable}
            , PtrBase<this_t, Td, is_const>{value, name}
    {}

    IR_TRANSLATABLE
};


template<typename>
struct is_dsl_ptr : std::false_type {};
template< typename TdPtr, typename TdPointee, bool is_const >
struct is_dsl_ptr< PtrBase<TdPtr, TdPointee, is_const> > : std::true_type {};
template<typename Td> inline constexpr bool is_dsl_ptr_v = is_dsl_ptr<Td>::value;


}
}

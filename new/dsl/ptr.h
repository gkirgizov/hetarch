#pragma once


#include "dsl_base.h"
#include "dsl_type_traits.h"
#include "var.h"


namespace hetarch {
namespace dsl {


using dsl::i_t; // by some reasons CLion can't resolve it automatically.
using dsl::f_t; // by some reasons CLion can't resolve it automatically.


template<typename TdPtr>
struct EDeref : public Value< EDeref<TdPtr>
                            , typename std::remove_reference_t<TdPtr>::element_t
                            , TdPtr::elt_const_q>
{
    using type = typename std::remove_reference_t<TdPtr>::element_t;
    using this_t = EDeref<TdPtr>;
    using Value<this_t, type, TdPtr::elt_const_q>::operator=;
    constexpr auto operator=(const this_t& rhs) const { return this->assign(rhs); };

    const TdPtr& ptr;
    explicit constexpr EDeref(const TdPtr& ptr) : ptr{ptr} {}

    IR_TRANSLATABLE
};


//template< bool is_global
//        , typename TdPtr
template< typename TdPtr
        , typename TdPointee
        , bool is_const = false
        , typename = typename std::enable_if_t<std::is_base_of_v<ValueBase, TdPointee>>
>
struct PtrBase: public Value< TdPtr, f_t<TdPointee>*, is_const >
{
    using element_t = f_t<TdPointee>;
    using dsl_element_t = remove_cvref_t<TdPointee>;
    static const bool const_q = is_const;
    static const bool elt_const_q = dsl_element_t::const_q;
    static const bool elt_volatile_q = dsl_element_t::volatile_q;

    constexpr auto operator*() const { return EDeref<TdPtr>{static_cast<const TdPtr&>(*this)}; }

    // todo: anything with assignment? const assignment?
};


// todo: allow nullptr?
template<typename Td, bool is_const = false>
struct Ptr : public PtrBase< Ptr<Td, is_const>, Td, is_const >
           , public Named
{
    const Td& pointee;
    explicit constexpr Ptr(const Td& pointee)
            : pointee{pointee}, Named{std::string{"ptr_"} + pointee.name().data()} {}

    IR_TRANSLATABLE
};

template<typename AddrT, typename Td, bool is_const = false>
struct ResidentPtr
        : public PtrBase< ResidentPtr<AddrT, Td, is_const>, Td, is_const >
        , public ResidentGlobal<AddrT, f_t<Td>*, is_const>
        , public Named
{
    using type = f_t<Td>*;

    constexpr ResidentPtr(
            conn::IConnection<AddrT>& conn,
            mem::MemManager<AddrT>& memMgr, mem::MemRegion<AddrT> memRegion, bool unloadable,
            const std::string_view &name = ""
    )
            : ResidentGlobal<AddrT, type, is_const>{conn, memMgr, memRegion, unloadable}
            , Named{name}
    {}

    IR_TRANSLATABLE
};


template<typename Td, typename = typename std::enable_if_t<std::is_base_of_v<ValueBase, Td>>>
struct ETakeAddr: public Expr<i_t<Td>> {
    using type = i_t<Td>*;
    const Td& pointee;
    explicit constexpr ETakeAddr(const Td& pointee) : pointee{pointee} {}
};


}
}

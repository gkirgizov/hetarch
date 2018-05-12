#pragma once

#include <tuple>

#include "dsl_base.h"
#include "dsl_type_traits.h"
#include "var.h"


namespace hetarch {
namespace dsl {


using dsl::i_t; // by some reasons CLion can't resolve it automatically.
using dsl::f_t; // by some reasons CLion can't resolve it automatically.


template<typename ...Tds>
using struct_impl_t = std::tuple<f_t<Tds>...>;


template<typename TdStruct, std::size_t I>
struct EStructAccess : get_base_t<
        typename std::remove_reference_t<TdStruct>::template dsl_element_t<I>
//                                 get_dsl_element_t<TdStruct, I>
                                 , EStructAccess<TdStruct, I>
                                 >
{

    using type = typename std::remove_reference_t<TdStruct>::template element_t<I>;
//    using type = get_element_t<TdStruct>;
    using this_t = EStructAccess<TdStruct, I>;
    using Value<this_t, type, TdStruct::template elt_const_q<I>>::operator=;
    constexpr auto operator=(const this_t& rhs) const { return this->assign(rhs); };
    constexpr EStructAccess(this_t&&) = default;
    constexpr EStructAccess(const this_t&) = default;

    const TdStruct& s;
    explicit constexpr EStructAccess(const TdStruct& s) : s{s} {}
    IR_TRANSLATABLE
};


template<typename TdStruct, bool is_const, bool is_volatile, typename ...Tds>
struct StructBase : Value< TdStruct
                         , struct_impl_t<Tds...>
                         , is_const
                         >
                  , Named
{
    template<typename Td> using base_t = StructBase<Td, is_const, is_volatile, Tds...>;

    static_assert(is_val_v<Tds...>, "Expected DSL types as elements of DSL tuple!");

    using TdsTuple = std::tuple<Tds...>;
    template<std::size_t I> using dsl_element_t = remove_cvref_t< std::tuple_element_t<I, TdsTuple> >;
    template<std::size_t I> using element_t = f_t< dsl_element_t<I> >;
    template<std::size_t I> static const bool elt_const_q = dsl_element_t<I>::const_q;
    template<std::size_t I> static const bool elt_volatile_q = dsl_element_t<I>::volatile_q;
    static const bool const_q = is_const;
    static const bool volatile_q = is_volatile;

    // todo: why only at compile time? unnsecessary restrictions. we do dynamic code gen.
    // can reliably index Struct only by index (i.e. like struct.first, struct.second)
    template<std::size_t I>
    constexpr auto get() const {
        static_assert(I >= 0 && I < sizeof...(Tds), "Struct element index is out of bounds");
        return EStructAccess<TdStruct, I>{ static_cast<const TdStruct&>(*this) };
    }


//    using key_t = std::string_view;
    // todo: make it of needed length
//    using map_t = std::tuple<std::pair<key_t, std::size_t>>;
    // todo: oh shit; it is possible only if Struct contains keys in its type.
//    static constexpr key_map =
//    template<typename Td> using field_t = pair<key_t, Td>;


private:
    using struct_t = struct_impl_t<Tds...>;

    struct_t m_initial_val{};
    bool m_initialised{false};

public:
    constexpr void initialise(const struct_t& value) {
        m_initial_val = value;
        m_initialised = true;
    }
    constexpr bool initialised() const { return m_initialised; }
    constexpr const struct_t& initial_val() const { return m_initial_val; }

    explicit constexpr StructBase() = default;
    explicit constexpr StructBase(const struct_t& s, const std::string_view& name = "")
            : m_initial_val{s}, m_initialised{true}
            , Named{name} {}
};


template<bool is_const, bool is_volatile, typename ...Tds>
struct xStruct : StructBase< xStruct< is_const, is_volatile, Tds... >
                                    , is_const, is_volatile, Tds... >
{
    using type = struct_impl_t<Tds...>;
    using this_t = xStruct<is_const, is_volatile, Tds...>;
    using Value<this_t, type, is_const>::operator=;
    constexpr auto operator=(const this_t& rhs) const { return this->assign(rhs); };
    constexpr xStruct(this_t&&) = default;
    constexpr xStruct(const this_t&) = default;

    using StructBase< this_t, is_const, is_volatile, Tds... >::StructBase;

    IR_TRANSLATABLE
};

// todo: make factory funcs to allow type deduction
template<typename ...Tds> using Struct   = xStruct<false, false, Tds...>;
template<typename ...Tds> using cStruct  = xStruct<true,  false, Tds...>;
template<typename ...Tds> using vStruct  = xStruct<false, true,  Tds...>;
template<typename ...Tds> using cvStruct = xStruct<true,  true,  Tds...>;


template< typename AddrT
        , bool is_const, bool is_volatile, typename ...Tds >
struct xResidentStruct
        : StructBase< xResidentStruct< AddrT, is_const, is_volatile, Tds... >
                                            , is_const, is_volatile, Tds... >
        , ResidentGlobal< AddrT
                        , struct_impl_t<Tds...>
                        , is_const >
{
    using type = struct_impl_t<Tds...>;
    using this_t = xResidentStruct<AddrT, is_const, is_volatile, Tds...>;
    using Value<this_t, type, is_const>::operator=;
    constexpr auto operator=(const this_t& rhs) const { return this->assign(rhs); };
    constexpr xResidentStruct(this_t&&) = default;
    constexpr xResidentStruct(const this_t&) = default;

//    using StructBase< this_t, is_const, is_volatile, Tds... >::StructBase;
    constexpr xResidentStruct(
            conn::IConnection<AddrT>& conn,
            mem::MemManager<AddrT>& memMgr, mem::MemRegion<AddrT> memRegion, bool unloadable,
            const type& value = type{},
            const std::string_view &name = ""
    )
            : ResidentGlobal<AddrT, type, is_const>{conn, memMgr, memRegion, unloadable}
            , StructBase<this_t, is_const, is_volatile, Tds...>{value, name} // note: always initialised
    {}

    IR_TRANSLATABLE
};

template<typename AddrT, typename ...Tds> using ResidentStruct   = xResidentStruct<AddrT, false, false, Tds...>;
template<typename AddrT, typename ...Tds> using cResidentStruct  = xResidentStruct<AddrT, true,  false, Tds...>;
template<typename AddrT, typename ...Tds> using vResidentStruct  = xResidentStruct<AddrT, false, true,  Tds...>;
template<typename AddrT, typename ...Tds> using cvResidentStruct = xResidentStruct<AddrT, true,  true,  Tds...>;

}
}

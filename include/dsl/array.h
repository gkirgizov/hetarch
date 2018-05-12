#pragma once

#include <array>

#include "dsl_base.h"
#include "dsl_type_traits.h"
#include "value.h"


namespace hetarch {
namespace dsl {


using dsl::i_t; // by some reasons CLion can't resolve it automatically.
using dsl::f_t; // by some reasons CLion can't resolve it automatically.


template< typename TdArr, typename TdInd >
struct EArrayAccess : get_base_t< get_dsl_element_t<TdArr>, EArrayAccess<TdArr, TdInd> >
{
    using type = get_element_t<TdArr>;
    using this_t = EArrayAccess<TdArr, TdInd>;
    using Value<this_t, type, TdArr::elt_const_q>::operator=;
    constexpr auto operator=(const this_t& rhs) const { return this->assign(rhs); };
    constexpr EArrayAccess(this_t&&) = default;
    constexpr EArrayAccess(const this_t&) = default;

    const TdArr arr;
    const TdInd ind;
    constexpr EArrayAccess(TdArr arr, TdInd ind) : arr{arr}, ind{ind} {}

    IR_TRANSLATABLE
};


template<typename TdElem, std::size_t N>
using arr_impl_t = std::array<f_t<TdElem>, N>;

template< typename TdArr
        , typename TdElem
        , std::size_t N
        , bool is_const = false
//        , bool is_volatile = false
>
class ArrayBase : public Value< TdArr, std::array<f_t<TdElem>, N>, is_const >
                , public Named
{
public:
    // actually, class can't be isntantiated if TdElem is not DSL type, so this assert won't be checked ever
    static_assert(is_val_v<TdElem>, "DSL Array element type must be DSL type!");

    template<typename TdChild> using base_t = ArrayBase<TdChild, TdElem, N, is_const>;

    using element_t = f_t<TdElem>;
    using dsl_element_t = remove_cvref_t<TdElem>;
    static const bool const_q = is_const;
    static const bool volatile_q = false;
    static const bool elt_const_q = dsl_element_t::const_q;
    static const bool elt_volatile_q = dsl_element_t::volatile_q;

    template<typename TdInd
            , typename = typename std::enable_if_t< std::is_integral_v<f_t<TdInd>> >>
    constexpr auto operator[](TdInd ind) const {
        return EArrayAccess<TdArr, TdInd>{
                static_cast<const TdArr&>(*this), ind
        };
    }

    // todo: allow indexing by const int
//    constexpr auto operator[](const std::size_t I) const {
//        static_assert(I < N, "out of bounds error");
//        return EArrayAccess{*this, I};
//    }

private:
    using arr_t = arr_impl_t<TdElem, N>; // equiv. to Value::type
    arr_t m_initial_val{};
    bool m_initialised{false};

public:
    constexpr void initialise(const arr_t& value) {
        m_initial_val = value;
        m_initialised = true;
    }
    constexpr bool initialised() const { return m_initialised; }
    constexpr const arr_t& initial_val() const { return m_initial_val; }

    explicit constexpr ArrayBase() = default;
    explicit constexpr ArrayBase(const arr_t& a, const std::string_view& name = "")
            : m_initial_val{a}, m_initialised{true}
            , Named{name} {}
};


template< typename TdElem
        , std::size_t N
        , bool is_const = false
//        , bool is_volatile = false
>
struct Array : ArrayBase< Array< TdElem, N, is_const >
                               , TdElem, N, is_const >
{
    using type = arr_impl_t<TdElem, N>;
    using this_t = Array<TdElem, N, is_const>;
    using Value<this_t, type, is_const>::operator=;
    constexpr auto operator=(const this_t& rhs) const { return this->assign(rhs); };
    constexpr Array(this_t&&) = default;
    constexpr Array(const this_t&) = default;

    using ArrayBase< this_t, TdElem, N, is_const >::ArrayBase;

    IR_TRANSLATABLE
};


template< typename AddrT
        , typename TdElem
        , std::size_t N
        , bool is_const = false
//        , bool is_volatile = false
>
struct ResidentArray
        : ArrayBase< ResidentArray< AddrT, TdElem, N, is_const >
                                         , TdElem, N, is_const >
        , ResidentGlobal< AddrT
                        , arr_impl_t<TdElem, N>
                        , is_const >
{
    using type = arr_impl_t<TdElem, N>;
    using this_t = ResidentArray<AddrT, TdElem, N, is_const>;
    using Value<this_t, type, is_const>::operator=;
    constexpr auto operator=(const this_t& rhs) const { return this->assign(rhs); };
    constexpr ResidentArray(this_t&&) = default;
    constexpr ResidentArray(const this_t&) = default;

    constexpr ResidentArray(
            conn::IConnection<AddrT>& conn,
            mem::MemManager<AddrT>& memMgr, mem::MemRegion<AddrT> memRegion, bool unloadable,
            const type& value = type{},
            const std::string_view &name = ""
    )
            : ResidentGlobal<AddrT, type, is_const>{conn, memMgr, memRegion, unloadable}
            , ArrayBase<this_t, TdElem, N, is_const>{value, name} // note: always initialised
    {}

    IR_TRANSLATABLE
};



}
}

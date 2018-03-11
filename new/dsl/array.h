#pragma once

#include <array>

#include "dsl_base.h"
#include "dsl_type_traits.h"
#include "value.h"


namespace hetarch {
namespace dsl {


using dsl::i_t; // by some reasons CLion can't resolve it automatically.
using dsl::f_t; // by some reasons CLion can't resolve it automatically.


template< typename TdArr
        , typename TdInd
        , typename = typename std::enable_if_t< std::is_integral_v<f_t<TdInd>> >>
struct EArrayAccess : Value< EArrayAccess<TdArr, TdInd>
                           , typename TdArr::element_t
                           , TdArr::elt_const_q
                           >
{
    using type = typename TdArr::element_t;

    const TdArr& arr;
    const TdInd ind;

    constexpr EArrayAccess(const TdArr& arr, TdInd&& ind) : arr{arr}, ind{std::forward<TdInd>(ind)} {}

    using this_t = EArrayAccess<TdArr, TdInd>;
    using Value<this_t, type, TdArr::elt_const_q>::operator=;
    constexpr auto operator=(const this_t& rhs) const { return this->assign(rhs); };

    IR_TRANSLATABLE
};


template< typename TdArr
        , typename TdElem
        , std::size_t N
        , bool is_const = false
//        , bool is_volatile = false
        , typename = typename std::enable_if_t< is_val_v<TdElem> >
>
class ArrayBase : public Value< TdArr, std::array<f_t<TdElem>, N>, is_const >
                , public Named
{
public:
//    template<Td> using base_t = ArrayBase<Td, TdElem, N, is_const>;

    using element_t = f_t<TdElem>;
    using dsl_element_t = remove_cvref_t<TdElem>;
    static const bool const_q = is_const;
//    static const bool volatile_q = is_volatile;
    static const bool elt_const_q = dsl_element_t::const_q;
    static const bool elt_volatile_q = dsl_element_t::volatile_q;

    template<typename TdInd
            , typename = typename std::enable_if_t< std::is_integral_v<f_t<TdInd>> >>
    constexpr auto operator[](TdInd&& ind) const {
        return EArrayAccess<TdArr, TdInd>{
                static_cast<const TdArr&>(*this), std::forward<TdInd>(ind)
        };
    }

    // todo: allow indexing by const int
//    constexpr auto operator[](const std::size_t I) const {
//        static_assert(I < N, "out of bounds error");
//        return EArrayAccess{*this, I};
//    }

private:
    using arr_impl_t = std::array<element_t, N>; // equiv. to Value::type

    arr_impl_t m_initial_val{};
    bool m_initialised{false};

public:
    constexpr void initialise(const arr_impl_t& value) {
        m_initial_val = value;
        m_initialised = true;
    }
    constexpr bool initialised() const { return m_initialised; }
    constexpr const arr_impl_t& initial_val() const { return m_initial_val; }

    explicit constexpr ArrayBase() = default;
    explicit constexpr ArrayBase(const arr_impl_t& a, const std::string_view& name = "")
            : m_initial_val{a}, m_initialised{true}
            , Named{name} {}
};


template< typename TdElem
        , std::size_t N
        , bool is_const = false
//        , bool is_volatile = false
>
struct Array : public ArrayBase< Array< TdElem, N, is_const >
                                      , TdElem, N, is_const >
{
    using type = std::array<f_t<TdElem>, N>;
    using this_t = Array<TdElem, N, is_const>;
    using Value<this_t, type, is_const>::operator=;
    constexpr auto operator=(const this_t& rhs) const { return this->assign(rhs); };
    constexpr Array(this_t&&) = default;

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
        : public ArrayBase< ResidentArray< AddrT, TdElem, N, is_const >
                                                , TdElem, N, is_const >
        , public ResidentGlobal< AddrT
                               , std::array<f_t<TdElem>, N>
                               , is_const >
{
    using type = std::array<f_t<TdElem>, N>;
    using this_t = ResidentArray<AddrT, TdElem, N, is_const>;
    using Value<this_t, type, is_const>::operator=;
    constexpr auto operator=(const this_t& rhs) const { return this->assign(rhs); };
    constexpr ResidentArray(this_t&&) = default;

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

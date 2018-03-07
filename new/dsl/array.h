#pragma once

#include <array>

#include "dsl_base.h"
#include "dsl_type_traits.h"
#include "var.h"


namespace hetarch {
namespace dsl {


using dsl::i_t; // by some reasons CLion can't resolve it automatically.
using dsl::f_t; // by some reasons CLion can't resolve it automatically.


template<typename T, std::size_t N, bool is_const> class Array;

template< typename TdInd
        , typename TdElem
        , std::size_t N
        , bool is_const
//        , bool is_volatile
        , typename = typename std::enable_if_t< std::is_integral_v<i_t<TdInd>> >>
struct EArrayAccess : Value< EArrayAccess<TdInd, TdElem, N, is_const>
                           , f_t<TdElem>
                           , is_const
                           >
{
//    using arr_t = Array<T, N, is_const, is_volatile>;
    using arr_t = Array<TdElem, N, is_const>;
    using type = typename arr_t::element_t;

    const arr_t& arr;
    const TdInd ind;

    constexpr EArrayAccess(const arr_t& arr, TdInd&& ind) : arr{arr}, ind{std::forward<TdInd>(ind)} {}

    using this_t = EArrayAccess<TdInd, TdElem, N, is_const>;
    using Value<this_t, type, is_const>::operator=;
    constexpr auto operator=(const this_t& rhs) const { return this->assign(rhs); };

    IR_TRANSLATABLE
};


template< typename TdArr
        , typename TdElem
        , std::size_t N
        , bool is_const = false
//        , bool is_volatile = false
        , typename = typename std::enable_if_t<std::is_base_of_v<ValueBase, TdElem>>
>
class ArrayBase : public Value< TdArr, std::array<f_t<TdElem>, N>, is_const >
                , public Named
{
public:
    using element_t = f_t<TdElem>;
    using dsl_element_t = remove_cvref_t<TdElem>;
    static const bool const_q = is_const;
//    static const bool volatile_q = is_volatile;
    static const bool elt_const_q = dsl_element_t::const_q;
    static const bool elt_volatile_q = dsl_element_t::volatile_q;

    template<typename TdInd
            , typename = typename std::enable_if_t< std::is_integral_v<f_t<TdInd>> >>
    constexpr auto operator[](TdInd&& ind) const {
        return EArrayAccess<TdInd, TdElem, N, is_const>{
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

/*    template< typename ...TInit
            , typename = typename std::enable_if_t<
                    std::is_constructible_v<arr_impl_t, TInit...>
            >
    >
    explicit constexpr ArrayBase(TInit&&... xs)
            : m_initial_val{std::forward<TInit>(xs)...} {}*/

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

    using ArrayBase< this_t, TdElem, N, is_const >::ArrayBase;

    IR_TRANSLATABLE
};


}
}

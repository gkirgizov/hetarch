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
using struct_impl_t = std::tuple<Tds...>;


template<typename TdStruct, std::size_t I>
//struct EStructAccess : public typename TdStruct::dsl_element_t::typename
//                                                                base_t< EStructAccess<TdStruct, I> >
struct EStructAccess : public get_base_t< get_dsl_element_t<TdStruct>, EStructAccess<TdStruct, I> >
{
    const TdStruct& s;
    explicit constexpr EStructAccess(const TdStruct& s) : s{s} {}
    IR_TRANSLATABLE
};


template<typename TdStruct, bool is_const, bool is_volatile, typename ...Tds>
struct StructBase : public Value< TdStruct,
                                , struct_impl_t<Tds...>
                                , is_const
                                >
                  , public Named
{
    template<typename Td> using base_t = StructBase<Td, is_const, is_volatile, Tds...>;

    static_assert( is_val_v<Tds...>, "Expected DSL types as elements of DSL tuple!");

    using TdsTuple = std::tuple<Tds...>;
    template<std::size_t I> using dsl_element_t = remove_cvref_t< std::tuple_element<I, TdsTuple> >;
    template<std::size_t I> using element_t = f_t< dsl_element_t<I> >;
    template<std::size_t I> static const bool elt_const_q = dsl_element_t<I>::const_q;
    template<std::size_t I> static const bool elt_volatile_q = dsl_element_t<I>::volatile_q;
    static const bool const_q = is_const;
    static const bool volatile_q = is_volatile;

    // can reliably index Struct only by index (i.e. like struct.first, struct.second)
    template<std::size_t I>
    constexpr auto element() {
        static_assert(I >= 0 && I < sizeof...(Tds), "Struct element index is out of bounds");
        return EStructAccess<TdStruct, I>{ static_cast<const TdStruct&>(*this) };
    }

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
struct xStruct : public StructBase< Struct< is_const, is_volatile, Tds... >
                                         , is_const, is_volatile, Tds... >
{
    using type = struct_impl_t<Tds...>;
    using this_t = xStruct<is_const, is_volatile, Tds...>;
    using Value<this_t, type, is_const>::operator=;
    constexpr auto operator=(const this_t& rhs) const { return this->assign(rhs); };
    constexpr xStruct(this_t&&) = default;

    using StructBase< this_t, is_const, is_volatile, Tds... >::StructBase;

    IR_TRANSLATABLE
};

template<typename ...Tds> using Struct   = xStruct<false, false, Tds...>;
template<typename ...Tds> using cStruct  = xStruct<true,  false, Tds...>;
template<typename ...Tds> using vStruct  = xStruct<false, true,  Tds...>;
template<typename ...Tds> using cvStruct = xStruct<true,  true,  Tds...>;

}
}

#pragma once

#include <array>

#include "dsl_base.h"
#include "dsl_type_traits.h"
#include "var.h"


namespace hetarch {
namespace dsl {


using ::hetarch::dsl::i_t; // by some reasons CLion can't resolve it automatically.


template<typename T, std::size_t N, bool is_const, bool is_volatile> class Array;

template<typename TdInd, typename T, std::size_t N, bool is_const, bool is_volatile
        , typename = typename std::enable_if_t< std::is_integral_v<i_t<TdInd>> >>
struct EArrayAccess : Value<EArrayAccess<TdInd, T, N, is_const, is_volatile>, T, is_const> {
    using arr_t = Array<T, N, is_const, is_volatile>;

    const arr_t& arr;
    const TdInd ind;

    constexpr EArrayAccess(const arr_t& arr, TdInd&& ind) : arr{arr}, ind{std::forward<TdInd>(ind)} {}

    using this_t = EArrayAccess<TdInd, T, N, is_const, is_volatile>;
    using Value<this_t, T, is_const>::operator=;
    constexpr auto operator=(const this_t& rhs) const { return this->assign(rhs); };

    inline void toIR(IRTranslator &irTranslator) const { toIRImpl(*this, irTranslator); }
};


template<typename T, std::size_t N, bool is_const = std::is_const_v<T>, bool is_volatile = std::is_volatile_v<T>
//        , typename = typename std::enable_if_t<std::is_arithmetic_v<T>>
>
struct Array : public Var<std::array<T, N>, is_const, is_volatile> {
    using Var<std::array<T, N>, is_const, is_volatile>::Var;

    using value_t = T;

    // todo: allow indexing by const int
//    constexpr auto operator[](const std::size_t I) const {
//        static_assert(I < N, "out of bounds error");
//        return EArrayAccess{*this, I};
//    }

    template<typename TdInd
            , typename = typename std::enable_if_t< std::is_integral_v<i_t<TdInd>> >>
    constexpr auto operator[](TdInd&& ind) const {
        return EArrayAccess<TdInd, T, N, is_const, is_volatile>{*this, std::forward<TdInd>(ind)};
    }

//    inline void toIR(IRTranslator &irTranslator) const { toIRImpl(*this, irTranslator); }
};


}
}

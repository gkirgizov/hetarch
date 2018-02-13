#pragma once

#include <array>

#include "dsl_base.h"
#include "dsl_type_traits.h"
#include "var.h"


namespace hetarch {
namespace dsl {


using dsl::i_t; // by some reasons CLion can't resolve it automatically.


template<typename T, std::size_t N, bool is_const, bool is_volatile> class Array;

template<typename TdInd, typename T, std::size_t N, bool is_const, bool is_volatile
        , typename = typename std::enable_if_t< std::is_integral_v<i_t<TdInd>> >>
struct EArrayAccess : Value<EArrayAccess<TdInd, T, N, is_const, is_volatile>, is_const> {
//struct EArrayAccess : std::conditional_t<is_const,
//        Expr<T>, // don't enable assignment
//        Value<EArrayAccess<TdInd, T, N, is_const, is_volatile>, is_const> // derive from Value to allow assignment
//> {
    using type = T;
    using arr_t = Array<T, N, is_const, is_volatile>;

    const arr_t& arr;
    const TdInd ind;

    constexpr EArrayAccess(const arr_t& arr, TdInd&& ind) : arr{arr}, ind{std::forward<TdInd>(ind)} {}

    inline void toIR(IRTranslator &irTranslator) const override { toIRImpl(*this, irTranslator); }
};

// todo: alow using constexpr C++ values (and not constexpr too, too with InBounds)
//  todo: specialise translation using InBoundsGep
//template<typename T, std::size_t N, bool is_const, bool is_volatile>
//using EConstArrayAccess = EArrayAccess<std::size_t, T, N, is_const, is_volatile>;

template<typename T, std::size_t N, bool is_const = false, bool is_volatile = false>
struct Array : public Var<std::array<T, N>, is_const, is_volatile> {
    using Var<std::array<T, N>, is_const, is_volatile>::Var;

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

//    inline void toIR(IRTranslator &irTranslator) const override { toIRImpl(*this, irTranslator); }
};


}
}

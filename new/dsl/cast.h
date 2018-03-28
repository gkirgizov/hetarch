#pragma once

#include <llvm/IR/Instruction.h>

#include "dsl_base.h"
#include "dsl_type_traits.h"


namespace hetarch {
namespace dsl {

using dsl::i_t; // by some reasons CLion can't resolve it automatically.
using dsl::f_t; // by some reasons CLion can't resolve it automatically.


using Casts = llvm::Instruction::CastOps;

namespace {

template<typename To, typename From>
constexpr Casts get_llvm_cast() {
    if constexpr (std::is_floating_point<To>::value && std::is_floating_point<From>::value) {
        if constexpr (sizeof(To) < sizeof(From)) {
            return Casts::FPTrunc;
        } else {
            return Casts::FPExt;
        }
    } else if constexpr (std::is_floating_point<To>::value && std::is_integral<From>::value) {
        if constexpr (std::is_unsigned<From>::value) {
            return Casts::UIToFP;
        } else {
            return Casts::SIToFP;
        }
    } else if constexpr (std::is_integral<To>::value && std::is_floating_point<From>::value) {
        if constexpr (std::is_unsigned<To>::value) {
            return Casts::FPToUI;
        } else {
            return Casts::FPToSI;
        }
    } else if constexpr (std::is_integral<To>::value && std::is_integral<From>::value) {
        if constexpr (sizeof(To) < sizeof(From)) {
            return Casts::Trunc;
        } else {
            return Casts::SExt;
            // Casts::ZExt;
        }
    } else if constexpr (std::is_integral<To>::value && std::is_pointer<From>::value) {
        return Casts::PtrToInt;
    } else if constexpr (std::is_pointer<To>::value && std::is_integral<From>::value) {
        return Casts::IntToPtr;
    } else {
        static_assert(false && "Uncastable types:" && From::from_this_type && To::to_this_type);
    }

    // cases left
    // Casts::BitCast;
    // Casts::AddrSpaceCast;
};

}


template< typename TdTo
        , typename TdFrom
        , Casts Op = get_llvm_cast<i_t<TdTo>, i_t<TdFrom>>()
>
struct ECast : Expr<i_t<TdTo>> {
    using To = i_t<TdTo>;
    using From = i_t<TdFrom>;

    const TdFrom src;
    explicit constexpr ECast(TdFrom src) : src{src} {}
    IR_TRANSLATABLE
};

template<typename T> using bool_cast = ECast<Expr<bool>, T>;


template< typename TdTo, typename TdFrom >
inline constexpr auto Cast(TdFrom x) {
    // don't need cast when both are same or both are pointers
    if constexpr (std::is_same_v< i_t<TdTo>, i_t<TdFrom> >
                  || (std::is_pointer_v< i_t<TdTo> > && std::is_pointer_v< i_t<TdFrom> >))
    {
        return x;
    } else {
        return ECast<TdTo, TdFrom>{x};
    }
};


}
}



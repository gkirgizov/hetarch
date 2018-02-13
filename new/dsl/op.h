#pragma once


#include <llvm/IR/Instruction.h>

#include "dsl_base.h"
#include "dsl_type_traits.h"


namespace hetarch {
namespace dsl {

using dsl::i_t; // by some reasons CLion can't resolve it automatically.

using BinOps = llvm::Instruction::BinaryOps;
using Casts = llvm::Instruction::CastOps;


template<typename To, typename From, typename = std::enable_if<!std::is_same<To, From>::value>>
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
//        static_assert(false && "Uncastable types:");
    }

    // cases left
    // Casts::BitCast;
    // Casts::AddrSpaceCast;
};

//template<typename To> constexpr auto get_llvm_cast<To, To>() {};


template<typename To, typename TdFrom>
struct ECast : public Expr<To> {
    const TdFrom src;
    const Casts op;

    explicit constexpr ECast(TdFrom&& src) : src{std::forward<TdFrom>(src)}, op{get_llvm_cast<To, i_t<TdFrom>>()} {}

    inline void toIR(IRTranslator &irTranslator) const override { toIRImpl(*this, irTranslator); }
};

// empty cast for convenience
/*
template<typename To, typename TdFrom, typename = typename std::enable_if_t<std::is_same_v<To, i_t<TdFrom>::type>>>
struct ECast<To, To> : public Expr<To> {
    using type = To;

    const TdFrom src;

    explicit constexpr ECast(TdFrom&& src) : src{std::forward<TdFrom>(src)} {}

    inline void toIR(IRTranslator &irTranslator) const override { toIRImpl(*this, irTranslator); }
};
*/

template<typename T> using bool_cast = ECast<bool, T>;


template<BinOps bOp, typename TdLhs, typename TdRhs = TdLhs
        , typename = typename std::enable_if_t< is_ev_v<TdLhs, TdRhs> >
>
struct EBinOp : public Expr<i_t<TdLhs>> {
    const TdLhs lhs;
    const TdRhs rhs;

    constexpr EBinOp(TdLhs&& lhs, TdRhs&& rhs) : lhs{std::forward<TdLhs>(lhs)}, rhs{std::forward<TdRhs>(rhs)} {}

    inline void toIR(IRTranslator &irTranslator) const override { toIRImpl(*this, irTranslator); }
};

template<BinOps bOp, typename TdLhs, typename TdRhs = TdLhs
        , typename = typename std::enable_if_t< is_same_raw_v<bool, TdLhs, TdRhs> >
>
struct EBinOpLogical : public EBinOp<bOp, TdLhs, TdRhs> {
    using EBinOp<bOp, TdLhs, TdRhs>::EBinOp;

    inline void toIR(IRTranslator &irTranslator) const override { toIRImpl(*this, irTranslator); }
};



template<typename T1, typename T2 = T1
        , typename = typename std::enable_if_t< is_ev_v<T1, T2> >
>
constexpr auto operator+(T1&& lhs, T2&& rhs) {
    using cast_t = ECast<i_t<T1>, T2>;
    return EBinOp<std::is_floating_point<T1>::value ? BinOps::FAdd : BinOps::Add, T1, cast_t>{
            std::forward<T1>(lhs),
            cast_t{std::forward<T2>(rhs)}
    };
}


template<typename T1, typename T2 = T1
        , typename = typename std::enable_if_t< is_ev_v<T1, T2> >
>
constexpr auto operator-(T1&& lhs, T2&& rhs) {
    using cast_t = ECast<i_t<T1>, T2>;
    return EBinOp<std::is_floating_point<T1>::value ? BinOps::FSub: BinOps::Sub, T1, cast_t>{
            std::forward<T1>(lhs),
            cast_t{std::forward<T2>(rhs)}
    };
}

template<typename T1, typename T2 = T1
        , typename = typename std::enable_if_t< is_ev_v<T1, T2> >
>
constexpr auto operator*(T1&& lhs, T2&& rhs) {
    using cast_t = ECast<i_t<T1>, T2>;
    return EBinOp<std::is_floating_point<T1>::value ? BinOps::FMul: BinOps::Mul, T1, cast_t>{
            std::forward<T1>(lhs),
            cast_t{std::forward<T2>(rhs)}
    };
}

template<typename T1, typename T2 = T1
        , typename = typename std::enable_if_t< is_ev_v<T1, T2> >
>
constexpr auto operator&&(T1&& lhs, T2&& rhs) {
    return EBinOpLogical<BinOps::And, bool_cast<T1>, bool_cast<T2>>{
            bool_cast<T1>{std::forward<T1>(lhs)},
            bool_cast<T2>{std::forward<T2>(rhs)}
    };
};

template<typename T1, typename T2 = T1
        , typename = typename std::enable_if_t< is_ev_v<T1, T2> >
>
constexpr auto operator||(T1&& lhs, T2&& rhs) {
    return EBinOpLogical<BinOps::Or, bool_cast<T1>, bool_cast<T2>>{
            bool_cast<T1>{std::forward<T1>(lhs)},
            bool_cast<T2>{std::forward<T2>(rhs)}
    };
};

//template<typename T1>
//struct Operand : public Expr<T1> {
//    template<typename T2 = T1>
//    constexpr auto operator+=(const Expr<T2> &rhs) {
//        return EAssign<T1>{*this, EBinOp<BinOps::Add, T1, T2>{*this, rhs}};
//    }
//};


//void test() {
//    BinOps::FDiv;
//    BinOps::SDiv;
//    BinOps::UDiv;
//
//    BinOps::FRem;
//    BinOps::SRem;
//    BinOps::URem;
//
////    BinOps::And;
////    BinOps::Or;
//    BinOps::Xor;
//
//    BinOps::AShr;
//    BinOps::LShr;
//    BinOps::Shl;
//
//}

}
}

#pragma once


#include <llvm/IR/Instruction.h>

#include "dsl_base.h"
#include "dsl_type_traits.h"


namespace hetarch {
namespace dsl {

using dsl::i_t; // by some reasons CLion can't resolve it automatically.
using dsl::f_t; // by some reasons CLion can't resolve it automatically.

using BinOps = llvm::Instruction::BinaryOps;
using Casts = llvm::Instruction::CastOps;
using OtherOps = llvm::Instruction::OtherOps;
using Predicate = llvm::CmpInst::Predicate;


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
//        static_assert(false && "Uncastable types:");
    }

    // cases left
    // Casts::BitCast;
    // Casts::AddrSpaceCast;
};

//template<typename To> constexpr auto get_llvm_cast<To, To>() {};


template<typename TdTo, typename TdFrom, Casts Op = get_llvm_cast<i_t<TdTo>, i_t<TdFrom>>()>
struct ECast : public Expr<i_t<TdTo>> {
    using To = i_t<TdTo>;
    using From = i_t<TdFrom>;

    const TdFrom src;
    explicit constexpr ECast(TdFrom&& src) : src{std::forward<TdFrom>(src)} {}

    inline void toIR(IRTranslator &irTranslator) const override { toIRImpl(*this, irTranslator); }
};


using dsl::Expr; // wtf CLion doesn't see exactly here
template<typename T> using bool_cast = ECast<Expr<bool>, T>;


template<BinOps bOp, typename TdLhs, typename TdRhs = TdLhs
        , typename = typename std::enable_if_t< is_ev_v<TdLhs, TdRhs> >
>
struct EBinOp : public Expr<f_t<TdLhs>> {
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


template<OtherOps Op, Predicate P, typename TdLhs, typename TdRhs
        , typename = typename std::enable_if_t< is_ev_v<TdLhs, TdRhs> >
>
struct EBinCmp : public Expr<bool> {
//    constexpr static OtherOps opKind{P < 32 ? OtherOps::FCmp : OtherOps::ICmp};
    const TdLhs lhs;
    const TdRhs rhs;

    constexpr EBinCmp(TdLhs&& lhs, TdRhs&& rhs) : lhs{std::forward<TdLhs>(lhs)}, rhs{std::forward<TdRhs>(rhs)} {}
    inline void toIR(IRTranslator &irTranslator) const override { toIRImpl(*this, irTranslator); }
};

template<Predicate P, typename TdLhs, typename TdRhs>
using EBinFCmp = EBinCmp<OtherOps::FCmp, P, TdLhs, TdRhs>;
template<Predicate P, typename TdLhs, typename TdRhs>
using EBinICmp = EBinCmp<OtherOps::ICmp, P, TdLhs, TdRhs>;

/// Binary plus
template<typename T1, typename T2 = T1
        , typename = typename std::enable_if_t< is_ev_v<T1, T2> >
>
constexpr auto operator+(T1&& lhs, T2&& rhs) {
    using cast_t = ECast<T1, T2>;
    return EBinOp<(std::is_floating_point_v<i_t<T1>> ? BinOps::FAdd : BinOps::Add), T1, cast_t>{
            std::forward<T1>(lhs),
            cast_t{std::forward<T2>(rhs)}
    };
}

/// Unary plus
template<typename T1, typename = typename std::enable_if_t< is_ev_v<T1> >>
constexpr auto operator+(T1&& lhs) {
    using const_t = DSLConst<i_t<T1>>;
    return EBinOp<(std::is_floating_point_v<i_t<T1>> ? BinOps::FAdd: BinOps::Add), const_t, T1>{
            const_t{0},
            std::forward<T1>(lhs)
    };
}


/// Binary minus
template<typename T1, typename T2 = T1
        , typename = typename std::enable_if_t< is_ev_v<T1, T2> >
>
constexpr auto operator-(T1&& lhs, T2&& rhs) {
    using cast_t = ECast<T1, T2>;
    return EBinOp<(std::is_floating_point_v<i_t<T1>> ? BinOps::FSub: BinOps::Sub), T1, cast_t>{
            std::forward<T1>(lhs),
            cast_t{std::forward<T2>(rhs)}
    };
}

/// Unary minus
template<typename T1, typename = typename std::enable_if_t< is_ev_v<T1> >>
constexpr auto operator-(T1&& lhs) {
    using const_t = DSLConst<i_t<T1>>;
    return EBinOp<(std::is_floating_point_v<i_t<T1>> ? BinOps::FSub: BinOps::Sub), const_t, T1>{
            const_t{0},
            std::forward<T1>(lhs)
    };
}


/// Multiplication
template<typename T1, typename T2 = T1
        , typename = typename std::enable_if_t< is_ev_v<T1, T2> >
>
constexpr auto operator*(T1&& lhs, T2&& rhs) {
    using cast_t = ECast<T1, T2>;
    return EBinOp<(std::is_floating_point<T1>::value ? BinOps::FMul: BinOps::Mul), T1, cast_t>{
            std::forward<T1>(lhs),
            cast_t{std::forward<T2>(rhs)}
    };
}



// todo: type conversions in bitwise operations?

/// Bitwise And
template<typename T1, typename T2
        , typename = typename std::enable_if_t< is_ev_v<T1, T2> >
>
constexpr auto operator&(T1&& lhs, T2&& rhs) {
    static_assert(std::is_integral_v<i_t<T1>> && std::is_integral_v<i_t<T2>>, "Bitwise operations expect integral types!");
    return EBinOp<BinOps::And, T1, T2>{
            std::forward<T1>(lhs),
            std::forward<T2>(rhs)
    };
};

/// Bitwise Or
template<typename T1, typename T2
        , typename = typename std::enable_if_t< is_ev_v<T1, T2> >
>
constexpr auto operator|(T1&& lhs, T2&& rhs) {
    static_assert(std::is_integral_v<i_t<T1>> && std::is_integral_v<i_t<T2>>, "Bitwise operations expect integral types!");
    return EBinOp<BinOps::Or, T1, T2>{
            std::forward<T1>(lhs),
            std::forward<T2>(rhs)
    };
};

/// Bitwise Xor
template<typename T1, typename T2
        , typename = typename std::enable_if_t< is_ev_v<T1, T2> >
>
constexpr auto operator^(T1&& lhs, T2&& rhs) {
    static_assert(std::is_integral_v<i_t<T1>> && std::is_integral_v<i_t<T2>>, "Bitwise operations expect integral types!");
    return EBinOp<BinOps::Xor, T1, T2>{
            std::forward<T1>(lhs),
            std::forward<T2>(rhs)
    };
};



/// Logical Not
template<typename T1, typename = typename std::enable_if_t< is_ev_v<T1> >>
constexpr auto operator!(T1&& lhs) {
    using const_t = DSLConst<bool>;
    return EBinOpLogical<BinOps::Xor, bool_cast<T1>, const_t>{
            bool_cast<T1>{std::forward<T1>(lhs)},
            const_t{true}
    };
};

/// Logical And
template<typename T1, typename T2 = T1
        , typename = typename std::enable_if_t< is_ev_v<T1, T2> >
>
constexpr auto operator&&(T1&& lhs, T2&& rhs) {
    return EBinOpLogical<BinOps::And, bool_cast<T1>, bool_cast<T2>>{
            bool_cast<T1>{std::forward<T1>(lhs)},
            bool_cast<T2>{std::forward<T2>(rhs)}
    };
};

/// Logical Or
template<typename T1, typename T2 = T1
        , typename = typename std::enable_if_t< is_ev_v<T1, T2> >
>
constexpr auto operator||(T1&& lhs, T2&& rhs) {
    return EBinOpLogical<BinOps::Or, bool_cast<T1>, bool_cast<T2>>{
            bool_cast<T1>{std::forward<T1>(lhs)},
            bool_cast<T2>{std::forward<T2>(rhs)}
    };
};


template<typename T1, typename T2
        , typename = typename std::enable_if_t< is_ev_v<T1, T2> >
>
constexpr auto operator==(T1&& lhs, T2&& rhs) {
    using t1 = i_t<T1>;
    using t2 = i_t<T2>;
    static_assert(std::is_arithmetic_v<t1> && std::is_arithmetic_v<t2>, "Expected arithmetic types in comparison");

    constexpr bool t1_fp = std::is_floating_point_v<t1>;
    constexpr bool t2_fp = std::is_floating_point_v<t2>;
//    constexpr bool t1_u = std::is_unsigned_v<t1>;
//    constexpr bool t2_u = std::is_unsigned_v<t2>;

    // if one of the operands is float, then convert other to the float
    if constexpr (t1_fp && t2_fp) {
        return EBinFCmp<Predicate::FCMP_OEQ, T1, T2>{
                std::forward<T1>(lhs),
                std::forward<T2>(rhs)
        };
    // todo: emit warning: losing precision
    } else if constexpr (t1_fp && !t2_fp) {
        using cast_t = ECast<T1, T2>;
        return EBinFCmp<Predicate::FCMP_OEQ, T1, cast_t>{
                std::forward<T1>(lhs),
                cast_t{std::forward<T2>(rhs)}
        };
    } else if constexpr (!t1_fp && t2_fp) {
        using cast_t = ECast<T2, T1>;
        return EBinFCmp<Predicate::FCMP_OEQ, cast_t, T2>{
                cast_t{std::forward<T1>(lhs)},
                std::forward<T2>(rhs)
        };
    } else {
        // todo: do i need cast if not the same types?
        return EBinICmp<Predicate::ICMP_EQ, T1, T2>{
                std::forward<T1>(lhs),
                std::forward<T2>(rhs)
        };
    }
};


template<typename T1, typename T2
        , typename = typename std::enable_if_t< is_ev_v<T1, T2> >
>
constexpr auto operator<(T1&& lhs, T2&& rhs) {
    using t1 = i_t<T1>;
    using t2 = i_t<T2>;
    static_assert(std::is_arithmetic_v<t1> && std::is_arithmetic_v<t2>, "Expected arithmetic types in comparison");

    constexpr bool t1_fp = std::is_floating_point_v<t1>;
    constexpr bool t2_fp = std::is_floating_point_v<t2>;
    constexpr bool t1_u = std::is_unsigned_v<t1>;
    constexpr bool t2_u = std::is_unsigned_v<t2>;

    if constexpr (t1_fp && t2_fp) {
        return EBinFCmp<Predicate::FCMP_OLT, T1, T2>{
                std::forward<T1>(lhs),
                std::forward<T2>(rhs)
        };
    // todo: emit warning: losing precision
    // if one of the operands is float, then convert other to the float
    } else if constexpr (t1_fp) {
        using cast_t = ECast<T1, T2>;
        return EBinFCmp<Predicate::FCMP_OLT, T1, cast_t>{
                std::forward<T1>(lhs),
                cast_t{std::forward<T2>(rhs)}
        };
    } else if constexpr (t2_fp) {
        using cast_t = ECast<T2, T1>;
        return EBinFCmp<Predicate::FCMP_OLT, cast_t, T2>{
                cast_t{std::forward<T1>(lhs)},
                std::forward<T2>(rhs)
        };
    } else if constexpr (t1_u && t2_u) {
        return EBinICmp<Predicate::ICMP_ULT, T1, T2>{
                std::forward<T1>(lhs),
                std::forward<T2>(rhs)
        };
    // if one of the operand is signed, then convert other to signed
    } else if constexpr (!t1_u) {
        using cast_t = ECast<T1, T2>;
        return EBinICmp<Predicate::ICMP_SLT, T1, cast_t>{
                std::forward<T1>(lhs),
                cast_t{std::forward<T2>(rhs)}
        };
    } else if constexpr (!t2_u) { // one of the types is signed
        using cast_t = ECast<T2, T1>;
        return EBinICmp<Predicate::ICMP_SLT, cast_t, T2>{
                cast_t{std::forward<T1>(lhs)},
                std::forward<T2>(rhs)
        };
    }
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
////    BinOps::Xor;
//
//    BinOps::AShr;
//    BinOps::LShr;
//    BinOps::Shl;
//
//}

}
}

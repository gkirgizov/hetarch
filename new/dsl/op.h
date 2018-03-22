#pragma once

#include <llvm/IR/Instruction.h>

#include "dsl_base.h"
#include "dsl_type_traits.h"
#include "value.h"


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


using dsl::Expr; // wtf CLion doesn't see exactly here
template<typename T> using bool_cast = ECast<Expr<bool>, T>;


template<BinOps bOp, typename TdLhs, typename TdRhs = TdLhs
        , typename = typename std::enable_if_t< is_ev_v<TdLhs, TdRhs> >
>
struct EBinOp : Expr<f_t<TdLhs>> {
    const TdLhs lhs;
    const TdRhs rhs;
    constexpr EBinOp(TdLhs lhs, TdRhs rhs) : lhs{lhs}, rhs{rhs} {}
    IR_TRANSLATABLE
};

template<BinOps bOp, typename TdLhs, typename TdRhs = TdLhs
        , typename = typename std::enable_if_t< is_same_raw_v<bool, TdLhs, TdRhs> >
>
struct EBinOpLogical : EBinOp<bOp, TdLhs, TdRhs> {
    using EBinOp<bOp, TdLhs, TdRhs>::EBinOp;
    IR_TRANSLATABLE
};

// todo: does it inherit const_q from TdPtr?
template<BinOps Op, typename TdPtr, typename Td
        , typename = typename std::enable_if_t< std::is_pointer_v<f_t<TdPtr>> >
>
struct EBinPtrOp : get_base_t< TdPtr, EBinPtrOp<Op, TdPtr, Td> >
{
    using type = f_t<TdPtr>;
    using this_t = EBinPtrOp<Op, TdPtr, Td>;
    using Value<this_t, type, std::remove_reference_t<TdPtr>::const_q>::operator=;
    constexpr auto operator=(const this_t& rhs) const { return this->assign(rhs); };
    constexpr EBinPtrOp(this_t&&) = default;
    constexpr EBinPtrOp(const this_t&) = default;

    const TdPtr ptr;
    const Td operand;
    constexpr EBinPtrOp(TdPtr ptr, Td operand)
            : ptr{ptr}, operand{operand} {}
    IR_TRANSLATABLE
};


template<OtherOps Op, Predicate P, typename TdLhs, typename TdRhs
        , typename = typename std::enable_if_t< is_ev_v<TdLhs, TdRhs> >
>
struct EBinCmp : Expr<bool> {
    const TdLhs lhs;
    const TdRhs rhs;
    constexpr EBinCmp(TdLhs lhs, TdRhs rhs) : lhs{lhs}, rhs{rhs} {}
    IR_TRANSLATABLE
};

template<Predicate P, typename TdLhs, typename TdRhs>
using EBinFCmp = EBinCmp<OtherOps::FCmp, P, TdLhs, TdRhs>;
template<Predicate P, typename TdLhs, typename TdRhs>
using EBinICmp = EBinCmp<OtherOps::ICmp, P, TdLhs, TdRhs>;


#define UNARY_OP(SYM, OP) \
template<typename T1, typename = typename std::enable_if_t< is_ev_v<T1> >> \
constexpr auto operator SYM (T1 lhs) { \
    using t1 = i_t<T1>; \
    using const_t = DSLConst<t1>; \
    constexpr bool is_fp = std::is_floating_point_v<t1>; \
    return EBinOp<(is_fp ? BinOps::F##OP: BinOps:: OP), const_t, T1>{ \
            const_t{0}, \
            lhs \
    }; \
}

UNARY_OP(+, Add)
UNARY_OP(-, Sub)


// todo: implement ptrdiff & handle subtract ptr from val

#define ADDITIVE_OP(SYM, F_OP, I_OP) \
template<typename T1, typename T2 \
        , typename = typename std::enable_if_t< is_ev_v<T1, T2> > \
> \
constexpr auto operator SYM (T1 lhs, T2 rhs) { \
    using t1 = f_t<T1>; \
    using t2 = f_t<T2>; \
    if constexpr (std::is_pointer_v<t1>) { \
        if constexpr (std::is_pointer_v<t2>) { \
            static_assert(true, "ptrdiff isn't implemented yet"); \
            \
        } else { \
            static_assert(std::is_integral_v<t2>); \
            return EBinPtrOp<BinOps:: I_OP , T1, T2>{ \
                    lhs, \
                    rhs \
            }; \
        } \
    } else if constexpr (std::is_pointer_v<t2>) { \
        static_assert(std::is_integral_v<t1>); \
        assert( BinOps:: I_OP != BinOps::Sub && "Substracting pointer from value is disallowed!"); \
        return EBinPtrOp<BinOps:: I_OP , T2, T1>{ \
                rhs, \
                lhs \
        }; \
    } else if constexpr (std::is_floating_point_v<t1>) { \
        using cast_t = ECast<T1, T2>; \
        return EBinOp<BinOps:: F_OP , T1, cast_t>{ \
            lhs, \
            cast_t{rhs} \
        }; \
    } else if constexpr (std::is_floating_point_v<t2>) { \
        using cast_t = ECast<T2, T1>; \
        return EBinOp<BinOps:: F_OP , cast_t, T2>{ \
            cast_t{lhs}, \
            rhs \
        }; \
    } else { \
        using cast_t = ECast<T1, T2>; \
        return EBinOp<BinOps:: I_OP , T1, cast_t>{ \
            lhs, \
            cast_t{rhs} \
        }; \
    } \
}

#define MULTIPLICATIVE_OP(SYM, F_OP, S_OP, U_OP) \
template<typename T1, typename T2 \
        , typename = typename std::enable_if_t< is_ev_v<T1, T2> > \
> \
constexpr auto operator SYM (T1 lhs, T2 rhs) { \
    using t1 = f_t<T1>; \
    using t2 = f_t<T2>; \
    if constexpr (std::is_floating_point_v<t1>) { \
        using cast_t = ECast<T1, T2>; \
        return EBinOp<BinOps:: F_OP , T1, cast_t>{ \
            lhs, \
            cast_t{rhs} \
        }; \
    } else if constexpr (std::is_floating_point_v<t2>) { \
        using cast_t = ECast<T2, T1>; \
        return EBinOp<BinOps:: F_OP , cast_t, T2>{ \
            cast_t{lhs}, \
            rhs \
        }; \
    } else { \
        using cast_t = ECast<T1, T2>; \
        return EBinOp<BinOps:: S_OP , T1, cast_t>{ \
            lhs, \
            cast_t{rhs} \
        }; \
    } \
}


ADDITIVE_OP(+, FAdd, Add)
ADDITIVE_OP(-, FSub, Sub)
MULTIPLICATIVE_OP(*, FMul, Mul, Mul)
// todo: ensure these operations
//MULTIPLICATIVE_OP(/, FDiv, SDiv, UDiv)
//MULTIPLICATIVE_OP(%, FRem, SRem, URem)


#define BITWISE_OP(SYM, OP) \
template<typename T1, typename T2 \
        , typename = typename std::enable_if_t< is_ev_v<T1, T2> > \
> \
constexpr auto operator SYM (T1 lhs, T2 rhs) { \
    using t1 = f_t<T1>; \
    using t2 = f_t<T2>; \
    static_assert(std::is_integral_v<t1> && std::is_integral_v<t2>, "Bitwise operations expect integral types!"); \
    \
    if constexpr (sizeof(t1) > sizeof(t2)) { \
        using cast_t = ECast<T1, T2>; \
        return EBinOp<BinOps:: OP , T1, cast_t>{ \
                lhs, \
                cast_t{rhs} \
        }; \
    } else if constexpr (sizeof(t1) < sizeof(t2)) { \
        using cast_t = ECast<T2, T1>; \
        return EBinOp<BinOps:: OP , cast_t, T2>{ \
                cast_t{lhs}, \
                rhs \
        }; \
    } else { \
        return EBinOp<BinOps:: OP , T1, T2>{ \
                lhs, \
                rhs \
        }; \
    }\
};


BITWISE_OP(&, And)
BITWISE_OP(|, Or)
BITWISE_OP(^, Xor)

/// Bitwise Neg
template<typename T1, typename = typename std::enable_if_t< is_ev_v<T1> >>
constexpr auto operator~ (T1 x) {
    using t1 = f_t<T1>;
    static_assert(std::is_arithmetic_v<t1>, "Bitwise negation expects arithmetic type!");
    using zero_t = DSLConst<i_t<T1>>;
    if constexpr (std::is_integral_v<t1>) {
        return EBinOp<BinOps::Sub, zero_t, T1>{ zero_t{0}, x };
    } else if constexpr (std::is_floating_point_v<f_t<T1>>) {
        return EBinOp<BinOps::FSub, zero_t, T1>{ zero_t{-0.0}, x };
    }
};


/// Logical Not
template<typename T1, typename = typename std::enable_if_t< is_ev_v<T1> >>
constexpr auto operator!(T1 lhs) {
    using const_t = DSLConst<bool>;
    return EBinOpLogical<BinOps::Xor, bool_cast<T1>, const_t>{
            bool_cast<T1>{lhs},
            const_t{true}
    };
};

/// Logical And
template<typename T1, typename T2 = T1
        , typename = typename std::enable_if_t< is_ev_v<T1, T2> >
>
constexpr auto operator&&(T1 lhs, T2 rhs) {
    return EBinOpLogical<BinOps::And, bool_cast<T1>, bool_cast<T2>>{
            bool_cast<T1>{lhs},
            bool_cast<T2>{rhs}
    };
};

/// Logical Or
template<typename T1, typename T2 = T1
        , typename = typename std::enable_if_t< is_ev_v<T1, T2> >
>
constexpr auto operator||(T1 lhs, T2 rhs) {
    return EBinOpLogical<BinOps::Or, bool_cast<T1>, bool_cast<T2>>{
            bool_cast<T1>{lhs},
            bool_cast<T2>{rhs}
    };
};


// todo cmp pointers
// todo: emit warning: losing precision when cast float to int
// todo: handle unordered floats comparisons
#define CMP_OPERATOR(SYM, F_OP, S_OP, U_OP) \
template<typename T1, typename T2, typename = typename std::enable_if_t< is_ev_v<T1, T2> >> \
constexpr auto operator SYM (T1 lhs, T2 rhs) { \
    using t1 = i_t<T1>; \
    using t2 = i_t<T2>; \
    static_assert(std::is_scalar_v<t1> && std::is_scalar_v<t2>, "Expected scalar types in comparison!"); \
 \
    constexpr bool t1_fp = std::is_floating_point_v<t1>; \
    constexpr bool t2_fp = std::is_floating_point_v<t2>; \
    constexpr bool t1_u = std::is_unsigned_v<t1>; \
    constexpr bool t2_u = std::is_unsigned_v<t2>; \
 \
    if constexpr (t1_fp && t2_fp) { \
        return EBinFCmp<Predicate::FCMP_O##F_OP, T1, T2>{ \
                lhs, \
                rhs \
        }; \
    } else if constexpr (t1_fp) { \
        using cast_t = ECast<T1, T2>; \
        return EBinFCmp<Predicate::FCMP_O##F_OP, T1, cast_t>{ \
                lhs, \
                cast_t{rhs} \
        }; \
    } else if constexpr (t2_fp) { \
        using cast_t = ECast<T2, T1>; \
        return EBinFCmp<Predicate::FCMP_O##F_OP, cast_t, T2>{ \
                cast_t{lhs}, \
                rhs \
        }; \
    } else if constexpr (t1_u && t2_u) { \
        return EBinICmp<Predicate::ICMP_##U_OP, T1, T2>{ \
                lhs, \
                rhs \
        }; \
    } else if constexpr (!t1_u) { \
        using cast_t = ECast<T1, T2>; \
        return EBinICmp<Predicate::ICMP_##S_OP, T1, cast_t>{ \
                lhs, \
                cast_t{rhs} \
        }; \
    } else if constexpr (!t2_u) { \
        using cast_t = ECast<T2, T1>; \
        return EBinICmp<Predicate::ICMP_##S_OP, cast_t, T2>{ \
                cast_t{lhs}, \
                rhs \
        }; \
    } \
};

CMP_OPERATOR(==, EQ, EQ,  EQ)
CMP_OPERATOR(!=, EQ, EQ,  EQ)
CMP_OPERATOR(<,  LT, ULT, SLT)
CMP_OPERATOR(<=, LE, ULE, SLE)
CMP_OPERATOR(>,  GT, UGT, SGT)
CMP_OPERATOR(>=, GE, UGE, SGE)


// todo: bitshift


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

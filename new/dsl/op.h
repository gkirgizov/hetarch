#pragma once


#include <type_traits>

#include <llvm/IR/Instruction.h>

#include "dsl_base.h"


namespace hetarch {
namespace dsl {


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


template<typename To, typename From>
struct ECast : public Expr<To> {
    const Expr<From> &src;
    const Casts op;

    explicit constexpr ECast(const Expr<From> &src) : src{src}, op{get_llvm_cast<To, From>()} {}

    inline void toIR(IRTranslator &irTranslator) const override { toIRImpl(*this, irTranslator); }
};

// empty cast for convenience
template<typename To>
struct ECast<To, To> : public Expr<To> {
    const Expr<To> &src;
    explicit constexpr ECast(const Expr<To> &src) : src{src} {}

    inline void toIR(IRTranslator &irTranslator) const override { toIRImpl(*this, irTranslator); }

    ~ECast() {
        std::cout << "called " << typeid(*this).name() << " destructor" << std::endl;
    }
};

//template<typename To, typename From>
//constexpr auto cast(const Expr<From> &src) { return ECast<To, From>{src}; };


template<BinOps bOp, typename T1, typename T2 = T1>
struct EBinOp : public Expr<T1> { // 'preference' for the type of left operand
    const Expr<T1> &lhs;
    const Expr<T2> &rhs;

    constexpr EBinOp(const Expr<T1> &lhs, const Expr<T2> &rhs) : lhs{lhs}, rhs{rhs} {}
//    constexpr EBinOp(const Expr<T1> &lhs, Expr<T2> &&rhs) : lhs{lhs}, rhs{rhs} {}
//    constexpr EBinOp(const Expr<T1> &lhs, Expr<T2> &&rhs) : lhs{lhs}, rhs{std::move(rhs)} {}

    void toIR(IRTranslator &irTranslator) const override { toIRImpl(*this, irTranslator); }
};

template<BinOps bOp>
struct EBinOpLogical : public EBinOp<bOp, bool> {
    using EBinOp<bOp, bool>::EBinOp;

    inline void toIR(IRTranslator &irTranslator) const override { toIRImpl(*this, irTranslator); }
};



template<typename T1, typename T2 = T1>
constexpr auto operator+(const Expr<T1> &lhs, const Expr<T2> &rhs) {
//    return EBinOp<BinOps::Add, T1>{lhs, rhs};
//        return EBinOp<BinOps::Add, T1>{lhs, cast<T1, T2>(rhs)};
    if constexpr (std::is_floating_point<T1>::value) {
        return EBinOp<BinOps::FAdd, T1>{lhs, ECast<T1, T2>{rhs}};
    } else {
        return EBinOp<BinOps::Add, T1>{lhs, ECast<T1, T2>{rhs}};
    }
}

template<typename T1, typename T2 = T1>
constexpr auto operator-(const Expr<T1> &lhs, const Expr<T2> &rhs) {
    if constexpr (std::is_floating_point<T1>::value) {
        return EBinOp<BinOps::FSub, T1>{lhs, ECast<T1, T2>{rhs}};
    } else {
        return EBinOp<BinOps::Sub, T1>{lhs, ECast<T1, T2>{rhs}};
    }
}

template<typename T1, typename T2 = T1>
constexpr auto operator*(const Expr<T1> &lhs, const Expr<T2> &rhs) {
    if constexpr (std::is_floating_point<T1>::value) {
        return EBinOp<BinOps::FMul, T1>{lhs, ECast<T1, T2>{rhs}};
    } else {
        return EBinOp<BinOps::Mul, T1>{lhs, ECast<T1, T2>{rhs}};
    }
}

template<typename T1, typename T2 = T1>
constexpr auto operator&&(const Expr<T1> &lhs, const Expr<T2> &rhs) {
    return EBinOpLogical<BinOps::And>{ECast<bool, T1>{lhs}, ECast<bool, T2>{rhs}};
};
template<typename T1, typename T2 = T1>
constexpr auto operator||(const Expr<T1> &lhs, const Expr<T2> &rhs) {
    return EBinOpLogical<BinOps::Or>{ECast<bool, T1>{lhs}, ECast<bool, T2>{rhs}};
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

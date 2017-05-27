#ifndef HETARCH_DSL_H
#define HETARCH_DSL_H 1

#include <vector>
#include <memory>
#include <map>
#include <type_traits>
#include <iostream>
#include <cassert>

//template<typename T> using eptr = std::shared_ptr<T>;

#include "MemMgr.h"
#include "dslutils.h"
#include "cg.h"
#include "conn.h"

namespace hetarch {
namespace dsl {

#define DSL_STRUCT(name) struct name: RStruct<name>

#define CLONE_OVERRIDE virtual ENonGeneric *clone() const override
#define VALREG_OVERRIDE(DUMPER_VAR) virtual std::string valreg(IRDumper &DUMPER_VAR) const override
#define PTRREG_OVERRIDE(DUMPER_VAR) virtual std::string ptrreg(IRDumper &DUMPER_VAR) const override
#define TOIR_OVERRIDE(DUMPER_VAR) virtual void toIR(IRDumper &DUMPER_VAR) const override

template<unsigned A> unsigned widenToAlignment(unsigned size) {
    if ((size & (A - 1)) == 0) { return size; }
    return ((size & (~(A - 1))) + A);
}

class IRDumper;
struct ENonGeneric;
template<typename T> struct RValueBase;
template<typename T, class Integral = void> struct LValueBase;
template<typename T> struct LValue;
template<typename T> struct RValue;

template<typename T> struct E;
template<typename T> struct ELocalVar;
template<typename T> struct EGlobalVar;
template<typename T> struct EParam;
template<typename T> struct EConst;
template<typename T> struct EAssign;
template<typename T> struct ELValue;
template<typename T, typename TR> struct EBin;
template<typename T> struct EGetAddr;
template<typename T> struct EDeref;
template<typename T> struct EUnaryMinus;
template<typename T, typename TR> struct EIRConv;
template<typename T, typename R> struct ESeq;
struct ELogicalAnd;
template<typename AddrT, typename Ret, typename ... Ts> struct ECall;
template<typename AddrT, typename T> struct Global;

template<typename AddrT> struct FunctionBase;
template<typename AddrT, typename T> struct Function;

enum __E_LLVM_IR_CONV {
    __E_LLVM_IR_CONV__pointer,
    __E_LLVM_IR_CONV_sext,
    __E_LLVM_IR_CONV_zext,
    __E_LLVM_IR_CONV_trunc,
    __E_LLVM_IR_CONV__to_pointer,
    __E_LLVM_IR_CONV__from_pointer,
};

struct STATISTICS {
    static int RValueBaseNonGeneric_moveCtor;
    static int RValueBaseNonGeneric_copyCtor;
    static int RValueBaseNonGeneric_dtor;
    static int ParamT_moveCtor;
    static void dumpStats();
};

template<typename Ret>
struct ICloneable {
    virtual Ret *clone() const = 0;
};

struct ENonGeneric {
    virtual ENonGeneric *clone() const = 0;
    virtual std::string ptrreg(IRDumper &dumper) const = 0;
    virtual std::string valreg(IRDumper &dumper) const = 0;
    virtual void toIR(IRDumper &dumper) const = 0;
};

struct GlobalBase {
    HETARCH_TARGET_ADDRT addr;
    std::string name;
    GlobalBase(): name(""), addr(0){}
    GlobalBase(HETARCH_TARGET_ADDRT _addr): name(""), addr(_addr) {}
    GlobalBase(const char* _name, HETARCH_TARGET_ADDRT _addr): name(_name), addr(_addr) {}
    GlobalBase(const std::string &_name, HETARCH_TARGET_ADDRT _addr): name(_name), addr(_addr) {}
};

#include "IRDumper.h"

template<typename AddrT, typename T> std::string convertToIR(
        const RValueBase<T> &e, std::vector<FunctionBase<AddrT>*> &inlines)
{
    IRDumper dumper;
    dumper.indent++;
    if (std::is_same<T, void>::value) {
        e.toIR(dumper);
        dumper.appendln("ret void");
    }
    else {
        std::string retReg = e.valreg(dumper);
        dumper.appendln("ret " + IRTypename<T>() + " " + retReg);
    }
    return dumper.getText(inlines);
}

enum Operations {
    OP_PLUS, OP_MINUS, OP_MULT, OP_DIV, OP_MOD,
    OP_BIT_NOT, OP_BIT_AND, OP_BIT_OR, OP_BIT_XOR,
    OP_NOT, OP_AND, OP_OR,
    OP_LT, OP_GT, OP_LE, OP_GE, OP_EQ, OP_NE,
};

template<typename T> RValue<T> RValueFromE(ENonGeneric *e) {
    return RValue<T>(e, false);
}

template<typename T> LValue<T> LValueFromE(ENonGeneric *e) {
    return LValue<T>(e, false);
}

template<typename T> struct Param: public RValue<T> {
    Param(): RValue<T>(new EParam<T>("", IRDumper::nextIndStatic()), false) {}
    Param(const std::string &name): RValue<T>(new EParam<T>(name, 0), false) {}
    Param(const Param<T> &src): RValue<T>(src.e ? src.e->clone() : nullptr, false) {}
    Param(Param<T> &&src): RValue<T>(src.e, false) { src.e = nullptr; STATISTICS::ParamT_moveCtor++; }
    std::string ToString() { return reinterpret_cast<EParam<T>* >(this->e)->GetVarname(); }
};

struct RValueBaseNonGeneric {
    RValueBaseNonGeneric(const RValueBaseNonGeneric &src): e(src.e ? src.e->clone() : nullptr) {
        STATISTICS::RValueBaseNonGeneric_copyCtor++;
    }
    RValueBaseNonGeneric(RValueBaseNonGeneric &&src): e(src.e) {
        //std::cout << "move RValueBaseNonGeneric" << std::endl;
        STATISTICS::RValueBaseNonGeneric_moveCtor++;
        src.e = nullptr;
    }
    virtual std::string ptrreg(IRDumper &dumper) const { return e ? e->ptrreg(dumper) : ""; }
    virtual std::string valreg(IRDumper &dumper) const { return e ? e->valreg(dumper) : ""; }
    virtual void toIR(IRDumper &dumper) const { if (e) { e->toIR(dumper); } }
    virtual ~RValueBaseNonGeneric() {
        //std::cout << "~RValueBaseNonGeneric" << std::endl;
        STATISTICS::RValueBaseNonGeneric_dtor++;
        if (e) { delete e; }
    }
protected:
    ENonGeneric *e;
    RValueBaseNonGeneric(ENonGeneric *_e, bool _unused): e(_e) {}
};

template<typename T>
struct RValueBase: public RValueBaseNonGeneric {
    RValueBase(): RValueBaseNonGeneric(nullptr, false) {}
    RValueBase(const RValueBase<T> &src): RValueBaseNonGeneric(src) {}
    RValueBase(RValueBase<T> &&src):
            RValueBaseNonGeneric(std::forward<RValueBaseNonGeneric>(static_cast<RValueBaseNonGeneric&&>(src))) {}

    void __veryHiddenConstruction(const RValueBase &src) {
        e = src.e ? src.e->clone() : nullptr;
    }
    E<T> *getE() const { return reinterpret_cast<E<T>*>(e);}

    template<typename T2> RValue<T2> then(const RValueBase<T2> &next) const {
        return RValueFromE<T2>(new ESeq<T, T2>(*this, next));
    }
    template<typename T2> RValue<T2> then(RValueBase<T2> &&next) const {
        return RValueFromE<T2>(new ESeq<T, T2>(*this, std::forward<RValueBase<T2> >(next)));
    }
protected:
    RValueBase(ENonGeneric *_e): RValueBaseNonGeneric(_e, false) {}
};

template<typename T> T StructAccess(const RValueBase<T> &rval) {
    return T(&rval);
}



// TODO std::is_base_of, enable_if
//
template<typename T> T Fields(const RValueBase<T> &rval) {
    return T(&rval);
}

template<typename T> struct E: public ENonGeneric {
};

template<typename T> struct EConst: public E<T> {
    T value;
    EConst(T x): E<T>(), value(x) {}
    PTRREG_OVERRIDE(dumper) { throw "unimplemented"; }
    VALREG_OVERRIDE(dumper) {
        auto reg = dumper.nextReg();
        auto str = std::to_string(value);
        dumper.appendln(reg + " = add " + IRTypename<T>() + " " + str + ", 0 ; CONSTANT(" + str + ")");
        return reg;
    }
    TOIR_OVERRIDE(dumper) {}
    CLONE_OVERRIDE { return new EConst(value); }
};

template<typename T> struct RValue: public RValueBase<T> {
    RValue(const RValue<T> &src): RValueBase<T>(src) {}
    RValue(ENonGeneric *_e, bool _unused): RValueBase<T>(_e) {}
};

template<> struct RValue<void>: public RValueBase<void> {
    RValue(const RValue<void> &src): RValueBase<void>(src) {}
    RValue(RValue<void> &&src): RValueBase<void>(std::forward<RValueBase<void> >(src)) {}
    RValue(ENonGeneric *_e, bool _unused): RValueBase<void>(_e) {}
};

#define OPER(oper, t, tr, openc) \
    RValue<tr> operator oper (const RValueBase<t> &b) \
        { return RValueFromE<tr>(new EBin<t, tr>(*this, b, openc)); } \
    RValue<tr> operator oper (RValueBase<t> &&b) \
        { return RValueFromE<tr>(new EBin<t, tr>(*this, std::forward<RValueBase<t> >(b), openc)); } \
    RValue<tr> operator oper (t b) \
        { return RValueFromE<tr>(new EBin<t, tr>(*this, RValue<T>(b), openc)); }

template<typename T> struct RValueNumBase: public RValueBase<T> {
    RValueNumBase(T x): RValueBase<T>(new EConst<T>(x)) {}
    RValueNumBase(const RValueNumBase<T> &src): RValueBase<T>(src) {}
    RValueNumBase(RValueNumBase<T> &&src): RValueBase<T>(std::forward<RValueNumBase<T> >(src)) {}

    template<typename TR>
    RValue<TR> Cast() {
        if (sizeof_safe<T>() > sizeof_safe<TR>()) {
            return RValueFromE<TR>(new EIRConv<T, TR>(*this, __E_LLVM_IR_CONV_trunc));
        }
        else if (sizeof_safe<T>() < sizeof_safe<TR>()) {
            return RValueFromE<TR>(new EIRConv<T, TR>(*this,
                    std::is_signed<T>() ? __E_LLVM_IR_CONV_sext : __E_LLVM_IR_CONV_zext));
        }
        else {
            // TODO incomplete typecheck
            return RValueFromE<TR>(new EIRConv<T, TR>(*this, __E_LLVM_IR_CONV__to_pointer));
        }
    }

    OPER(+, T, T, OP_PLUS)
    OPER(-, T, T, OP_MINUS)
    OPER(*, T, T, OP_MULT)
    OPER(/, T, T, OP_DIV)
    OPER(%, T, T, OP_MOD)

    OPER(==, T, bool, OP_EQ)
    OPER(!=, T, bool, OP_NE)
    OPER(<, T, bool, OP_LT)
    OPER(<=, T, bool, OP_LE)
    OPER(>, T, bool, OP_GT)
    OPER(>=, T, bool, OP_GE)

    OPER(&, T, T, OP_BIT_AND)
    OPER(|, T, T, OP_BIT_OR)
    OPER(^, T, T, OP_BIT_XOR)

    RValueNumBase(ENonGeneric *_e): RValueBase<T>(_e) {}
};

#define RVALUE_INTEGRAL_SPEC(type_t) \
template<> struct RValue<type_t>: public RValueNumBase<type_t> { \
    RValue(type_t x): RValueNumBase<type_t>(x) {} \
    RValue(const RValue<type_t> &src): RValueNumBase<type_t>(src) {} \
    RValue(ENonGeneric *_e, bool _unused): RValueNumBase<type_t>(_e) {} \
};

RVALUE_INTEGRAL_SPEC(uint8_t)
RVALUE_INTEGRAL_SPEC(int8_t)
RVALUE_INTEGRAL_SPEC(uint16_t)
RVALUE_INTEGRAL_SPEC(int16_t)
RVALUE_INTEGRAL_SPEC(uint32_t)
RVALUE_INTEGRAL_SPEC(int32_t)
RVALUE_INTEGRAL_SPEC(uint64_t)
RVALUE_INTEGRAL_SPEC(int64_t)

#define DSL_BIN_OP(rett, t1, t2, v1, v2, op, openc) \
    template<typename T> RValue<typename std::enable_if<std::is_integral<T>::value, rett>::type> operator op (t1 a, t2 b) { \
        return RValue<rett>(new EBin<T, rett>(v1, v2, openc), false); \
    }

#define FULL_DSL_BIN_OP(rett, op, openc) \
DSL_BIN_OP(rett, const RValueBase<T>&, const RValueBase<T>&, a,            b,            op, openc) \
DSL_BIN_OP(rett, const RValueBase<T>&, T,                    a,            RValue<T>(b), op, openc)

/*
#define DSL_BIN_OP(t1, t2, v1, v2, op, openc) \
    template<typename T> RValue<typename std::enable_if<std::is_integral<T>::value, bool>::type> operator op (t1 a, t2 b) { \
        return RValue<bool>(new EBin<T, bool>(v1, v2, openc), false); \
    }

#define FULL_DSL_BIN_OP(op, openc) \
DSL_BIN_OP(const RValueBase<T>&, const RValueBase<T>&, a,            b,            op, openc) \
DSL_BIN_OP(const RValueBase<T>&, Param<T>&,            a,            b.Access(),   op, openc) \
DSL_BIN_OP(const RValueBase<T>&, T,                    a,            RValue<T>(b), op, openc) \
DSL_BIN_OP(Param<T>&,            const RValueBase<T>&, a.Access(),   b,            op, openc) \
DSL_BIN_OP(Param<T>&,            Param<T>&,            a.Access(),   b.Access(),   op, openc) \
DSL_BIN_OP(Param<T>&,            T,                    a.Access(),   RValue<T>(b), op, openc) \
DSL_BIN_OP(T,                    const RValueBase<T>&, RValue<T>(a), b,            op, openc) \
DSL_BIN_OP(T,                    Param<T>&,            RValue<T>(a), b.Access(),   op, openc) \
DSL_BIN_OP(T,                    T,                    RValue<T>(a), RValue<T>(b), op, openc)
*/

FULL_DSL_BIN_OP(bool, >, OP_GT)
FULL_DSL_BIN_OP(bool, <, OP_LT)
FULL_DSL_BIN_OP(bool, >=, OP_GE)
FULL_DSL_BIN_OP(bool, <=, OP_LE)
FULL_DSL_BIN_OP(bool, ==, OP_EQ)
FULL_DSL_BIN_OP(bool, !=, OP_NE)

FULL_DSL_BIN_OP(T, +, OP_PLUS)
FULL_DSL_BIN_OP(T, -, OP_MINUS)
FULL_DSL_BIN_OP(T, *, OP_MULT)
FULL_DSL_BIN_OP(T, /, OP_DIV)
FULL_DSL_BIN_OP(T, %, OP_MOD)

FULL_DSL_BIN_OP(T, &, OP_BIT_AND)
FULL_DSL_BIN_OP(T, |, OP_BIT_OR)
FULL_DSL_BIN_OP(T, ^, OP_BIT_XOR)

template<typename T>
RValue<typename std::enable_if<std::is_integral<T>::value, T>::type> operator -(const RValueBase<T> &src) {
    return RValueFromE<T>(new EUnaryMinus<T>(src));
}

template<typename T, typename IdxT, int N> struct EArrayAccess: E<T> {
    RValueBase<T[N]> arr;
    RValueBase<IdxT> idx;
    EArrayAccess(const RValueBase<T[N]> &src, const RValueBase<IdxT> _idx): arr(src), idx(_idx) {}
    CLONE_OVERRIDE { return new EArrayAccess(arr, idx); }
    PTRREG_OVERRIDE(dumper) {
        dumper.appendln("; ----- EArrayAccess START");
        auto arrBaseArrReg = arr.ptrreg(dumper);
        auto arrIdxReg = idx.valreg(dumper);
        auto resReg = dumper.nextReg();
        auto arrBaseReg = dumper.nextReg();
        auto typeName = IRTypename<T>();
        dumper.appendBitcast(arrBaseReg, IRTypename<T[N]>() + "*", arrBaseArrReg, typeName + "*");
        dumper.appendln(resReg + " = getelementptr " + typeName + ", " + typeName + "* " + arrBaseReg +
                ", " + IRTypename<IdxT>() + " " + arrIdxReg);
        return resReg;
    }
    VALREG_OVERRIDE(dumper) {
        auto ptrReg = this->ptrreg(dumper);
        auto resReg = dumper.nextReg();
        dumper.appendLoad<T>(resReg, ptrReg);
        return resReg;
    }
    TOIR_OVERRIDE(dumper) { throw "unimplemented"; }
};

template<typename T, int N> struct RValue<T[N]>: public RValueBase<T[N]> {
    RValue(ENonGeneric *_e, bool _unused): RValueBase<T[N]>(_e) {}

    template<typename IdxT>
    LValue<T> operator[](RValueBase<IdxT> idx) {
        return LValueFromE<T>(new EArrayAccess<T, IdxT, N>(*this, idx));
    }
    /*LValue<T> operator[](RValueNumBase<IdxT> idx) {
        return LValueFromE<T>(new EArrayAccess<T, N>(*this, idx.Cast<uint16_t>()));
    }*/
    LValue<T> operator[](int idx) {
        return LValueFromE<T>(new EArrayAccess<T, int, N>(*this, RValue<int>(idx)));
    }
    /*template<typename IdxT>
    LValue<T> operator[](typename std::enable_if<std::is_integral<IdxT>::value, IdxT>::type idx) {
        return LValueFromE<T>(new EArrayAccess<T, IdxT, N>(*this, RValue<IdxT>(idx)));
    }*/
};


template<typename T> struct EUnaryMinus: E<T> {
    RValueBase<T> val;
    EUnaryMinus(const RValueBase<T> &src) : val(src) {}
    CLONE_OVERRIDE { return new EUnaryMinus(val); }
    PTRREG_OVERRIDE(dumper) { throw "unimplemented"; }
    VALREG_OVERRIDE(dumper) {
        auto valReg = val.valreg(dumper);
        auto resReg = dumper.nextReg();
        dumper.appendln(resReg + " = sub nsw " + IRTypename<T>() + " 0, " + valReg);
        return resReg;
    }
    TOIR_OVERRIDE(dumper) { throw "unimplemented"; }
};


template<typename T> struct EDeref: E<T> {
    RValueBase<T*> ptr;
    EDeref(const RValueBase<T*> &src): ptr(src) {}
    CLONE_OVERRIDE { return new EDeref(ptr); }
    PTRREG_OVERRIDE(dumper) {
        return ptr.valreg(dumper);
    }
    VALREG_OVERRIDE(dumper) {
        auto ptrTo = this->ptrreg(dumper);
        auto resReg = dumper.nextReg();
        dumper.appendLoad<T>(resReg, ptrTo);
        return resReg;
    }
    TOIR_OVERRIDE(dumper) { throw "unimplemented"; }
};

template<typename T> struct EGetAddr: E<T*> {
    RValueBase<T> from;
    EGetAddr(const RValueBase<T> &src): from(src) {}
    CLONE_OVERRIDE { return new EGetAddr<T>(from); }
    PTRREG_OVERRIDE(dumper) {
        throw "unimplemented";
    }
    VALREG_OVERRIDE(dumper) {
        return from.ptrreg(dumper);
    }
    TOIR_OVERRIDE(dumper) { throw "unimplemented"; }
};

template<typename T, typename TDiff>
struct EPtrArithmetics: E<T*> {
    bool positive;
    RValueBase<T*> ptr;
    RValueBase<TDiff> diff;
    EPtrArithmetics(const RValueBase<T*> &_ptr, const RValueBase<TDiff> &_diff, bool _positive):
            ptr(_ptr), diff(_diff), positive(_positive) {}
    CLONE_OVERRIDE { return new EPtrArithmetics<T, TDiff>(ptr, diff, positive); }
    PTRREG_OVERRIDE(dumper) {
        throw "unimplemented";
    }
    VALREG_OVERRIDE(dumper) {
        auto srcPtr = ptr.valreg(dumper);
        std::string diffVal = "";
        if (positive) {
            diffVal = diff.valreg(dumper);
        }
        else {
            auto initialDiffVal = diff.valreg(dumper);
            diffVal = dumper.nextReg();
            dumper.appendln(diffVal + " = sub nsw " + IRTypename<T>() + " 0, " + initialDiffVal);
        }

        auto resReg = dumper.nextReg();
        auto typeName = IRTypename<T>();
        dumper.appendln(resReg + " = getelementptr " + typeName + ", " + typeName + "* " + srcPtr +
                        ", " + IRTypename<TDiff>() + " " + diffVal);
        return resReg;
    }
    TOIR_OVERRIDE(dumper) { throw "unimplemented"; }
};

template<typename T> struct RValue<T*>: public RValueBase<T*> {
    RValue(ENonGeneric *_e, bool _unused): RValueBase<T*>(_e) {}
    LValue<T> Deref() const { return LValueFromE<T>(new EDeref<T>(*this)); }
    LValue<T> operator*() const { return LValueFromE<T>(new EDeref<T>(*this)); }

    template<typename TDiff>
    RValue<T*> operator+(const RValueBase<TDiff> &diff) {
        return RValueFromE<T*>(new EPtrArithmetics<T, TDiff>(*this, diff, true));
    }

    template<typename TDiff>
    RValue<T*> operator-(const RValueBase<TDiff> &diff) {
        return RValueFromE<T*>(new EPtrArithmetics<T, TDiff>(*this, diff, false));
    }

    template<typename TR>
    RValue<TR> Cast() {
        auto castType = (std::is_pointer<TR>::value) ? __E_LLVM_IR_CONV__pointer : __E_LLVM_IR_CONV__from_pointer;
        return RValueFromE<TR>(new EIRConv<T*, TR>(*this, castType));
    }

    template<typename Ret, typename ... Ts>
    RValue<Ret> Call(const RValueBase<Ts> & ... args) {
        auto addr = this->Cast<HETARCH_TARGET_ADDRT>();
        auto e = new ECall<HETARCH_TARGET_ADDRT, Ret, Ts...>(addr, nullptr, args...);
        return RValueFromE<Ret>(e);
    }
};

//typename std::enable_if<!std::is_same<T, uint16_t>::value, T>::type

template<typename T> struct EAssign: public E<T> {
    LValue<T> lhs;
    RValueBase<T> rhs;
    EAssign(const LValue<T> &_lhs, const RValueBase<T> &_rhs): lhs(_lhs), rhs(_rhs) {}
    EAssign(const LValue<T> &_lhs, RValueBase<T> &&_rhs): lhs(_lhs), rhs(std::forward<RValueBase<T> >(_rhs)) {}
    //EAssign(const EAssign<T> &src): lhs(src.lhs), rhs(src.rhs) {}
    PTRREG_OVERRIDE(dumper) { throw "unimplemented"; }
    VALREG_OVERRIDE(dumper) {
        auto rhsval = rhs.valreg(dumper);
        auto lhsaddr = lhs.ptrreg(dumper);
        dumper.appendStore<T>(lhsaddr, rhsval);
        return rhsval;
    }
    TOIR_OVERRIDE(dumper) {
        this->valreg(dumper);
    }
    CLONE_OVERRIDE { return new EAssign(lhs, rhs); }
};

template<typename T>
struct LValue: public RValue<T> {
    LValue<T> Assign(const RValueBase<T> &val) { return LValueFromE<T>(new EAssign<T>(*this, val)); }
    LValue<T> Assign(RValueBase<T> &&val) {
        return LValueFromE<T>(new EAssign<T>(*this, std::forward<RValueBase<T> >(val))); }
    RValue<T*> operator &() const { return RValueFromE<T*>(new EGetAddr<T>(*this)); }
    RValue<T*> GetAddr() const { return RValueFromE<T*>(new EGetAddr<T>(*this)); }
    LValue(ENonGeneric *_e, bool _unused): RValue<T>(_e, false) {}
};

#define INTEGRAL_LVALUE(T) \
template<> struct LValue<T>: public RValue<T> { \
    LValue<T> Assign(const RValueBase<T> &val) { return LValueFromE<T>(new EAssign<T>(*this, val)); } \
    LValue<T> Assign(RValueBase<T> &&val) { \
        return LValueFromE<T>(new EAssign<T>(*this, std::forward<RValueBase<T> >(val))); } \
    LValue<T> Assign(T val) { return LValueFromE<T>(new EAssign<T>(*this, RValue<T>(val))); } \
    RValue<T*> operator &() const { return RValueFromE<T*>(new EGetAddr<T>(*this)); } \
    RValue<T*> GetAddr() const { return RValueFromE<T*>(new EGetAddr<T>(*this)); } \
    LValue(ENonGeneric *_e, bool _unused): RValue<T>(_e, false) {} \
};

INTEGRAL_LVALUE(int8_t)
INTEGRAL_LVALUE(uint8_t)
INTEGRAL_LVALUE(int16_t)
INTEGRAL_LVALUE(uint16_t)
INTEGRAL_LVALUE(int32_t)
INTEGRAL_LVALUE(uint32_t)
INTEGRAL_LVALUE(int64_t)
INTEGRAL_LVALUE(uint64_t)

template<typename T> struct Local: LValue<T> {
        Local(): LValue<T>(new ELocalVar<T>("", IRDumper::nextIndStatic()), false) {}
    Local(const std::string &name): LValue<T>(new ELocalVar<T>(name, 0), false) {}
};


template<class...> struct disjunction : std::false_type { };
template<class B1> struct disjunction<B1> : B1 { };
template<class B1, class... Bn>
struct disjunction<B1, Bn...>
    : std::conditional<bool(B1::value), B1, disjunction<Bn...>>  { };

template<typename AddrT, typename T>
struct Global: public LValue<T> {
    typedef mem::MemMgr<AddrT> MemMgrT;
    Global(MemMgrT &mgr): LValue<T>(
            new EGlobalVar<T>("", mgr.Alloc(sizeof_safe<T>(), {mem::MemClass::ReadWrite}).first), false) {}
    Global(AddrT addr): LValue<T>(new EGlobalVar<T>("", addr), false) {}
    Global(const std::string &name, AddrT addr): LValue<T>(new EGlobalVar<T>(name, addr), false) {}
    Global(const std::string &name, MemMgrT &mgr): LValue<T>(
            new EGlobalVar<T>(name, mgr.Alloc(sizeof_safe<T>(), mem::MemClass::ReadWrite)), false) {}

    AddrT Addr() {
        return reinterpret_cast<EGlobalVar<T>* >(this->e)->addr;
    }

    template<class Q = T>
    typename std::enable_if<std::is_integral<Q>::value, T>::type Get(conn::IConnection<AddrT> &conn) {
        T data;
        conn.AddrRead(Addr(), sizeof_safe<T>(), (uint8_t*)&data);
        return data;
    }

    /*template<class Q = T>
    void Set(conn::IConnection<AddrT> &conn,
             typename std::enable_if<disjunction<
                    std::is_integral<Q>,
                    std::is_pointer<Q> >::value,
             Q>::type value)
    {
        T temp = value;
        AddrT addr = reinterpret_cast<EGlobalVar<T>* >(this->e)->addr;
        conn.AddrWrite(addr, sizeof_safe<T>(), reinterpret_cast<uint8_t*>(&temp));
    }*/

    template<class Q = T>
    void Set(conn::IConnection<AddrT> &conn, typename std::enable_if<std::is_integral<Q>::value, T>::type value) {
        T temp = value;
        conn.AddrWrite(Addr(), sizeof_safe<T>(), reinterpret_cast<uint8_t*>(&temp));
    }

};

template<typename AddrT, typename T, int N> struct Global<AddrT, T[N]>: LValue<T[N]> {
    typedef mem::MemMgr<AddrT> MemMgrT;
    Global(MemMgrT &mgr): LValue<T[N]>(
            new EGlobalVar<T[N]>("", mgr.Alloc(sizeof_safe<T[N]>(), {mem::MemClass::ReadWrite}).first), false) {}
    Global(AddrT addr): LValue<T[N]>(new EGlobalVar<T[N]>("", addr), false) {}
    Global(const std::string &name, AddrT addr): LValue<T[N]>(new EGlobalVar<T>(name, addr), false) {}
    Global(const std::string &name, MemMgrT &mgr): LValue<T[N]>(
            new EGlobalVar<T[N]>(name, mgr.Alloc(sizeof_safe<T[N]>(), mem::MemClass::ReadWrite)), false) {}

    AddrT Addr() {
        return reinterpret_cast<EGlobalVar<T[N]>* >(this->e)->addr;
    }

    template<class Q = T>
    typename std::enable_if<std::is_integral<Q>::value, T>::type GetNth(conn::IConnection<AddrT> &conn, int idx) {
        T data;
        AddrT addr = Addr() + idx * sizeof_safe<T>();
        conn.AddrRead(addr, sizeof_safe<T>(), reinterpret_cast<uint8_t*>(&data));
        return data;
    }
    template<class Q = T>
    void SetNth(conn::IConnection<AddrT> &conn, int idx, typename std::enable_if<std::is_integral<Q>::value, T>::type value) {
        T temp = value;
        AddrT addr = Addr() + idx * sizeof_safe<T>();
        conn.AddrWrite(addr, sizeof_safe<T>(), reinterpret_cast<uint8_t*>(&temp));
    }
};

struct RFieldBase;

struct RStructBase {
    int offset;
    RValueBaseNonGeneric &base;
    RStructBase(RValueBaseNonGeneric &_base): base(_base), offset(0) {}
    int structSize() { return offset; }
};

template<typename Actual> struct RStruct: public RStructBase {
    RValueBase<Actual> base;
    RStruct(const RValueBase<Actual> *_base): base(), RStructBase(base)  {
        if (_base) { base.__veryHiddenConstruction(*_base); }
    }
    operator const Actual & () const { return Actual(base); }
};

struct RFieldBase {};

template<typename T> struct EStructFieldAccess: E<T> {
    int offset;
    int completeStructSize;
    RValueBaseNonGeneric base;
    EStructFieldAccess(RStructBase &_base, int _offset): base(_base.base), offset(_offset)
    {
        //std::cout << "EStructFieldAccess::ctor" << std::endl;
    }
    EStructFieldAccess(const RValueBaseNonGeneric &_base, int _offset, int _completeStructSize):
            base(_base), offset(_offset), completeStructSize(_completeStructSize)
    {
        //std::cout << "EStructFieldAccess::ctor copy" << std::endl;
    }
    CLONE_OVERRIDE { return new EStructFieldAccess(base, offset, completeStructSize); }
    PTRREG_OVERRIDE(dumper) {
        auto structBaseReg = (&base)->ptrreg(dumper);
        auto convReg = dumper.nextReg();
        auto convResReg = dumper.nextReg();
        auto resReg = dumper.nextReg();

        auto convSrcT = std::string("i") + std::to_string(getStructSize() * 8) + "*";
        dumper.appendBitcast(convReg, convSrcT, structBaseReg, "i8*");
        dumper.appendln(convResReg + " = getelementptr i8, i8* " +
                convReg + ", i32 " + std::to_string(offset));
        dumper.appendBitcast(resReg, "i8*", convResReg, IRTypename<T>() + "*");
        return resReg;
    }
    VALREG_OVERRIDE(dumper) {
        auto fieldPtrReg = ptrreg(dumper);
        auto resReg = dumper.nextReg();
        dumper.appendLoad<T>(resReg, fieldPtrReg);
        return resReg;
    }
    TOIR_OVERRIDE(dumper) { throw "unimplemented"; }
    int getStructSize() const { return completeStructSize; }
};

template<typename T, bool IsVolatile = false> struct RField: public RFieldBase, public LValue<T> {
public:
    int offset;
    RField(RStructBase &str):
            LValue<T>(new EStructFieldAccess<T>(str, str.offset), false)
    {
        offset = str.offset;
        int fieldSize = sizeof_safe<T>();
        fieldSize = widenToAlignment<sizeof(HETARCH_TARGET_ADDRT)>(fieldSize);
        str.offset += fieldSize;
    }
    void Init(const RStructBase *str) {
        reinterpret_cast<EStructFieldAccess<T>*>(this->getE())->completeStructSize = str->offset;
    }
    //RField(const RField<T> &src): LValue<T>(src.e, false), offset(src.offset), isVolatile(src.isVolatile) {}
};

template<typename T> struct ELocalVar: public E<T> {
    std::string name;
    int ind;
    ELocalVar(std::string _name, int _ind): name(_name), ind(_ind) {}
    TOIR_OVERRIDE(dumper) { throw "unimplemented"; }
    CLONE_OVERRIDE { return new ELocalVar<T>(name, ind); }
    VALREG_OVERRIDE(dumper) {
        auto ptrregname = ptrreg(dumper);
        auto reg = dumper.nextReg();
        dumper.appendLoad<T>(reg, ptrregname);
        return reg;
    }
    PTRREG_OVERRIDE(dumper) {
        std::string varname = (name != "") ? name : std::string("loc") + std::to_string(ind);
        dumper.alloc(varname, sizeof_safe_bits<T>());
        return "%" + varname;
    }
};

template<typename T> struct EGlobalVar: public E<T> {
    std::string name;
    HETARCH_TARGET_ADDRT addr;
    EGlobalVar(std::string _name, HETARCH_TARGET_ADDRT _addr): name(_name), addr(_addr) {}
    TOIR_OVERRIDE(dumper) { throw "unimplemented"; }
    CLONE_OVERRIDE { return new EGlobalVar<T>(name, addr); }
    VALREG_OVERRIDE(dumper) {
        auto ptrregname = ptrreg(dumper);
        auto reg = dumper.nextReg();
        dumper.appendLoad<T>(reg, ptrregname);
        return reg;
    }
    PTRREG_OVERRIDE(dumper) {
        std::string varname = (name != "") ? name : std::string("g") + std::to_string(addr);
        dumper.global<T>(varname, GlobalBase(name, addr));
        return "%" + varname;
    }
};

template<typename T, int N> struct ELocalVar<T[N]>: public E<T[N]> {
    std::string name;
    int ind;
    ELocalVar(std::string _name, int _ind): name(_name), ind(_ind) {}
    TOIR_OVERRIDE(dumper) { throw "unimplemented"; }
    CLONE_OVERRIDE { return new ELocalVar<T[N]>(name, ind); }
    VALREG_OVERRIDE(dumper) {
        auto ptrregname = ptrreg(dumper);
        auto reg = dumper.nextReg();
        dumper.appendLoad<T>(reg, ptrregname);
        return reg;
    }
    PTRREG_OVERRIDE(dumper) {
        std::string varname = (name != "") ? name : std::string("loc") + std::to_string(ind);
        dumper.allocArr(varname, 1, sizeof_safe<T>());
        return "%" + varname;
    }
};

template<typename T, int N> struct EGlobalVar<T[N]>: public E<T[N]> {
    std::string name;
    HETARCH_TARGET_ADDRT addr;
    EGlobalVar(std::string _name, HETARCH_TARGET_ADDRT _addr): name(_name), addr(_addr) {}
    TOIR_OVERRIDE(dumper) { throw "unimplemented"; }
    CLONE_OVERRIDE { return new EGlobalVar<T[N]>(name, addr); }
    VALREG_OVERRIDE(dumper) {
        auto ptrregname = ptrreg(dumper);
        auto reg = dumper.nextReg();
        dumper.appendLoad<T>(reg, ptrregname);
        return reg;
    }
    PTRREG_OVERRIDE(dumper) {
        std::string varname = (name != "") ? name : std::string("g") + std::to_string(addr);
        dumper.globalArr<T, N>(varname, GlobalBase(name, addr));
        return "%" + varname;
    }
};

template<typename T> struct EParam: public E<T> {
    std::string name;
    int ind;
    EParam(const std::string &_name, int _ind): name(_name), ind(_ind) {
        if (name == "") { name = std::string("p_") + std::to_string(ind); }
    }
    TOIR_OVERRIDE(dumper) { throw "unimplemented"; }
    CLONE_OVERRIDE { return new EParam<T>(name, ind); }
    PTRREG_OVERRIDE(dumper) { throw "unimplemented"; }
    VALREG_OVERRIDE(dumper) {
        return GetVarname();
    }
    std::string GetVarname() const {
        return std::string("%") + name;
    }
};

template<typename T, typename TR> struct EBin: public E<TR> {
    RValueBase<T> lhs; RValueBase<T> rhs;
    Operations op;
    EBin(const RValueBase<T> &a, const RValueBase<T> &b, Operations _op): lhs(a), rhs(b), op(_op) {}
    EBin(const RValueBase<T> &a, RValueBase<T> &&b, Operations _op):
            lhs(a), rhs(std::forward<RValueBase<T> >(b)), op(_op) {}
    TOIR_OVERRIDE(dumper) {
        dumper.appendln("toIR(EBin)");
    }
    PTRREG_OVERRIDE(dumper) { throw "unimplemented"; }
    VALREG_OVERRIDE(dumper) {
        auto lhsr = lhs.valreg(dumper);
        auto rhsr = rhs.valreg(dumper);
        auto ret = dumper.nextReg();
        switch (op) {
            case OP_PLUS: case OP_MINUS: case OP_MULT: {
                const char *opname = nullptr;
                switch (op) {
                    case OP_PLUS: opname = "add"; break;
                    case OP_MINUS: opname = "sub"; break;
                    case OP_MULT: opname = "mul"; break;
                    default: throw "binfail";
                }
                // TODO nsw?
                dumper.appendln(ret + " = " + opname + " nsw " + IRTypename<TR>() +
                                " " + lhsr + ", " + rhsr);
                return ret;
            }
            case OP_BIT_AND: case OP_BIT_OR: case OP_BIT_XOR: case OP_MOD: case OP_DIV:  {
                const char *opname = nullptr;
                switch (op) {
                    case OP_BIT_AND: opname = "and"; break;
                    case OP_BIT_OR: opname = "or"; break;
                    case OP_BIT_XOR: opname = "xor"; break;
                    case OP_MOD: opname = "srem"; break;
                    case OP_DIV: opname = "sdiv"; break;
                    default: throw "binfail";
                }
                dumper.appendln(ret + " = " + opname + " " +IRTypename<T>() + " " + lhsr + ", " + rhsr);
                return ret;
            }
            case OP_LT: case OP_GT: case OP_LE: case OP_GE: case OP_EQ: case OP_NE: {
                const char *opname = nullptr;
                switch (op) {
                    case OP_LT: opname = "slt"; break;
                    case OP_GT: opname = "sgt"; break;
                    case OP_LE: opname = "sle"; break;
                    case OP_GE: opname = "sge"; break;
                    case OP_EQ: opname = "eq"; break;
                    case OP_NE: opname = "ne"; break;
                    default: throw "binfail";
                }
                dumper.appendln(ret + " = icmp " + opname + " " +IRTypename<T>() +
                                " " + lhsr + ", " + rhsr);
                return ret;
            }
            default: break;
        }
        throw "unimplemented";
    }
    CLONE_OVERRIDE { return new EBin<T, TR>(lhs, rhs, op); }
};

template<typename T, typename TR> struct EIRConv: E<TR> {
    RValueBase<T> from;
    __E_LLVM_IR_CONV conv;
    EIRConv(const RValueBase<T> &a, __E_LLVM_IR_CONV _conv): from(a), conv(_conv) {}
    TOIR_OVERRIDE(dumper) { throw "unimplemented"; }
    PTRREG_OVERRIDE(dumper) { throw "unimplemented"; }
    VALREG_OVERRIDE(dumper) {
        if (IRTypename<T>() == IRTypename<TR>()) {
            return from.valreg(dumper);
        }
        const char* convOper = nullptr;
        switch (conv) {
            case __E_LLVM_IR_CONV_sext:          convOper = "sext";    break;
            case __E_LLVM_IR_CONV_zext:          convOper = "zext";    break;
            case __E_LLVM_IR_CONV_trunc:         convOper = "trunc";   break;
            case __E_LLVM_IR_CONV__pointer:      convOper = "bitcast"; break;
            case __E_LLVM_IR_CONV__to_pointer:   convOper = "inttoptr"; break;
            case __E_LLVM_IR_CONV__from_pointer: convOper = "ptrtoint"; break;
        }
        assert(convOper != nullptr && "Unknow IR CONV TYPE");
        auto srcReg = from.valreg(dumper);
        auto resReg = dumper.nextReg();
        dumper.appendln(resReg + " = " + convOper + " " + IRTypename<T>() + " " + srcReg + " to " + IRTypename<TR>());
        return resReg;
    }
    CLONE_OVERRIDE { return new EIRConv<T, TR>(from, conv); }
};

struct ELogicalAnd: E<bool> {
    RValueBase<bool> lhs, rhs;
    ELogicalAnd(const RValueBase<bool> &a, const RValueBase<bool> &b): lhs(a), rhs(b) {}
    TOIR_OVERRIDE(dumper) {
        dumper.appendln("toIR(EBin)");
        std::cout << "toIR(EVar)\n";
    }
    PTRREG_OVERRIDE(dumper) { throw "unimplemented"; }
    VALREG_OVERRIDE(dumper) {
        auto resPtr = dumper.tempVarReg<bool>();
        dumper.appendStore<bool>(resPtr, "1");

        auto labelCalcRhs = dumper.nextName();
        auto labelFalse = dumper.nextName();
        auto labelEnd = dumper.nextName();

        auto lhsr = lhs.valreg(dumper);
        dumper.appendBr(lhsr, labelCalcRhs, labelFalse);

        dumper.appendLabel(labelCalcRhs);
        auto rhsr = rhs.valreg(dumper);
        dumper.appendStore<bool>(resPtr, rhsr);
        dumper.appendBr(labelEnd);

        dumper.appendLabel(labelFalse);
        dumper.appendStore<bool>(resPtr, "0");
        dumper.appendBr(labelEnd);

        dumper.appendLabel(labelEnd);
        auto resReg = dumper.nextReg();
        dumper.appendLoad<bool>(resReg, resPtr);
        return resReg;
    }
    CLONE_OVERRIDE { return new ELogicalAnd(lhs, rhs); }
};

RValue<bool> operator &&(const RValueBase<bool> &a, const RValueBase<bool> &b);

template<typename R, typename T>
struct ESeq: E<T> {
    RValueBase<R> lhs; RValueBase<T> rhs;
    ESeq(const RValueBase<R> &a, const RValueBase<T> &b): lhs(a), rhs(b) {}
    ESeq(const RValueBase<R> &a, RValueBase<T> &&b): lhs(a), rhs(std::forward<RValueBase<T> >(b)) {}
    PTRREG_OVERRIDE(dumper) { throw "unimplemented"; }
    TOIR_OVERRIDE(dumper) {
        lhs.toIR(dumper);
        dumper.appendln("; ----- SEQ MIDDLE -----");
        rhs.toIR(dumper);
    }
    VALREG_OVERRIDE(dumper) {
        lhs.toIR(dumper);
        dumper.appendln("; ----- SEQ MIDDLE -----");
        return rhs.valreg(dumper);
    }
    CLONE_OVERRIDE { return new ESeq<R, T>(lhs, rhs); }
};

template <typename ...TS> struct select_last;
template <typename T> struct select_last<T> { using type = T; };
template <typename T, typename ...TS> struct select_last<T, TS...>{
    using type = typename select_last<TS...>::type;
};

/*template<typename T, typename T2>
T2 Seq(T &&a, T2 &&b) { return a.then(std::forward<T2>(b)); }

template<typename T, typename T2, typename ...TS>
T2 Seq(T &&a, T2 &&b, TS&& ... tail) {
    return a.then(Seq2(std::forward<T2>(b), std::forward<TS>(tail)...));
}*/

template<typename T, typename T2>
RValue<T2> Seq(const RValueBase<T> &a, const RValueBase<T2> &b) { return a.then(b); }

template<typename T, typename T2, typename ...TS>
RValue<typename select_last<TS...>::type> Seq(const RValueBase<T> &a, const RValueBase<T2> &b, const RValueBase<TS>& ... tail) {
    return a.then(Seq(b, (tail)...));
}

template<typename T, typename T2>
RValue<void> Seq2(T &&a, T2 &&b) { return a.then(std::forward<T2>(b)); }

template<typename T, typename T2, typename ...TS>
RValue<void> Seq2(T &&a, T2 &&b, TS&& ... tail) {
    return a.then(Seq2(std::forward<T2>(b), std::forward<TS>(tail)...));
}

template<typename T>
struct EWhile: E<void> {
    EWhile(const RValueBase<bool> &_cond, const RValueBase<T> &_body): cond(_cond), body(_body) {}
    EWhile(const EWhile &src): cond(src.cond), body(src.body) {}

    PTRREG_OVERRIDE(dumper) { throw "unimplemented"; }
    VALREG_OVERRIDE(dumper) { throw "unimplemented"; }
    TOIR_OVERRIDE(dumper) {
        auto labelCond = dumper.nextName();
        auto labelBody = dumper.nextName();
        auto labelEnd = dumper.nextName();

        dumper.appendln("; WHILE COND BEGIN"); dumper.indent += 1;
        dumper.appendBr(labelCond);
        dumper.appendLabel(labelCond);
        auto headreg = cond.valreg(dumper);
        dumper.appendBr(headreg, labelBody, labelEnd);
        dumper.appendln("; WHILE COND END"); dumper.indent -= 1;

        dumper.appendLabel(labelBody);
        dumper.indent += 2;
        body.toIR(dumper);
        dumper.appendBr(labelCond);
        dumper.indent -= 2;

        dumper.appendLabel(labelEnd);
    }

    CLONE_OVERRIDE { return new EWhile<T>(*this); }
private:
    RValueBase<bool> cond; RValueBase<T> body;
};

template<typename T>
RValue<void> While(const RValueBase<bool> &_cond, const RValueBase<T> &_body) {
    return RValueFromE<void>(new EWhile<T>(_cond, _body));
}

template<typename T1, typename T2, typename T3>
struct EFor: E<void> {
    RValueBase<T1> init; RValueBase<bool> cond; RValueBase<T2> incr; RValueBase<T3> body;
    EFor(const RValueBase<T1> &_init, const RValueBase<bool> &_cond,
         const RValueBase<T2> &_incr, const RValueBase<T3> &_body):
            init(_init), cond(_cond), incr(_incr), body(_body) {}

    //EFor(const EFor &src):
    //        init(src.init), cond(src.cond), incr(src.incr), body(src.body) {}

    PTRREG_OVERRIDE(dumper) { throw "unimplemented"; }
    VALREG_OVERRIDE(dumper) { throw "unimplemented"; }
    TOIR_OVERRIDE(dumper) {
        init.toIR(dumper);
        auto labelCond = dumper.nextName();
        auto labelBody = dumper.nextName();
        auto labelEnd = dumper.nextName();

        dumper.appendln("; FOR COND BEGIN"); dumper.indent += 1;

        dumper.appendBr(labelCond);
        dumper.appendLabel(labelCond);
        auto headreg = cond.valreg(dumper);
        dumper.appendln(std::string("br i1 ") + " " + headreg + ", label %" + labelBody + ", label %" + labelEnd);
        dumper.indent -= 1; dumper.appendln("; FOR COND END");

        dumper.appendLabel(labelBody);
        dumper.indent += 2;
        body.toIR(dumper);
        incr.toIR(dumper);
        dumper.appendBr(labelCond);
        dumper.indent -= 2;

        dumper.appendLabel(labelEnd);
    }

    CLONE_OVERRIDE { return new EFor(*this); }
};

template<typename T1, typename T2, typename T3>
RValue<void> For(const RValueBase<T1> &_init, const RValueBase<bool> &_cond,
         const RValueBase<T2> &_incr, const RValueBase<T3> &_body) {
    return RValueFromE<void>(new EFor<T1, T2, T3>(_init, _cond, _incr, _body));
}

template<typename T>
struct EIf: E<T> {
    RValueBase<bool> cond;
    RValueBase<T> thenPart;
    RValueBase<T> elsePart;

    EIf(const RValueBase<bool> &_cond, RValueBase<T> &&_then):
            cond(_cond),
            thenPart(std::forward<RValueBase<T> >(_then)) {}

    EIf(const RValueBase<bool> &_cond, RValueBase<T> &&_then, RValueBase<T> &&_else):
            cond(_cond),
            thenPart(std::forward<RValueBase<T> >(_then)),
            elsePart(std::forward<RValueBase<T> >(_else)) {}

    EIf(const EIf<T> &src):
            cond(src.cond), thenPart(src.thenPart), elsePart(src.elsePart) {}

    CLONE_OVERRIDE { return new EIf<T>(*this); }

    std::string dump(IRDumper &dumper, bool rv) const {
        auto resptr = rv ? dumper.tempVarReg<T>() : "";

        dumper.appendln("; IF HEADER BEGIN"); dumper.indent += 1;
        auto headreg = cond.valreg(dumper);
        dumper.indent -= 1; dumper.appendln("; IF HEADER END");

        auto labelThen = dumper.nextName();
        auto labelElse = dumper.nextName();
        auto labelEnd = dumper.nextName();

        dumper.appendln(std::string("br i1 ") + " " + headreg + ", label %" + labelThen + ", label %" + labelElse);

        dumper.appendLabel(labelThen);
        dumper.indent += 2;

        if (rv) {
            auto thenVal = thenPart.valreg(dumper);
            dumper.appendStore<T>(resptr, thenVal);
        }
        else {
            thenPart.toIR(dumper);
        }
        dumper.appendBr(labelEnd);
        dumper.indent -= 2;

        dumper.appendLabel(labelElse);
        dumper.indent += 2;

        if (rv) {
            auto elseVal = elsePart.valreg(dumper);
            dumper.appendStore<T>(resptr, elseVal);
        }
        else {
            elsePart.toIR(dumper);
        }
        dumper.appendBr(labelEnd);
        dumper.indent -= 2;

        dumper.appendLabel(labelEnd);
        if (rv) {
            auto resval = dumper.nextReg();
            dumper.appendLoad<T>(resval, resptr);
            return resval;
        }
        return "";

    }
    TOIR_OVERRIDE(dumper) {
        dump(dumper, false);
    }
    PTRREG_OVERRIDE(dumper) { throw "unimplemented"; }
    VALREG_OVERRIDE(dumper) {
        return dump(dumper, true);
    }
};

template<typename T>
RValue<T> If(const RValueBase<bool> &cond, RValueBase<T> &&_then, RValueBase<T> &&_else) {
    return RValueFromE<T>(
            new EIf<T>(cond, std::forward<RValueBase<T> >(_then), std::forward<RValueBase<T> >(_else)));
}

template<typename T>
RValue<T> If(const RValueBase<bool> &cond, RValueBase<T> &&_then) {
    return RValueFromE<T>(new EIf<T>(cond, std::forward<RValueBase<T> >(_then)));
}

template<std::size_t I = 0, typename... Tp>
inline typename std::enable_if<I == sizeof...(Tp), std::string>::type
ecallFmtTypes(const std::tuple<RValueBase<Tp> ...>& t) { return ""; }

template<std::size_t I = 0, typename... Tp>
inline typename std::enable_if<I < sizeof...(Tp), std::string>::type
ecallFmtTypes(const std::tuple<RValueBase<Tp> ...>& t)
{
    std::string tailStr = ecallFmtTypes<I + 1, Tp...>(t);
    return IRTypename<typename std::tuple_element<I, std::tuple<Tp...> >::type>() + ", " + tailStr;
}


template<std::size_t I = 0, typename... Tp>
inline typename std::enable_if<I == sizeof...(Tp), std::string>::type
ecallFmtArgs(IRDumper &dumper, const std::tuple<RValueBase<Tp> ...>& t) { return ""; }

template<std::size_t I = 0, typename... Tp>
inline typename std::enable_if<I < sizeof...(Tp), std::string>::type
ecallFmtArgs(IRDumper &dumper, const std::tuple<RValueBase<Tp> ...>& t)
{
    std::string argStr = std::get<I>(t).valreg(dumper);
    std::string tailStr = ecallFmtArgs<I + 1, Tp...>(dumper, t);
    return IRTypename<typename std::tuple_element<I, std::tuple<Tp...> >::type>() + " " +
            argStr + ", " + tailStr;
}

class FunctionAddrNotFixedException: public std::exception {
    virtual const char* what() const throw()
    {
        return "FunctionAddrNotFixedException";
    }
};

inline std::string trimTrailingComma(const std::string &src) {
    auto s = src;
    while (s.length() > 0 && *s.rbegin() == ' ') { s = s.substr(0, s.length() - 1); }
    while (s.length() > 0 && *s.rbegin() == ',') { s = s.substr(0, s.length() - 1); }
    return s;
}

template<typename AddrT, typename Ret, typename ... Ts>
struct ECall: E<Ret> {
    FunctionBase<AddrT> *fn;
    RValueBase<AddrT> addr;
    std::tuple<RValueBase<Ts>...> args;

    ECall(const RValueBase<AddrT> &_addr, FunctionBase<AddrT> *_fn, const std::tuple<RValueBase<Ts>...>& _args):
            addr(_addr), fn(_fn), args(_args) {}
    ECall(const RValueBase<AddrT> &_addr, FunctionBase<AddrT> *_fn, const RValueBase<Ts>& ... _args):
            addr(_addr), fn(_fn), args(_args...) {}

    CLONE_OVERRIDE { return new ECall<AddrT, Ret, Ts...>(addr, fn, args); }

    TOIR_OVERRIDE(dumper) {
        dump(dumper, false);
    }

    PTRREG_OVERRIDE(dumper) { throw "unimplemented"; }

    VALREG_OVERRIDE(dumper) {
        return dump(dumper, true);
    }
    std::string dump(IRDumper &dumper, bool withRet) const {
        auto rett = IRTypename<Ret>();

        std::string fnExpr;
        if (!fn) {
            auto addrReg = addr.valreg(dumper);
            auto argTypes = trimTrailingComma(ecallFmtTypes(args));
            fnExpr = dumper.nextReg();
            dumper.appendln(fnExpr + " = inttoptr " + IRTypename<AddrT>() + " " +
                            addrReg + " to " + IRTypename<Ret>() + "(" + argTypes + ")* ");
        }
        else if (fn->IsInline()) {
            auto inlineName = fn->GetInlineName();
            fnExpr = '@' + inlineName;
            dumper.inlineFn(inlineName, fn);
        }
        else {
            throw FunctionAddrNotFixedException();
        }

        auto argsStr = trimTrailingComma(ecallFmtArgs(dumper, args));

        if (withRet) {
            auto retReg = dumper.nextReg();
            if (rett == "void"){
                int x= 1;
            }
            dumper.appendln(retReg + " = call " + rett + " " + fnExpr + "(" + argsStr + ")");
            return retReg;
        }
        else {
            dumper.appendln(std::string("call ") + rett + " " + fnExpr + "(" + argsStr + ")");
            return "";
        }
    }
};

template<typename T, typename TH>
struct ESwitchTrue: E<T> {
    typedef RValueBase<T> ActionT;
    RValueBase<TH> head;
    std::vector<TH> unterminatedCases;
    std::vector<std::pair<std::vector<TH>, ActionT > > cases;
    ActionT defaultCase;

    ESwitchTrue(const ESwitchTrue<T, TH> &src):
            head(src.head),
            unterminatedCases(src.unterminatedCases),
            cases(src.cases),
            defaultCase(src.defaultCase) {}

    ESwitchTrue(const RValueBase<TH> &_head): head(_head) {}
    ESwitchTrue &Case(TH _case, const RValueBase<T> &action) {
        unterminatedCases.push_back(_case);
        cases.push_back(std::make_pair(unterminatedCases, action));
        unterminatedCases.clear();
        return *this;
    }
    ESwitchTrue &Case(TH _case) {
        unterminatedCases.push_back(_case);
        return *this;
    }
    RValue<void> Default() {
        return RValueFromE<void>((E<void>*)new ESwitchTrue(*this));
    }
    RValue<T> Default(const RValueBase<T> &action) {
        defaultCase = action;
        return RValueFromE<T>(new ESwitchTrue<T, TH>(*this));
    }
    std::string dump(IRDumper &dumper, bool rv) const {
        std::string isize = std::to_string((int)(sizeof(TH) * 8));
        auto headreg = head.valreg(dumper);
        auto resptr = rv ? dumper.tempVarReg<T>() : "";
        auto labelDef = dumper.nextName();
        auto labelEnd = dumper.nextName();

        std::string allCases = std::string("switch i") + isize + " " + headreg + ", label %" + labelDef + " [";
        std::vector<std::string> caseLabels;

        bool firstCase = true;

        for (int i = 0; i < cases.size(); i++) {
            auto lbl = dumper.nextName();
            caseLabels.push_back(lbl);
            for (auto val: cases[i].first) {
                std::string str = "i" + isize + " " + std::to_string(val) + ", label %" + lbl;
                if (!firstCase) { str = " " + str; } else { firstCase = false; }
                allCases += str;
            }
        }
        dumper.appendln(allCases + "]");
        for (int i = 0; i < cases.size(); i++) {
            dumper.appendLabel(caseLabels[i]);
            dumper.indent++;
            auto caseval = cases[i].second.valreg(dumper);
            if (rv) { dumper.appendStore<T>(resptr, caseval); }
            dumper.appendBr(labelEnd);
            dumper.indent--;
        }

        dumper.appendLabel(labelDef);
        auto caseval = defaultCase.valreg(dumper);
        if (rv) { dumper.appendStore<T>(resptr, caseval); }
        dumper.appendBr(labelDef);

        dumper.appendLabel(labelEnd);
        if (rv) {
            auto resval = dumper.nextReg();
            dumper.appendLoad<T>(resval, resptr);
            return resval;
        }
        return "";
    }
    PTRREG_OVERRIDE(dumper) { throw "unimplemented"; }
    TOIR_OVERRIDE(dumper) {
        dump(dumper, false);
    }
    VALREG_OVERRIDE(dumper) {
        return dump(dumper, true);
    }
    CLONE_OVERRIDE { return new ESwitchTrue<T, TH>(*this); }
};

template<typename T, typename TH>
struct ESwitch: E<T> {
    typedef RValueBase<T> ActionT;
    RValueBase<TH> head;
    std::vector<TH> unterminatedCases;
    std::vector<std::pair<std::vector<TH>, ActionT > > cases;
    ActionT defaultCase;

    ESwitch(const ESwitchTrue<T, TH> &src):
            head(src.head),
            unterminatedCases(src.unterminatedCases),
            cases(src.cases),
            defaultCase(src.defaultCase) {}

    ESwitch(const RValueBase<TH> &_head): head(_head) {}
    ESwitch &Case(TH _case, const RValueBase<T> &action) {
        unterminatedCases.push_back(_case);
        cases.push_back(std::make_pair(unterminatedCases, action));
        unterminatedCases.clear();
        return *this;
    }
    ESwitch &Case(TH _case) {
        unterminatedCases.push_back(_case);
        return *this;
    }
    RValue<void> Default() {
        return RValueFromE<void>((E<void>*)new ESwitch(*this));
    }
    RValue<T> Default(const RValueBase<T> &action) {
        defaultCase = action;
        return RValueFromE<T>(new ESwitch<T, TH>(*this));
    }
    std::string dump(IRDumper &dumper, bool rv) const {
        std::string isize = std::to_string((int)(sizeof(TH) * 8));
        auto headreg = head.valreg(dumper);
        auto resptr = rv ? dumper.tempVarReg<T>() : "";
        auto labelDef = dumper.nextName();
        auto labelEnd = dumper.nextName();

        std::vector<std::string> caseLabels;
        for (int i = 0; i < cases.size(); i++) {
            auto lbl = dumper.nextName();
            caseLabels.push_back(lbl);
            std::string nextCase;
            std::string reg;
            for (auto val: cases[i].first) {
                reg = dumper.nextReg();
                dumper.appendln(reg + " = icmp eq " + IRTypename<TH>() + " " + headreg + ", " + std::to_string(val));
                nextCase = dumper.nextName();
                dumper.appendBr(reg, lbl, nextCase);
                dumper.appendLabel(nextCase);
            }
        }
        dumper.appendBr(labelDef);

        for (int i = 0; i < cases.size(); i++) {
            dumper.appendLabel(caseLabels[i]);
            dumper.indent++;
            if (rv) {
                auto caseval = cases[i].second.valreg(dumper);
                dumper.appendStore<T>(resptr, caseval);
            }
            else {
                cases[i].second.toIR(dumper);
            }
            dumper.appendBr(labelEnd);
            dumper.indent--;
        }

        dumper.appendLabel(labelDef);
        auto caseval = defaultCase.valreg(dumper);
        if (rv) { dumper.appendStore<T>(resptr, caseval); }
        dumper.appendBr(labelEnd);
        dumper.appendLabel(labelEnd);
        if (rv) {
            auto resval = dumper.nextReg();
            dumper.appendLoad<T>(resval, resptr);
            return resval;
        }
        return "";
    }
    PTRREG_OVERRIDE(dumper) { throw "unimplemented"; }
    TOIR_OVERRIDE(dumper) {
        dump(dumper, false);
    }
    VALREG_OVERRIDE(dumper) {
        return dump(dumper, true);
    }
    CLONE_OVERRIDE { return new ESwitch<T, TH>(*this); }
};

template<typename T, typename TH>
ESwitch<T, TH> Switch(RValueBase<TH> value) {
    return ESwitch<T, TH>(value);
}

template<std::size_t I = 0, typename... Tp>
inline typename std::enable_if<I == sizeof...(Tp), std::string>::type
tupleParamsToString(std::tuple<Param<Tp> ...>& t) { return ""; }

template<std::size_t I = 0, typename... Tp>
inline typename std::enable_if<I < sizeof...(Tp), std::string>::type
tupleParamsToString(std::tuple<Param<Tp> ...>& t)
{
    return IRTypename<typename std::tuple_element<I, std::tuple<Tp...> >::type>() + " " +
            std::get<I>(t).ToString() + ", " + tupleParamsToString<I + 1, Tp...>(t);
}

struct IFunctionParamsImpl {
    virtual std::string ToString() = 0;
    virtual IFunctionParamsImpl* clone() = 0;
};

template<typename ...TS>
struct FunctionParamsImpl: IFunctionParamsImpl {
    std::tuple<Param<TS>... > params;
    FunctionParamsImpl(const Param<TS> & ... ps): params(ps...) {}
    FunctionParamsImpl(const std::tuple<Param<TS>... > &_params): params(_params) {}
    virtual std::string ToString() override {
        return tupleParamsToString(params);
    }
    virtual IFunctionParamsImpl *clone() override { return new FunctionParamsImpl<TS...>(params); }
};

struct FunctionParams {
    IFunctionParamsImpl *impl;
    FunctionParams(IFunctionParamsImpl *_impl): impl(_impl) {}
    FunctionParams(const FunctionParams &src): impl(src.impl ? src.impl->clone() : nullptr) {}
    std::string ToString() {
        return impl ? trimTrailingComma(impl->ToString()) : "";
    }
    ~FunctionParams() { if (impl) { delete impl; } }
};

template<typename AddrT> struct FunctionBase {
    typedef mem::MemMgr<AddrT> MemMgrT;
    AddrT addr;
    mem::MemClass::MemClass memClass;
    std::string name;
    std::string IRbody;
    std::vector<uint8_t> bytes;

    bool isInline;
    bool _compiledToIR;
    bool isAddrFixed;

    FunctionBase(const std::string &_name, bool _isInline):
            isInline(_isInline), _compiledToIR(false), IRbody(""),
            addr(0), name(_name),
            isAddrFixed(false) {}

    void Allocate(MemMgrT &memMgr) {
        std::tie<AddrT, mem::MemClass::MemClass>(addr, memClass) =
                memMgr.Alloc((AddrT)bytes.size(), {mem::MemClass::ReadPreferred, mem::MemClass::Any});
        isAddrFixed = true;
    }
    void Inline() { isInline = true; }
    bool HasAddr() const { return isAddrFixed; }
    bool IsInline() const { return isInline; }
    std::string GetInlineName() { return name; }

    AddrT Addr() const { return addr; }

    const std::vector<uint8_t>& Binary() const { return bytes; }

    void Send(conn::IConnection<AddrT> &conn) {
        conn.AddrWrite(this->addr, this->bytes.size(), this->bytes.data());
    }

    virtual std::string getFunctionModuleIR() = 0;

    virtual void compile(cg::ICodeGen &codeGen, MemMgrT &memMgr) = 0;

    virtual std::string getFullFunctionIR() = 0;
};

template<typename T>
RValue<T> Const(typename std::enable_if<std::is_integral<T>::value, T>::type val) {
    return RValue<T>(val);
}

inline RValue<void> Void() {
    return RValueFromE<void>(reinterpret_cast<E<void>* >(new EConst<int>(0)));
}

template<typename T> RValue<T> Undefined() { return RValueFromE<T>(nullptr); }

template<typename AddrT, typename Ret> class Function: public FunctionBase<AddrT> {
    typedef mem::MemMgr<AddrT> MemMgrT;
    RValue<Ret> body;
    std::vector<FunctionBase<AddrT>*> inlines;
    FunctionParams params;
public:
    Function(const RValue<Ret> &_body):
            FunctionBase<AddrT>("", true), body(_body) {}
    Function(const std::string &_name, bool _isInline, FunctionParams &&_params, const RValue<Ret> &_body):
            FunctionBase<AddrT>(_name, _isInline), params(_params), body(_body) {
    }
    Function(const Function &src):
            FunctionBase<AddrT>(src.name, src.isInline), params(src.params), body(src.body) {}
    void compileToIR(bool recompile = false) {
        if (recompile || !this->_compiledToIR) {
            this->IRbody = IRDumper::functionBody<Ret>(body, inlines);
            this->_compiledToIR = true;
        }
    }
    virtual std::string getFullFunctionIR() override {
        compileToIR();
        auto paramsStr = this->params.ToString();

        std::string attributes = "";
        if (this->IsInline()) { attributes += "alwaysinline "; }

        return "define " + IRTypename<Ret>() + " @" + this->name +
                "(" + paramsStr + ") " + attributes + "{\n" + this->IRbody + "}\n";
    }

    virtual std::string getFunctionModuleIR() override {
        auto fullFunction = getFullFunctionIR();
        std::string inlinesIR = "";
        for (FunctionBase<AddrT> *it: inlines) {
            auto inlineFnBody = it->getFullFunctionIR();
            inlinesIR += inlineFnBody;
        }
        return "; MODULE for function '@" + this->name+ "'\n" +
                inlinesIR +
                fullFunction +
                "; END MODULE for function '@" + this->name+ "'\n";

    };

    void compileIRToBinary(cg::ICodeGen &codeGen, MemMgrT &memMgr) {
        this->bytes = codeGen.compileFunction(getFunctionModuleIR());
        this->Allocate(memMgr);
    }

    virtual void compile(cg::ICodeGen &codeGen, MemMgrT &memMgr) override {
        compileToIR();
        compileIRToBinary(codeGen, memMgr);
    }
    template<typename ... Ts>
    RValue<Ret> operator()(const RValueBase<Ts> & ... args) {
        return Call<Ts...>(args...);
    }
    template<typename ... Ts>
    RValue<Ret> Call(const RValueBase<Ts> & ... args) {
        E<Ret> *e = nullptr;
        if (this->IsInline()) {
            e = new ECall<AddrT, Ret, Ts...>(Undefined<AddrT>(), this, args...);
        }
        else {
            e = new ECall<AddrT, Ret, Ts...>(Const<AddrT>(this->Addr()), nullptr, args...);
        }
        return RValueFromE<Ret>(e);
    }
};

inline FunctionParams Params() { return FunctionParams(nullptr); }

template<typename ...TS>
inline FunctionParams Params(const Param<TS> & ... p) {
    return FunctionParams(new FunctionParamsImpl<TS...>(p...));
}

template<typename AddrT, typename T>
Function<AddrT, T> MakeInlineFunction(const std::string &_name, FunctionParams &&_params, const RValue<T> &_body) {
return Function<AddrT, T>(_name, true, std::move(_params), _body);
}

template<typename AddrT, typename T>
Function<AddrT, T> MakeFunction(const std::string &_name, FunctionParams &&_params, RValue<T> &&_body) {
    return Function<AddrT, T>(_name, false, std::move(_params), std::forward<RValue<T> >(_body));
}

template<typename AddrT, typename T>
Function<AddrT, T> MakeFunction(const std::string &_name, const RValue<T> &_body) {
    return Function<AddrT, T>(_name, false, Params(), _body);
}

}
}
#endif

#ifndef HETARCH_DSL_LOGICAL_H
#define HETARCH_DSL_LOGICAL_H 1

#include "base.h"

namespace hetarch {
namespace dsl {

template<bool IsAnd>
struct ELogical: E<bool> {
    RValueBase<bool> lhs, rhs;
    ELogical(const RValueBase<bool> &a, const RValueBase<bool> &b): lhs(a), rhs(b) {}
    TOIR_OVERRIDE(dumper) {
        dumper.appendln("toIR(EBin)");
        std::cout << "toIR(EVar)\n";
    }
    PTRREG_OVERRIDE(dumper) { throw "unimplemented"; }
    VALREG_OVERRIDE(dumper) {
        auto resPtr = dumper.tempVarReg<bool>();
        dumper.appendStore<bool>(resPtr, (IsAnd ? "1" : "0"));

        auto labelCalcRhs = dumper.nextName();
        auto labelShort = dumper.nextName();
        auto labelEnd = dumper.nextName();

        auto lhsr = lhs.valreg(dumper);
        dumper.appendBr(lhsr, (IsAnd ? labelCalcRhs : labelShort), (IsAnd ? labelShort : labelCalcRhs));

        dumper.appendLabel(labelCalcRhs);
        auto rhsr = rhs.valreg(dumper);
        dumper.appendStore<bool>(resPtr, rhsr);
        dumper.appendBr(labelEnd);

        dumper.appendLabel(labelShort);
        dumper.appendStore<bool>(resPtr, (IsAnd ? "0": "1"));
        dumper.appendBr(labelEnd);

        dumper.appendLabel(labelEnd);
        auto resReg = dumper.nextReg();
        dumper.appendLoad<bool>(resReg, resPtr);
        return resReg;
    }
    CLONE_OVERRIDE { return new ELogical<IsAnd>(lhs, rhs); }
};

RValue<bool> operator &&(const RValueBase<bool> &a, const RValueBase<bool> &b);
RValue<bool> operator ||(const RValueBase<bool> &a, const RValueBase<bool> &b);

}
}

#endif
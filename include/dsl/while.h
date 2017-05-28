#ifndef HETARCH_DSL_WHILE_H
#define HETARCH_DSL_WHILE_H 1

#include "base.h"

namespace hetarch {
namespace dsl {

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

}
}

#endif
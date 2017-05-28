#ifndef HETARCH_DSL_FOR_H
#define HETARCH_DSL_FOR_H 1

#include "base.h"

namespace hetarch {
namespace dsl {

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


}
}

#endif
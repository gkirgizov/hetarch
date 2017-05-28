#ifndef HETARCH_DSL_IF_H
#define HETARCH_DSL_IF_H 1

#include "base.h"

namespace hetarch {
namespace dsl {

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

}
}

#endif
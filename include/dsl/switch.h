#ifndef HETARCH_DSL_SWITCH_H
#define HETARCH_DSL_SWITCH_H 1

#include "base.h"

namespace hetarch {
namespace dsl {

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

}
}

#endif
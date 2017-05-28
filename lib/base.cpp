#include "../include/dsl.h"
#include <iostream>

namespace hetarch {
namespace dsl {

int STATISTICS::RValueBaseNonGeneric_moveCtor = 0;
int STATISTICS::RValueBaseNonGeneric_copyCtor = 0;
int STATISTICS::RValueBaseNonGeneric_dtor = 0;
int STATISTICS::ParamT_moveCtor = 0;

void STATISTICS::dumpStats() {
    std::cout << "DSL STATISTICS: " << std::endl;
    std::cout << "  RValueBaseNonGeneric_moveCtor : " << RValueBaseNonGeneric_moveCtor << std::endl;
    std::cout << "  RValueBaseNonGeneric_copyCtor : " << RValueBaseNonGeneric_copyCtor << std::endl;
    std::cout << "  RValueBaseNonGeneric_dtor     : " << RValueBaseNonGeneric_dtor     << std::endl;
    std::cout << "  ParamT_moveCtor               : " << ParamT_moveCtor               << std::endl;

}

RValue<bool> operator &&(const RValueBase<bool> &a, const RValueBase<bool> &b) {
    return RValue<bool>(new ELogical<true>(a, b), false);
}

RValue<bool> operator ||(const RValueBase<bool> &a, const RValueBase<bool> &b) {
    return RValue<bool>(new ELogical<false>(a, b), false);
}

std::string ebin_valreg(IRDumper &dumper, Operations op, std::string lhsr, std::string rhsr, std::string argt) {
    auto ret = dumper.nextReg();
    switch (op) {
        case OP_PLUS: case OP_MINUS: case OP_MULT: {
            const char *opname = nullptr;
            switch (op) {
                case OP_PLUS: opname = "add"; break;
                case OP_MINUS: opname = "sub"; break;
                case OP_MULT: opname = "mul"; break;
            }
            // TODO nsw?
            dumper.appendln(ret + " = " + opname + " nsw " + argt +
                            " " + lhsr + ", " + rhsr);
            return ret;
        }
        case OP_BIT_AND: case OP_BIT_OR: case OP_BIT_XOR: case OP_MOD: case OP_DIV:
        case OP_SHR: case OP_SHL: {
            const char *opname = nullptr;
            switch (op) {
                case OP_BIT_AND: opname = "and";  break;
                case OP_BIT_OR:  opname = "or";   break;
                case OP_BIT_XOR: opname = "xor";  break;
                case OP_MOD:     opname = "srem"; break;
                case OP_DIV:     opname = "sdiv"; break;
                case OP_SHL:     opname = "shl"; break;
                case OP_SHR:     opname = "lshr"; break;
            }
            dumper.appendln(ret + " = " + opname + " " + argt + " " + lhsr + ", " + rhsr);
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
            }
            dumper.appendln(ret + " = icmp " + opname + " " + argt + " " + lhsr + ", " + rhsr);
            return ret;
        }
        default: break;
    }
    throw "unimplemented";
};

}
}
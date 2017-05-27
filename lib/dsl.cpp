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
    return RValue<bool>(new ELogicalAnd(a, b), false);
}

}
}
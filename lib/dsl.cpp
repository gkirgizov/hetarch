#include "../include/dsl.h"

namespace hetarch {
namespace dsl {

RValue<bool> operator &&(const RValueBase<bool> &a, const RValueBase<bool> &b) {
    return RValue<bool>(new ELogicalAnd(a, b), false);
}

}
}
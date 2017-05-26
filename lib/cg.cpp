#include "../include/cg.h"
#include "../include/cgImpl.h"

namespace hetarch {
namespace cg {

ICodeGen::~ICodeGen() {}

ICodeGen *createCodeGen(const std::string &triple) {
    auto codeGen = new CodeGenImpl(triple);
    return static_cast<ICodeGen*>(codeGen);
}

}
}

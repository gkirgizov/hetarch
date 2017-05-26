#ifndef _HETARCH_CG_H
#define _HETARCH_CG_H 1

#include <cstring>
#include <utility>
#include <string>
#include <vector>

namespace hetarch {
namespace cg {

class ICodeGen {
public:
    virtual std::vector<uint8_t> compileFunction(const std::string &ir) = 0;
    virtual void setTriple(const std::string &s) = 0;
    virtual ~ICodeGen();
};

ICodeGen *createCodeGen(const std::string &triple = "msp430");

}
}

#endif
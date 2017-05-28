#include "dsl/base.h"

using namespace hetarch::dsl;

unsigned IRDumper::staticCounter = 0;

std::string IRDumper::getText(std::vector<FunctionBase<AddrT>*> &_inlines)  {
    std::string copy = text;
    text = "";
    for (auto &it: allocs) {
        appendln(std::string("%") + it.first + " = alloca i" + std::to_string(it.second)
                 + ", align " + std::to_string(it.second));
    }
    for (auto &it: allocArrs) {
        auto elemSize = it.second.first;
        auto len = it.second.second;
        appendln(std::string("%") + it.first + " = alloca i" + std::to_string(elemSize * 8)
                 + ", i32 " + std::to_string(len) + ", align " + std::to_string(elemSize));
    }
    for (auto &it: globals) {
        auto typeName = "i" + std::to_string(it.second.first * 8);
        auto addr = it.second.second.addr;
        appendln(std::string("%") + it.first + " = inttoptr " + PtrIntType() + " " +
                 std::to_string(addr) + " to " + typeName + "* ");
    }
    for (auto &it: globalArrs) {
        auto typeName = "i" + std::to_string(it.second.first * 8);
        auto addr = it.second.second.first.addr;
        appendln(std::string("%") + it.first + " = inttoptr " + PtrIntType() + " " +
                 std::to_string(addr) + " to " + typeName + "* ");
    }
    for (auto &it: inlines) {
        _inlines.push_back(it.second);
    }
    text += copy;
    return text;
}

// TODO atomic
int IRDumper::nextIndStatic() {
    return ++IRDumper::staticCounter;
}
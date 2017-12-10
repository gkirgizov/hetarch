#pragma once

#include <memory>
#include <iostream>


#include "llvm/IR/Module.h"
#include "llvm/IR/LLVMContext.h"


namespace hetarch {
namespace utils {

std::unique_ptr<llvm::Module> loadModule(std::istream &stream, llvm::LLVMContext &context);

std::unique_ptr<llvm::Module> loadModule(std::string &fileName, llvm::LLVMContext &context);

}
}

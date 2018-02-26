#pragma once

#include <memory>
#include <iostream>


#include "llvm/IR/Module.h"
#include "llvm/IR/LLVMContext.h"

#include "llvm/IR/Verifier.h"
#include "llvm/Object/ObjectFile.h"

#include "../new/supportingClasses.h"


namespace hetarch {
namespace utils {

std::unique_ptr<llvm::Module> loadModule(std::istream &stream, llvm::LLVMContext &context);

std::unique_ptr<llvm::Module> loadModule(std::string &fileName, llvm::LLVMContext &context);

void printSectionName(const llvm::object::SectionRef& section);

void dumpSections(const llvm::object::ObjectFile &objFile);

void dumpSymbols(const llvm::object::ObjectFile &objFile);

bool verify_module(const llvm::Module& module);
inline bool verify_module(const IIRModule& module) { return verify_module(module.get()); };


}
}


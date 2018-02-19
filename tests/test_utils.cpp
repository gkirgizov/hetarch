#include "test_utils.h"

#include <iostream>
#include <fstream>

#include <boost/format.hpp>

#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/raw_ostream.h"


namespace hetarch {
namespace utils {


std::unique_ptr<llvm::Module> loadModule(std::istream &stream, llvm::LLVMContext &context) {

    std::string ir((std::istreambuf_iterator<char>(stream)), (std::istreambuf_iterator<char>()));

    llvm::SMDiagnostic error;
    llvm::MemoryBufferRef membufref(llvm::MemoryBuffer::getMemBuffer(llvm::StringRef(ir))->getMemBufferRef());
    auto module = llvm::parseIR(membufref, error, context);

    if(!module) {
        std::string what;
        llvm::raw_string_ostream os(what);
        error.print("error after ParseIR()", os);
        std::cerr << what;
    }

    return module;
}

std::unique_ptr<llvm::Module> loadModule(std::string &fileName, llvm::LLVMContext &context) {
    std::ifstream ir_stream(fileName);

    if (!ir_stream) {
        std::cerr << "cannot load file " << fileName << std::endl;
        return std::unique_ptr<llvm::Module>(nullptr);
    }

    return loadModule(ir_stream, context);
}


void dumpSections(const llvm::object::ObjectFile &objFile) {
    for (const auto &section : objFile.sections()) {
        llvm::StringRef sectionName;
        std::string id;
        auto err_code = section.getName(sectionName);
        id = err_code ? "error #" + std::to_string(err_code.value()) : sectionName.str();

        std::cerr
                << boost::format("addr: %x; size: %d; index: %d; name: %s")
                   % section.getAddress() % section.getSize() % section.getIndex() % id
                << std::endl;
    }
}

void dumpSymbols(const llvm::object::ObjectFile &objFile) {

    for (const auto &sym : objFile.symbols()) {
        llvm::StringRef sectionName{"error"};
        if (auto section = sym.getSection()) {
            auto err_code = section.get()->getName(sectionName);
        }

        std::cerr
                << "; type: " << sym.getType().get()
                << "; flags: 0x" << std::hex << sym.getFlags()
                << "; addr: 0x" << std::hex << sym.getAddress().get()
                << "; value: 0x" << std::hex << sym.getValue()
                << "; name: " << sym.getName().get().str()
                << "; section: " << sectionName.str()
                << std::endl;
    }
}

bool verify_module(const llvm::Module& module) {
    std::string error_str;
    llvm::raw_string_ostream os{error_str};
    bool failure = llvm::verifyModule(module, &os);
    if (failure) {
        std::cerr << "llvm::verifyModule failed for module '" << module.getName().str() << "' with error:" << std::endl
                  << error_str << std::endl;
    }
    // return true on success; return false on failure
    return !failure;
}

}
}


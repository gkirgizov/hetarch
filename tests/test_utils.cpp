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

void printSectionName(const llvm::object::SectionRef& section) {
    llvm::StringRef section_name;
    auto err_code0 = section.getName(section_name);
    if (!err_code0) {
        std::cerr << "section_name: " << section_name.str() << std::endl;
    } else {
        std::cerr << "error when accessing section.getName(): " << err_code0 << std::endl;
    }
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

        const auto err_msg = "--err in expected--";
        auto type = sym.getType();
        auto addr = sym.getAddress();
        auto name = sym.getName();

        std::cerr
                << "; type: " << (type ? type.get() : -1)
                << "; flags: 0x" << std::hex << sym.getFlags()
                << "; addr: 0x" << std::hex << (addr ? addr.get() : 0)
                << "; value: 0x" << std::hex << sym.getValue()
                << "; name: " << (name ? name.get().str() : err_msg)
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


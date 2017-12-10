#include "utils.h"

#include <fstream>

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

}
}

#include <iostream>

#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/LLVMContext.h"

#include "llvm/IRReader/IRReader.h"
#include "llvm/ExecutionEngine/Orc/IRCompileLayer.h"
#include "llvm/Object/ObjectFile.h"
#include "llvm/Object/ELFObjectFile.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/ExecutionEngine/Orc/CompileUtils.h"

#include "cg.h"

namespace hetarch {
namespace cg {

using namespace llvm;

class CodeGenImpl: public hetarch::cg::ICodeGen {
    Triple TheTriple;
    LLVMContext &context;

    std::unique_ptr<Module> getModule(StringRef text);

    object::OwningBinary<object::ObjectFile> compileModule(Module *M);

    StringRef extractCode(object::OwningBinary<object::ObjectFile> &binary);

    StringRef _compileFunction(StringRef text) {
        auto M = getModule(text);
        auto obj = compileModule(M.get());
        auto code = extractCode(obj);
        return code;
    }

public:
    CodeGenImpl(const std::string &triple) : context(getGlobalContext()) {
        setTriple(triple);
        // Initialize targets first, so that --version shows registered targets.

        LLVMInitializeMSP430TargetInfo();
        LLVMInitializeMSP430Target();
        LLVMInitializeMSP430TargetMC();
        LLVMInitializeMSP430AsmPrinter();

#ifndef HETARCH_MSP430ONLY
        LLVMInitializeX86TargetInfo();
        LLVMInitializeX86Target();
        LLVMInitializeX86TargetMC();
        LLVMInitializeX86AsmPrinter();
#endif

        //InitializeAllTargets();
        //InitializeAllTargetMCs();
        //InitializeAllAsmPrinters();
        //InitializeAllAsmParsers();

        // Initialize codegen and IR passes used by llc so that the -print-after,
        // -print-before, and -stop-after options work.
        PassRegistry *Registry = PassRegistry::getPassRegistry();
        initializeCore(*Registry);
        initializeCodeGen(*Registry);
        initializeLoopStrengthReducePass(*Registry);
        initializeLowerIntrinsicsPass(*Registry);
        initializeUnreachableBlockElimPass(*Registry);
    }

    virtual std::vector<uint8_t> compileFunction(const std::string &irText) override {
        auto strRef = _compileFunction(irText);
        std::vector<uint8_t> buf(strRef.size());
        return std::vector<uint8_t>(strRef.begin(), strRef.end());
    }

    virtual void setTriple(const std::string &s) override {
        TheTriple = Triple(Triple::normalize(s));
    }

    void setTripleMsp430() {
        setTriple("msp430");
    }
    virtual ~CodeGenImpl() override {
        // TODO implement
        std::cout << "TODO CodeGenImpl destructor" << std::endl;
    }
};

}
}


/*
class MSPExprAbstractBase {};

template<typename T>
class MSPExprBase {};

template<typename T>
struct MSPExpr {
MSPExprBase<T> *contents;
public:
MSPExpr(MSPExprBase<T> *val): contents(val) {}

MSPExpr(MSPExpr<T> &&src) { std::swap(contents, src.contents);  };
MSPExpr(const MSPExpr<T> &src) = delete;
};

template<typename T>
struct MSPExprVar: public MSPExprBase<T> {
int addr;
public:
MSPExprVar(int _addr) : addr(_addr) {}
};

template<typename T, int S>
struct MSPExprArray: public MSPExprBase<T[S]> {
int addr;
public:
MSPExprArray(int _addr): addr(_addr) {}
};

class MSPStmt {};

template<typename T>
class MSPStmtExpr: public MSPStmt {
MSPExpr<T> contents;
public:
MSPStmtExpr(MSPExpr<T> src): contents(src) {}
};

class MSPStmtGroup {
std::vector<MSPStmt*> stmts;
public:
template<typename T> MSPStmtGroup& X(MSPExpr<T> expr) {
return *this;
}
};

class MSP {
public:
template<typename T> static MSPExpr<T> Var(int addr = -1) {
return MSPExpr<T>(new MSPExprVar<T>(addr));
}
template<typename T, int S> static MSPExpr <T[S]> Array(int addr) {
return MSPExpr<T[S]>(new MSPExprArray<T, S>(addr));
}
template<typename T> static MSPStmtGroup X(MSPExpr<T> expr) {
return MSPStmtGroup();
}
};*/

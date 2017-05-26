#include <iostream>
#include "../include/cgImpl.h"
#include "llvm/CodeGen/CommandFlags.h"

namespace hetarch {
namespace cg {

std::unique_ptr<Module> CodeGenImpl::getModule(StringRef text) {
    std::unique_ptr<Module> M;
    SMDiagnostic Err;
    MemoryBufferRef membufref(MemoryBuffer::getMemBuffer(text)->getMemBufferRef());
    M = parseIR(membufref, Err, context);
    if (Err.getMessage() != "") {
        std::cout << "CodeGenImpl::getModule Error: " << Err.getMessage().str() << std::endl;
        std::cout << "    Line: " << Err.getLineNo() << std::endl;
        std::cout << "    " << Err.getLineContents().str() << std::endl;
        throw Err.getMessage();
    }
    M->setTargetTriple(TheTriple.str());
    return M;
}

object::OwningBinary<object::ObjectFile> CodeGenImpl::compileModule(Module *M) {
    std::string Error;
    const Target *TheTarget = TargetRegistry::lookupTarget("", TheTriple, Error);

    std::string CPUStr = getCPUStr(), FeaturesStr = getFeaturesStr();

    CodeGenOpt::Level OLvl = CodeGenOpt::Default;

    TargetOptions Options = InitTargetOptionsFromCodeGenFlags();

    std::unique_ptr<TargetMachine> Target(
            TheTarget->createTargetMachine(TheTriple.getTriple(), CPUStr, FeaturesStr,
                                           Options, Reloc::Default, CMModel, OLvl));

    assert(Target && "Could not allocate target machine!");

    M->setDataLayout(Target->createDataLayout());

    object::OwningBinary<object::ObjectFile> res = llvm::orc::SimpleCompiler(*Target)(*M);
    return res;
}

StringRef CodeGenImpl::extractCode(object::OwningBinary<object::ObjectFile> &binary) {
    object::ObjectFile *objfile = binary.getBinary();
    object::ELFObjectFile<object::ELFType<support::endianness::little, false> > *objf2 = (object::ELFObjectFile<object::ELFType<support::endianness::little, false> > *) objfile;

    for (auto &sect: objfile->sections()) {
        StringRef sectionName;
        sect.getName(sectionName);
        if (sectionName != ".text") { continue; }
        StringRef sectionContents;
        sect.getContents(sectionContents);
        return sectionContents;
    }
    return StringRef();

    /*for (auto &sym : objfile->symbols()) {
        if (auto nameOrErr = sym.getName()) {
            std::string name = (*nameOrErr).str();
            if (auto typeOrErr = sym.getType()) {
                object::SymbolRef::Type symType = (*typeOrErr);
                if (auto addrOrErr = sym.getAddress()) {
                    auto memref = res.getBinary()->getMemoryBufferRef();
                    uint64_t addr = (*addrOrErr);
                    if (auto sectOrErr = sym.getSection()) {
                        auto sect = (*sectOrErr);
                        auto sect2 = (*sect);
                        int p = 5;
                        StringRef str;

                        sect->getName(str);
                        auto size = sect2.getSize();
                        auto addr = sect2.getAddress();
                        StringRef cont;
                        sect2.getContents(cont);
                        int x = 5;
                    }
                }
            }

        }
    }*/
}
}
}
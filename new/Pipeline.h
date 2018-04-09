#pragma once

#include "CodeGen.h"
#include "conn/Connection.h"
#include "conn/ConnImplBase.h"
#include "CodeLoader.h"
#include "MemManager.h"
#include "Executor.h"


namespace hetarch {


template<typename AddrT>
struct SimplePipeline {
    CodeGen codeGen;
    IRTranslator irt;
    // todo: parametrize by Connection, not only ConnImpl ?
    conn::Connection<AddrT> conn;
    const MemRegion<AddrT> all_mem;
    mem::MemManagerBestFit<AddrT> memMgr;
    Executor<AddrT> exec;

    SimplePipeline(
            std::string target_triple,
            conn::ConnImplBase<AddrT> &connImpl,
            AddrT bufferSize = 1024
    )
            : codeGen(target_triple)
              , irt{is_thumb(target_triple)}
              , conn{connImpl}
              , all_mem{conn.getBuffer(0, bufferSize), bufferSize, mem::MemType::ReadWrite}
              , memMgr{all_mem}
              , exec{conn, memMgr, irt, codeGen}
    {}

    template<typename DSLFun>
    auto load(const DSLFun& dsl_fun) {
//        Function dsl_fun = make_dsl_fun<...>(dsl_gen);
        IRModule translated = irt.translate(dsl_fun);
        ObjCode compiled = codeGen.compile(translated, optLvl);
        ResidentObjCode resident_fun = CodeLoader::load(
                conn, memMgr, mem::MemType::ReadWrite,
                irt, codeGen,
                compiled
        );
        return resident_fun;
    }

private:
    bool is_thumb(std::string target_triple) const {
        return (cfg.triple.find("arm") != std::string::npos ||
                cfg.triple.find("thumb") != std::string::npos);
    }
};


}

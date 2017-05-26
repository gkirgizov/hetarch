#ifndef HETARCH_EXAMPLES_LOGGER_H
#define HETARCH_EXAMPLES_LOGGER_H 1

#include "../include/dsl.h"
#include "../include/conn.h"
#include "example_utils.h"

#include <iostream>

using namespace hetarch;
using namespace hetarch::dsl;

template<typename Platform, unsigned Length>
class Logger {
    Function<Platform, void> __logAppend() {
        Param<uint16_t> rec;
        return MakeFunction<Platform, void>("logAppend", Params(rec), Seq(
            If(logEnd < (Length - 1), Seq(
                logRecords[logEnd].Assign(rec),
                logEnd.Assign(logEnd + 1), Void()
            )),
            Void()
        ));
    }
    int hostCounter;
public:
    Global<Platform, uint16_t> logEnd;
    Global<Platform, uint16_t[Length]> logRecords;
    Function<Platform, void> logAppend;
    std::map<uint16_t, std::string> messagesMap;
    Logger(mem::MemMgr<Platform> &memMgr, cg::ICodeGen &codeGen, conn::IConnection<Platform> &conn):
            logEnd(memMgr),
            logRecords(memMgr),
            logAppend(__logAppend()),
            hostCounter(50000)
    {
        Rotate(conn);
        logAppend.compile(codeGen, memMgr);
        //log(logAppend.getFunctionModuleIR());
        //dumpBufferForDisasm(logAppend.Binary(), std::cout);
        logAppend.Send(conn);
    }

    RValue<uint16_t> message(const std::string &message) {
        uint16_t encoding = hostCounter++;
        messagesMap[encoding] = message;
        return Const<uint16_t>(encoding);
    }

    void Rotate(conn::IConnection<Platform> &conn) {
        logEnd.Set(conn, 0);
    }

    void dumpRecords(conn::IConnection<Platform> &conn) {
        uint16_t cnt = logEnd.Get(conn);
        std::cout << "LOGGER RECORDS (" << cnt << "): " << std::endl;
        for (int i = 0; i < cnt; i++) {
            uint16_t rec = logRecords.GetNth(conn, i);
            std::string decoded = messagesMap.count(rec) ? messagesMap[rec] :
                                  ("ENCODED(" + std::to_string(rec) + ")");
            std::cout << (i + 1) << ") " << decoded << std::endl;
        }
        std::cout << std::endl;
    }
};

#endif
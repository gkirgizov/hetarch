#include <iostream>
#include <iomanip>
#include <cstdint>
#include <unistd.h>

#include "../include/I2CConnection.h"
#include "dsl/base.h"

#include "basic.h"
#include "example_utils.h"
#include "msp_utils.h"

using namespace hetarch;
using namespace hetarch::dsl;
using hetarch::conn::I2CConnection;

typedef HETARCH_TARGET_ADDRT Platform;

Global<Platform, uint16_t> mainLoopHookPtr(0x2f2a);
Global<Platform, uint16_t> receiveHookPtr(0x2f2c);
Global<Platform, uint16_t> transmitHookPtr(0x2f2e);
Global<Platform, uint16_t> addrOfDyncodeGeneratedHookMainLoop(0x2f30);

uint16_t testSum(I2CConnection<Platform> &conn, cg::ICodeGen &codeGen, mem::MemMgr<Platform> &memMgr) {
    std::cout << "testSum: start " << std::endl;
    Global<Platform, uint16_t> a(memMgr), b(memMgr), c(memMgr);

    a.Set(conn, 8);
    b.Set(conn, 23);

    auto fun = MakeFunction<Platform>("sum3", Params(), c.Assign(a + b));

    log(fun.getFunctionModuleIR());
    fun.compile(codeGen, memMgr);
    dumpBufferForDisasm(fun.Binary(), std::cout);

    fun.Send(conn);

    mainLoopHookPtr.Set(conn, fun.Addr());

    usleep(300 * 1000);

    std::cout << "testSum: " << c.Get(conn) << std::endl;
    return 0;
}

const unsigned LOG_SIZE = 50;


void test1(I2CConnection<Platform> &conn, uint16_t codeBufAddr) {
    log("test1 start...");
    std::unique_ptr<cg::ICodeGen> codeGen =
            std::unique_ptr<cg::ICodeGen>(cg::createCodeGen("msp430"));

    mem::MemMgr<Platform> memMgr;
    // TODO temp
    codeBufAddr = 0x24A0;//10;
    memMgr.DefineArea(codeBufAddr, codeBufAddr + 1000, mem::MemClass::ReadWrite);
    log("codeGen and memMgr initialized...");


    Logger<Platform, LOG_SIZE> logger(memMgr, *codeGen, conn);

    std::cout << "LOGGER ADDR: " << logger.logAppend.Addr() << std::endl;


    BasicFuncs<Platform, LOG_SIZE> funcs(memMgr, logger);

    log(funcs.bubbleSortPoints.getFunctionModuleIR());
    funcs.bubbleSortPoints.compile(*codeGen, memMgr);
    dumpBufferForDisasm(funcs.bubbleSortPoints.Binary(), std::cout);

//    initRemoteFunctionCall(memMgr, *codeGen, conn);



    //auto testFunc1 = MakeFunction<uint16_t>("testFunc1", While(tempGlobal != (uint16_t)(20), tempGlobal.Assign((uint16_t)(20))));
    //If(tempGlobal < (uint16_t)(20), RValue<uint16_t>(20), RValue<uint16_t>(30)));

    log(funcs.bubbleSort.getFunctionModuleIR());
    funcs.bubbleSort.compile(*codeGen, memMgr);
    dumpBufferForDisasm(funcs.bubbleSort.Binary(), std::cout);

    funcs.elems.SetNth(conn, 0, 7);
    funcs.elems.SetNth(conn, 1, 3);
    funcs.elems.SetNth(conn, 2, 2);
    funcs.elems.SetNth(conn, 3, 4);
    funcs.elems.SetNth(conn, 4, 8);

    log("Reading elements check...");
    for (int i = 0; i < 5; i++) {
        int elem = funcs.elems.GetNth<uint32_t>(conn, i);
        std::cout << "elems[" << i << "]: " << elem << std::endl;
    }

    log("bubbleSort sending...");
    funcs.bubbleSort.Send(conn);
    log("bubbleSort sent...");

    std::cout << "Addr of hook: " << addrOfDyncodeGeneratedHookMainLoop.Get(conn) << std::endl;

    mainLoopHookPtr.Set(conn, funcs.bubbleSort.Addr());
    //mainLoopHookPtr.Set(conn, logger.logAppend.Addr());


    //testSum(conn, *codeGen, memMgr); healthcheck(conn); return;

    log("Ptr Set to " + std::to_string(funcs.bubbleSort.Addr()) + "...");
    auto ptrCheckRes = mainLoopHookPtr.Get(conn);
    log("Ptr check: " + std::to_string(ptrCheckRes));

    usleep(300 * 1000);

    log("Reading elements...");
    for (int i = 0; i < 5; i++) {
        int elem = funcs.elems.GetNth(conn, i);
        std::cout << "elems[" << i << "]: " << elem << std::endl;
    }

    logger.dumpRecords(conn);

    healthcheck(conn);
}

int main(int argc, char **argv) {
    initMSP430Intrinsics();

    log("Start...");
    log("Creating connection...");
    auto conn = std::unique_ptr<hetarch::conn::I2CConnection<Platform> >(
            new hetarch::conn::I2CConnection<Platform>("/dev/i2c-2", 0x48));
    log("");
    log("Connection created!");

    healthcheck(*conn);

    log("Getting buff for code...");

    uint16_t buff = conn->GetBuff(0);

    log("Got buff: " + std::to_string(buff));

    test1(*conn, buff);

    return 0;
}
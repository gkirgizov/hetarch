#include <iostream>
#include <iomanip>
#include <cstdint>
#include <unistd.h>
#include <algorithm>
#include <random>

#include "../include/I2CConnection.h"
#include "../include/dsl.h"

#include "basic.h"
#include "sensor.h"
#include "msp_ext_points.h"

using namespace hetarch;
using namespace hetarch::dsl;
using namespace hetarch::conn;

typedef HETARCH_TARGET_ADDRT Platform;

void testBubbleSort(mem::MemMgr<Platform> &memMgr, cg::ICodeGen &codeGen,
                    I2CConnection<Platform> &conn, BasicFuncs<Platform, 50> &funcs, MspExtensionPoints<Platform, 50> &extPts)
{
    std::vector<uint16_t> nums = { 1, 2, 3, 4, 5 };

    std::random_shuffle(std::begin(nums), std::end(nums));

    for (int i = 0; i < 5; i++) { funcs.elems.SetNth(conn, i, nums[i]); }
    log("testBubbleSort: Data initialized");

    for (int i = 0; i < 5; i++) { std::cout << funcs.elems.GetNth(conn, i) << " "; }
    log("");
    log("testBubbleSort: Data checked");

    funcs.bubbleSort.compile(codeGen, memMgr);
    funcs.bubbleSort.Send(conn);

    //extPts.mainLoopHookPtr.Set(conn, funcs.bubbleSort.Addr()); usleep(300 * 1000);

    log("receiveHookPtr: " + std::to_string(extPts.receiveHookPtr.Get(conn)));
    log("mainLoopHookPtr: " + std::to_string(extPts.mainLoopHookPtr.Get(conn)));
    uint8_t buf[100]; conn.AddrRead(extPts.receiveHookPtr.Get(conn), 20, buf);
    dumpBufferForDisasm(std::vector<uint8_t>(buf, buf + 20), std::cout);

    conn.Call(funcs.bubbleSort.Addr());
    //conn.Call(funcs.bubbleSortPoints.Addr());
    //conn.Call(funcs.bubbleSort.Addr());

    //int value = conn.ReadAnalogSensor(2);
    //int value2 = conn.ReadAnalogSensor(1);
    //std::cout << "ReadAnalogSensor: " << value << " " << value2 << std::endl;

    funcs.logger.dumpRecords(conn);

    for (int i = 0; i < 5; i++) { std::cout << funcs.elems.GetNth(conn, i) << " "; } std::cout << std::endl;
}

void testBubbleSortPoints(mem::MemMgr<Platform> &memMgr, cg::ICodeGen &codeGen,
                          I2CConnection<Platform> &conn, BasicFuncs<Platform, 50> &funcs)
{
    funcs.bubbleSortPoints.compile(codeGen, memMgr);
    funcs.bubbleSortPoints.Send(conn);

    for (int i = 0; i < 5; i++) {
        Global<Platform, uint16_t> xf(funcs.points2D.Addr() + i * 16);
        Global<Platform, uint16_t> yf(funcs.points2D.Addr() + i * 16 + 8);
        xf.Set(conn, 8 - i);
        yf.Set(conn, i + 5);
    }

    log("testBubbleSortPoints: Data initialized");

    for (int i = 0; i < 5; i++) {
        Global<Platform, uint16_t> xf(funcs.points2D.Addr() + i * 16);
        Global<Platform, uint16_t> yf(funcs.points2D.Addr() + i * 16 + 8);
        std::cout << "(" << xf.Get(conn) << ", " << yf.Get(conn) << ")" << std::endl;
    }


    //conn.Call(funcs.bubbleSortPoints.Addr());

    std::cout << "Points sorted" << std::endl;

    for (int i = 0; i < 5; i++) {
        Global<Platform, uint16_t> xf(funcs.points2D.Addr() + i * 16);
        Global<Platform, uint16_t> yf(funcs.points2D.Addr() + i * 16 + 8);
        std::cout << "(" << xf.Get(conn) << ", " << yf.Get(conn) << ")" << std::endl;
    }

    funcs.logger.dumpRecords(conn);
}

int main(int argc, char **argv) {
    log("Start...");
    log("Creating connection...");
    auto conn = std::unique_ptr<hetarch::conn::I2CConnection<Platform> >(
            new hetarch::conn::I2CConnection<Platform>("/dev/i2c-2", 0x48));

    std::unique_ptr<cg::ICodeGen> codeGen =
            std::unique_ptr<cg::ICodeGen>(cg::createCodeGen("msp430"));

    mem::MemMgr<Platform> memMgr;

    auto codeBufAddr = conn->GetBuff(0);

#ifndef DHETARCH_MSP430ONLY
    //codeBufAddr = 9200;
#endif
    memMgr.DefineArea(codeBufAddr, codeBufAddr + 1000, mem::MemClass::ReadWrite);
    log("codeGen and memMgr initialized...");

    Logger<Platform, 50> logger(memMgr, *codeGen, *conn);
    log("Logger initialized");

    Sensor<Platform, 50> sensor(memMgr, *codeGen, *conn, logger);

    BasicFuncs<Platform, 50> funcs(memMgr, logger);
    log("BasicFuncs initialized");

    MspExtensionPoints<Platform, 50> points(memMgr, *codeGen, *conn, logger, sensor);

    log("MspExtensionPoints initialized: " + std::to_string(points.SingleCallFunction.Get(*conn)));

    log("MspExtensionPoints.receiveHookPtr: " + std::to_string(points.receiveHookPtr.Get(*conn)));
    log("MspExtensionPoints.mainLoopHookPtr: " + std::to_string(points.mainLoopHookPtr.Get(*conn)));

    testBubbleSort(memMgr, *codeGen, *conn, funcs, points);

    // testBubbleSortPoints(memMgr, *codeGen, *conn, funcs);

    std::cout << "Finishing: closing connection" << std::endl;

    conn->Close();

    std::cout << "Finishing: connection closed" << std::endl;

    return 0;
}
#include <iostream>
#include <iomanip>
#include <cstdint>
#include <unistd.h>
#include <algorithm>
#include <random>

#include "../include/TCPTestConn.h"
#include "../include/dsl.h"

#include "basic.h"
#include "logger.h"
#include "example_utils.h"

using namespace hetarch;
using namespace hetarch::dsl;
using namespace hetarch::conn;

typedef HETARCH_TARGET_ADDRT Platform;

void testBubbleSort(mem::MemMgr<Platform> &memMgr, cg::ICodeGen &codeGen,
                    TCPTestConnection &conn, BasicFuncs<Platform, 50> &funcs)
{
    funcs.logger.Rotate(conn);

    std::vector<uint16_t> nums = { 1, 2, 3, 4, 5 };

    std::random_shuffle(std::begin(nums), std::end(nums));

    for (int i = 0; i < 5; i++) { funcs.elems.SetNth(conn, i, nums[i]); }
    log("testBubbleSort: Data initialized");

    for (int i = 0; i < 5; i++) { std::cout << funcs.elems.GetNth(conn, i) << " "; }
    log("");
    log("testBubbleSort: Data checked");

    funcs.bubbleSort.compile(codeGen, memMgr);
    funcs.bubbleSort.Send(conn);

    conn.Call(funcs.bubbleSort.Addr());

    for (int i = 0; i < 5; i++) { std::cout << funcs.elems.GetNth(conn, i) << " "; } std::cout << std::endl;

    funcs.logger.dumpRecords(conn);
}

void testBubbleSortPoints(mem::MemMgr<Platform> &memMgr, cg::ICodeGen &codeGen,
                          TCPTestConnection &conn, BasicFuncs<Platform, 50> &funcs)
{
    funcs.bubbleSortPoints.compile(codeGen, memMgr);
    funcs.bubbleSortPoints.Send(conn);

    funcs.logger.Rotate(conn);

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

    conn.Call(funcs.bubbleSortPoints.Addr());


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
    auto conn = std::unique_ptr<hetarch::conn::TCPTestConnection>(
            new hetarch::conn::TCPTestConnection("localhost", 1334));

    std::unique_ptr<cg::ICodeGen> codeGen =
            std::unique_ptr<cg::ICodeGen>(cg::createCodeGen("x86_64-unknown-linux-gnu"));

    mem::MemMgr<Platform> memMgr;

    auto codeBufAddr = conn->GetBuff(0);

    memMgr.DefineArea(codeBufAddr, codeBufAddr + 1000, mem::MemClass::ReadWrite);
    log("codeGen and memMgr initialized...");

    Logger<Platform, 50> logger(memMgr, *codeGen, *conn);
    log("Logger initialized");

    BasicFuncs<Platform, 50> funcs(memMgr, logger);
    log("BasicFuncs initialized");

    testBubbleSort(memMgr, *codeGen, *conn, funcs);

    testBubbleSortPoints(memMgr, *codeGen, *conn, funcs);

    conn->Close();

    return 0;
}
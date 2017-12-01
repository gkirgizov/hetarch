#include <iostream>
#include <iomanip>
#include <cstdint>
#include <unistd.h>
#include <algorithm>
#include <random>

#include "../include/TCPTestConn.h"
#include "dsl/base.h"

#include "basic.h"
#include "logger.h"
#include "example_utils.h"
#include "arithm.h"

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

template<typename T> void assertEqual(const std::string &name, T expected, T actual) {
    if (expected == actual) {
        std::cout << "OK: " << name << std::endl;
    }
    else {
        std::cout << "FAIL: " << name << ", expected:" << expected << ", actual: " << actual << std::endl;
    }
}

void compositionFuncTest(mem::MemMgr<Platform> &memMgr, cg::ICodeGen &codeGen, TCPTestConnection &conn) {
    Param<uint32_t> x1;
    auto sqr = MakeFunction<Platform, uint32_t>("sqr", Params(x1),
            x1 * x1
    );

    Local<uint32_t> a1, a2;
    Param<uint32_t> x2;
    auto sixth = MakeFunction<Platform, uint32_t>("sixth", Params(x2), Seq(
            a1.Assign(sqr(x2)),
            a2.Assign(sqr(a1)),
            a1 * a2
    ));

    Param<uint32_t> x3;
    auto ident = MakeFunction<Platform, uint32_t>("ident", Params(x3),
            sqr(x3)
    );

    log(sqr.getFunctionModuleIR());
    sqr.compile(codeGen, memMgr);
    sqr.Send(conn);
//    dumpBufferForDisasm(sqr.Binary(), std::cout);

    log(sixth.getFunctionModuleIR());
    sixth.compile(codeGen, memMgr);
    sixth.Send(conn);
//    dumpBufferForDisasm(sixth.Binary(), std::cout);

    log(ident.getFunctionModuleIR());
    ident.compile(codeGen, memMgr);
    ident.Send(conn);

    Global<Platform, uint32_t> x(memMgr), y(memMgr);
    auto testf = MakeFunction<Platform>("testf", Params(),
         y.Assign(ident(x))
    );

    log(testf.getFunctionModuleIR());
    testf.compile(codeGen, memMgr);
    testf.Send(conn);
//    dumpBufferForDisasm(testf.Binary(), std::cout);

    // do I need to send the 'sqr' and 'sixth' funcs also?
    x.Set(conn, 2);
    conn.Call(testf.Addr());
    std::cout << "result: " << y.Get(conn) << std::endl << std::endl;
//    assertEqual("test composition of funcs", (uint32_t)(64), y.Get(conn));

}

void simpleTests(mem::MemMgr<Platform> &memMgr, cg::ICodeGen &codeGen, TCPTestConnection &conn) {
    auto execute = [&memMgr, &codeGen, &conn](Function<Platform, void> &fn){
        fn.compile(codeGen, memMgr);
        fn.Send(conn);
        conn.Call(fn.Addr());
    };

    Global<Platform, uint16_t> x(memMgr), y(memMgr);
    auto tt1 = MakeFunction<Platform, uint16_t>("tt1", Params(), x.Assign(Const<uint16_t>(6)));

    x.Set(conn, 0x27AB); y.Set(conn, 0x1B34);
    auto test1 = MakeFunction<Platform>("test1", Params(), Seq(x.Assign(x & y), Void()));
    execute(test1);
    assertEqual("test1", (uint16_t)(0x27AB & 0x1B34), x.Get<uint16_t>(conn));

    x.Set(conn, 300);
    auto test2 = MakeFunction<Platform>("test1", Params(),
            While(x > 0, Seq(
                x.Assign(x - 1),
                y.Assign(y + 1)
            )));
    execute(test2);
    assertEqual("test2", (uint16_t)(0x1B34 + 300), y.Get(conn));

    x.Set(conn, 2); y.Set(conn, 2);
    auto test3 = MakeFunction<Platform>("test1", Params(), Seq(
        If(x > 0 && y > 0, x.Assign(x / y), x.Assign(x - y)), Void()
    ));
    execute(test3);
    assertEqual("test3", (uint16_t)(2 / 2), x.Get(conn));

    Arithmetics<Platform> arithm(memMgr, codeGen, conn);

    Global<Platform, uint32_t> x32(memMgr), y32(memMgr);
    x.Set(conn, 13921); y.Set(conn, 100);
    x32.Set(conn, 820); y32.Set(conn, 1312);
    auto test4 = MakeFunction<Platform>("test4", Params(), Seq(
        x.Assign(arithm.div16(x, y)),
        y.Assign(arithm.mul16(x, Const<uint16_t>(98))),
        x32.Assign(arithm.mul32(x32, y32)),
        y32.Assign(arithm.div32(x32, Const<uint32_t>(234))), Void()
    ));
    execute(test4);
    assertEqual("test4:1", (uint16_t)(13921 / 100), x.Get(conn));
    assertEqual("test4:2", (uint16_t)(13921 / 100 * 98), y.Get(conn));
    assertEqual("test4:3", (uint32_t)(820 * 1312), x32.Get(conn));
    assertEqual("test4:4", (uint32_t)(820 * 1312 / 234), y32.Get(conn));

    x.Set(conn, 4); y.Set(conn, 8);
    auto test5 = MakeFunction<Platform>("test5", Params(), Seq(
            x32.Assign((x * y).Cast<uint32_t>()), Void()
    ));
    execute(test5);
    assertEqual("test5", (uint32_t)(4 * 8), x32.Get(conn));
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

    memMgr.DefineArea(codeBufAddr, codeBufAddr + 2000, mem::MemClass::ReadWrite);
    log("codeGen and memMgr initialized...");

    Logger<Platform, 50> logger(memMgr, *codeGen, *conn);
    log("Logger initialized");

    BasicFuncs<Platform, 50> funcs(memMgr, logger);
    log("BasicFuncs initialized");

    testBubbleSort(memMgr, *codeGen, *conn, funcs);

    testBubbleSortPoints(memMgr, *codeGen, *conn, funcs);

    simpleTests(memMgr, *codeGen, *conn);

    std::cout << std::endl
              << "ATTENTION! testing composition of function! watch generated IR carefully!" << std::endl << std::endl;
    compositionFuncTest(memMgr, *codeGen, *conn);

    conn->Close();

    STATISTICS::dumpStats();

    log("Finishing...");

    return 0;
}
#ifndef HETARCH_EXAMPLES_MSP_EXT_POINTS_H
#define HETARCH_EXAMPLES_MSP_EXT_POINTS_H 1

#include <cstdint>
#include <iostream>

#include <dsl/base.h>
#include <conn.h>
#include "sensor.h"

#include "example_utils.h"

using namespace hetarch;
using namespace hetarch::dsl;

template<typename Platform, unsigned LoggerLength>
class MspExtensionPoints {
    void div() {
        int x = 345, y = 23, t, shift = 1, res = 0;
        while (x > y) {
            t = y;
            while (x >= (t << 1)) { t <<= 1; shift <<= 1; }
            x -= t;
            res |= shift;
        }
    }
    Function<Platform, void> __receiveHook() {
        return MakeFunction<Platform, void>("receiveHook", Params(), Seq(
                logger.logAppend.Call(logger.message("In receiveHook")),
                logger.logAppend.Call(receiveParams[0]),
                logger.logAppend.Call(receiveParams[1]),
                logger.logAppend.Call(receiveParams[2]),
                //If(__reg == 0x69, Seq(
                //        logger.logAppend.Call(logger.message("In receiveHook SingleCallFunction")), Void()
                        // SingleCallFunction.Assign(*(__data.Cast<uint16_t*>())), Void()
                //)),
                Void()
        ));
    }
    Function<Platform, void> __mainLoopHook() {
        return MakeFunction<Platform, void>("mainLoopHook", Params(), Seq(
            logger.logAppend.Call(logger.message("In mainLoopHook")),
            sensor.conversationJAx(),
            If(SingleCallFunction != 0, Seq(


                // GCC !!! error: expected primary-expression before ‘*’ token
                //   SingleCallFunction.Cast<uint16_t*>().Call<void>()
                static_cast<RValueNumBase<uint16_t>*>(std::addressof(SingleCallFunction))->Cast<uint16_t*>().Call<void>(),

                SingleCallFunction.Assign(0), Void()
            )),

            Void()
        ));
    }
    Function<Platform, void> __transmitHook() {
        return MakeFunction<Platform, void>("transmitHook", Params(), Seq(
                logger.logAppend.Call(logger.message("In transmitHook")),
                logger.logAppend.Call(transmitParams[0]),
                If (transmitParams[0] == 0x66, logger.logAppend.Call(logger.message("Analog 0 requested")),
                If (transmitParams[0] == 0x67, logger.logAppend.Call(logger.message("Analog 1 requested")),
                If (transmitParams[0] == 0x68, logger.logAppend.Call(logger.message("Analog 2 requested")),
                If (transmitParams[0] == 0x69, logger.logAppend.Call(logger.message("Analog 3 requested")),
                If (transmitParams[0] == 0x70, logger.logAppend.Call(logger.message("Analog 4 requested"))
                ))))),
                //If(__reg == 0x69, Seq(
                //        logger.logAppend.Call(logger.message("In receiveHook SingleCallFunction")), Void()
                // SingleCallFunction.Assign(*(__data.Cast<uint16_t*>())), Void()
                //)),
                Void()
        ));
    }
public:
    Logger<Platform, LoggerLength> &logger;
    Sensor<Platform, LoggerLength> &sensor;

    Global<Platform, uint16_t> mainLoopHookPtr;
    Global<Platform, uint16_t> receiveHookPtr;
    Global<Platform, uint16_t> transmitHookPtr;

    Global<Platform, uint16_t[3]> receiveParams;
    Global<Platform, uint16_t[3]> transmitParams;

    Global<Platform, uint16_t> SingleCallFunction;

    Function<Platform, void> mainLoopHook;
    Function<Platform, void> receiveHook;
    Function<Platform, void> transmitHook;

    MspExtensionPoints(mem::MemMgr<Platform> &memMgr,
                       cg::ICodeGen &codeGen,
                       conn::IConnection<Platform> &conn,
                       Logger<Platform, LoggerLength> &_logger,
                       Sensor<Platform, LoggerLength> &_sensor):
            logger(_logger),
            sensor(_sensor),
            mainLoopHookPtr(0x00002f34),
            receiveHookPtr(0x00002f36),
            transmitHookPtr(0x00002f38),
            receiveParams(0x00002d14),
            transmitParams(0x00002d1a),

            SingleCallFunction(memMgr),
            mainLoopHook(__mainLoopHook()),
            receiveHook(__receiveHook()),
            transmitHook(__transmitHook())
    {
        SingleCallFunction.Set(conn, 0);

        mainLoopHook.compile(codeGen, memMgr);
        mainLoopHook.Send(conn);

        receiveHook.compile(codeGen, memMgr);
        receiveHook.Send(conn);

        transmitHook.compile(codeGen, memMgr);
        transmitHook.Send(conn);

        log(receiveHook.getFunctionModuleIR());
        dumpBufferForDisasm(receiveHook.Binary(), std::cout);

        log(mainLoopHook.getFunctionModuleIR());
        dumpBufferForDisasm(mainLoopHook.Binary(), std::cout);

        mainLoopHookPtr.Set(conn, mainLoopHook.Addr());
        receiveHookPtr.Set(conn, receiveHook.Addr());
        transmitHookPtr.Set(conn, transmitHook.Addr());
    }

    void ScheduleRegular(const RValue<void> &action) {
        return MakeFunction<Platform, void>("mainLoopHook", Params(), Seq(
                logger.logAppend.Call(logger.message("In mainLoopHook")),
                sensor.conversationJAx(),
                If(SingleCallFunction != 0, Seq(


                        // GCC !!! error: expected primary-expression before ‘*’ token
                        //   SingleCallFunction.Cast<uint16_t*>().Call<void>()
                        static_cast<RValueNumBase<uint16_t>*>(std::addressof(SingleCallFunction))->Cast<uint16_t*>().Call<void>(),

                        SingleCallFunction.Assign(0), Void()
                )),

                Void()
        ));
    }
};

#endif
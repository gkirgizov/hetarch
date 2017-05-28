#ifndef HETARCH_EXAMPLES_BASIC_H
#define HETARCH_EXAMPLES_BASIC_H 1

#include "dsl/base.h"
#include "logger.h"

using namespace hetarch;
using namespace hetarch::dsl;

DSL_STRUCT(Point2Ds) {
    RField<uint8_t, true> x;
    RField<uint8_t, true> y;
    Point2Ds(const RValueBase<Point2Ds> *base): RStruct<Point2Ds>(base), x(*this), y(*this) {
        x.Init(this), y.Init(this);
    }
};

DSL_STRUCT(Point2D) {
    RField<uint16_t, true> x;
    RField<uint16_t, true> y;
    Point2D(const RValueBase<Point2D> *base): RStruct<Point2D>(base), x(*this), y(*this) {
        x.Init(this), y.Init(this);
    }
};

template<typename Platform, unsigned LogSize>
class BasicFuncs {
    Function<Platform, uint16_t> __sum2() {
        Param<uint16_t> a("a"), b("b");
        return MakeFunction<Platform, uint16_t>("sum2", Params(a, b), a + b);
    }

    Function<Platform, uint16_t> __getFixedValue() {
        return MakeFunction<Platform>("getFixedValue", Params(), sum2(RValue<uint16_t>(17), RValue<uint16_t>(3)));
    }

    Function<Platform, uint16_t> __initArray() {
        return MakeFunction<Platform>("initArray", Params(), Seq(
                elems[RValue<uint16_t>(0)].Assign((uint16_t)7),
                elems[RValue<uint16_t>(1)].Assign((uint16_t)2),
                elems[RValue<uint16_t>(2)].Assign((uint16_t)9),
                elems[RValue<uint16_t>(3)].Assign((uint16_t)1),
                elems[RValue<uint16_t>(4)].Assign((uint16_t)3)
        ));
    }

    Function<Platform, void> __bubbleSort() {
        Local<uint16_t> tmp("tmp"), i("i"), j("j");
        return MakeFunction<Platform>("bubbleSort", Params(), Seq(
            // TODO replace Call with operator()
            logger.logAppend(logger.message("bubbleSort start")),
            For(i.Assign(0), i < 5, ++i,
                For(j.Assign(0), j < 4, ++j,
                    If(elems[j] > elems[j + 1], Seq(
                        tmp.Assign(elems[j]),
                        elems[j].Assign(elems[j + 1]),
                        elems[j + 1].Assign(tmp)
                    ))
                )
            ),
            logger.logAppend(logger.message("bubbleSort finished"))
        ));
    }

    Function<Platform, void> __bubbleSortPoints() {
        Local<uint16_t> i("i"), j("j");
        Local<Point2D> tmp("tmp");
        auto e = tmp.Assign(points2D[j]);
        return MakeFunction<Platform>("bubbleSortPoints", Params(), Seq(
                logger.logAppend(logger.message("bubbleSortPoints start")),
                For(i.Assign(0), i < 5, ++i,
                    For(j.Assign(0), j < 4, ++j,
                        If(Fields(points2D[j]).x > Fields(points2D[j + 1]).x, Seq(
                            tmp.Assign(points2D[j]),
                            points2D[j].Assign(points2D[j + 1]),
                            points2D[j + 1].Assign(tmp)
                        ))
                    )
                ),
                logger.logAppend(logger.message("bubbleSortPoints finished"))
        ));
    }

    Function<Platform, uint16_t> __getStaticSum() {
        Local<uint16_t> x("a"), i("i");
        return MakeFunction<Platform>("getStaticSum", Params(), Seq(
                x.Assign(0),
                For(i.Assign(0), i < (uint16_t)7, ++i, Seq(
                        elems[i].Assign(i + (uint16_t)1),
                        x.Assign(x + elems[i])
                )),
                x
        ));
    }
public:
    Logger<Platform, LogSize> &logger;

    Global<Platform, uint16_t[20]> elems;

    Global<Platform, Point2D[5]> points2D;
    Global<Platform, Point2Ds[5]> points2Ds;

    Function<Platform, uint16_t> sum2;
    Function<Platform, uint16_t> getFixedValue;
    Function<Platform, uint16_t> getStaticSum;
    Function<Platform, uint16_t> initArray;
    Function<Platform, void> bubbleSort;
    Function<Platform, void> bubbleSortPoints;

    BasicFuncs(mem::MemMgr<Platform> &memMgr, Logger<Platform, LogSize> &_logger):
            logger           (_logger),

            elems            (memMgr),
            points2D         (memMgr),
            points2Ds        (memMgr),

            sum2             (__sum2()),
            getFixedValue    (__getFixedValue()),
            getStaticSum     (__getStaticSum()),
            initArray        (__initArray()),
            bubbleSort       (__bubbleSort()),
            bubbleSortPoints (__bubbleSortPoints()) {}
};

#endif
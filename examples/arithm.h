#ifndef HETARCH_EXAMPLES_ARITHM_H
#define HETARCH_EXAMPLES_ARITHM_H 1

#include <dsl.h>

// Platforms like MSP430 often have no multiplication or division instructions

template<typename Platform>
class Arithmetics {
    void mydiv() {
        int x = 345, y = 23, t, shift = 1, res = 0;
        while (x > y) {
            t = y;
            while (x >= (t << 1)) { t <<= 1; shift <<= 1; }
            x -= t;
            res |= shift;
        }
    }
    template<typename T>
    Function<Platform, T> __mul() {
        Param<T> a, b;
        Local<T> x, y, t, res;
        return MakeFunction<Platform, T>("mul", Params(a, b), Seq(
                x.Assign(a), y.Assign(b), res.Assign(0),
                While(y > 0, Seq(
                        If((y & Const<T>(1)) != 0, res.Assign(res + x)),
                        x.Assign(x << 1),
                        y.Assign(y >> 1)
                )),
                res
        ));
    };
    template<typename T>
    Function<Platform, T> __div() {
        Param<T> a, b;
        Local<T> x, y, t, shift, res;
        return MakeFunction<Platform, T>("div", Params(a, b), Seq(
                x.Assign(a), y.Assign(b), res.Assign(0),
                While(x > y, Seq(
                    t.Assign(y),
                    shift.Assign(1),
                    While(x >= (t << 1), Seq(
                        t.Assign(t << 1), shift.Assign(shift << 1), Void()
                    )),
                    x.Assign(x - t),
                    res.Assign(res | shift)
                )),
                res
        ));
    }
public:
    Function<Platform, uint16_t> div16;
    Function<Platform, uint32_t> div32;
    Function<Platform, uint16_t> mul16;
    Function<Platform, uint32_t> mul32;
    Arithmetics(mem::MemMgr<Platform> &memMgr,
                cg::ICodeGen &codeGen,
                conn::IConnection<Platform> &conn):
            div16(__div<uint16_t>()),
            div32(__div<uint32_t>()),
            mul16(__mul<uint16_t>()),
            mul32(__mul<uint32_t>())
    {
        div16.compile(codeGen, memMgr); div16.Send(conn);
        div32.compile(codeGen, memMgr); div32.Send(conn);
        mul16.compile(codeGen, memMgr); mul16.Send(conn);
        mul32.compile(codeGen, memMgr); mul32.Send(conn);
    }
};

#endif
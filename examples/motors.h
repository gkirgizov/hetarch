#ifndef HETARCH_EXAMPLES_MOTORS_H
#define HETARCH_EXAMPLES_MOTORS_H 1

#include <iostream>

#include "dsl/base.h"
#include "../include/cg.h"
#include "../include/MemMgr.h"
#include "../examples/example_utils.h

using namespace hetarch;
using namespace hetarch::dsl;

#include <memregmap.inc>

#define OUTMOD_0               (0*0x20u)      /* PWM output mode: 0 - output only */
#define OUTMOD_1               (1*0x20u)      /* PWM output mode: 1 - set */
#define OUTMOD_2               (2*0x20u)      /* PWM output mode: 2 - PWM toggle/reset */
#define OUTMOD_3               (3*0x20u)      /* PWM output mode: 3 - PWM set/reset */
#define OUTMOD_4               (4*0x20u)      /* PWM output mode: 4 - toggle */
#define OUTMOD_5               (5*0x20u)      /* PWM output mode: 5 - Reset */
#define OUTMOD_6               (6*0x20u)      /* PWM output mode: 6 - PWM toggle/set */
#define OUTMOD_7               (7*0x20u)      /* PWM output mode: 7 - PWM reset/set */

DSL_STRUCT(PwmOutput) {
        RField<uint16_t, true> cnt;
        RField<uint16_t, true> crxMode;
        PwmOutput(const RValueBase<PwmOutput> *base): RStruct<PwmOutput>(base), cnt(*this), crxMode(*this) {
            cnt.Init(this), crxMode.Init(this);
        }
};

const int PwmPinOutSize = 2;

DSL_STRUCT(PwmPin) {
        RField<uint8_t> idx;
        RField<PwmOutput[PwmPinOutSize]> out;
        PwmPin(const RValueBase<PwmPin> *base): RStruct<PwmPin>(base), idx(*this), out(*this)
        {
            idx.Init(this), out.Init(this);
        }
};

DSL_STRUCT(PwmMotor) {
        RField<PwmPin[2]> pins;
        RField<uint16_t> period;
        RField<int8_t> percent;
        RField<uint16_t, true> hardwareDefence;
        PwmMotor(const RValueBase<PwmMotor> *base):
                RStruct<PwmMotor>(base), pins(*this), period(*this), percent(*this), hardwareDefence(*this)
        {
            pins.Init(this), period.Init(this), percent.Init(this); hardwareDefence.Init(this);
        }
};

#define MIN_DUTY_US		5

template<typename Platform>
inline std::tuple<> prepare(cg::ICodeGen &cg, mem::MemMgr<Platform> &memMgr) {

    const uint16_t LOGIC_LOW = (~OUTMOD_7 | OUTMOD_5);
    const uint16_t LOGIC_HIGH = (~OUTMOD_7 | OUTMOD_1);

    Global<Platform, PwmMotor[4]> pwmMotor(20);

    Param<PwmPin*> pin;
    Param<uint16_t> p_lowValue, p_highValue;

    auto setPinPWMRange = MakeFunction<Platform, void>("setPinPWMRange", Params(pin, p_lowValue, p_highValue), Seq(
            Fields(Fields(*pin).out[0]).cnt.Assign(p_highValue),
            Fields(Fields(*pin).out[0]).crxMode.Assign(LOGIC_LOW),
            Fields(Fields(*pin).out[1]).cnt.Assign(p_lowValue),
            Fields(Fields(*pin).out[1]).crxMode.Assign(LOGIC_HIGH), Void()
    ));

    Param<uint16_t> crxMode;
    auto setPinPWMValue = MakeFunction<Platform, void>("setPinPWMValue", Params(pin, crxMode), Seq(
            Fields(Fields(*pin).out[0]).cnt.Assign(0),
            Fields(Fields(*pin).out[0]).crxMode.Assign(crxMode),
            Fields(Fields(*pin).out[1]).cnt.Assign(0),
            Fields(Fields(*pin).out[1]).crxMode.Assign(crxMode), Void()
    ));

    Param<uint16_t*> crrx, cctrlx, taxr;
    Local<uint8_t> idx;
    auto updateValueCTLx = MakeFunction<Platform, void>("updateValueCTLx", Params(cctrlx, pin), Seq(
        idx.Assign(Fields(*pin).idx),
        cctrlx.Deref().Assign(cctrlx.Deref() | Const<uint16_t>(OUTMOD_7)),
        cctrlx.Deref().Assign(cctrlx.Deref() & Fields(Fields(pin.Deref()).out[idx]).crxMode),
        Fields(pin.Deref()).idx.Assign((idx + 1) % PwmPinOutSize),
        Void()
    ));

    auto updateValuesTimeout = MakeFunction<Platform, void>("updateValuesTimeout", Params(crrx, cctrlx, pin, taxr), Seq(
        updateValueCTLx.Call(cctrlx, pin),
        crrx.Deref().Assign(taxr.Deref() + (uint16_t)2), Void()
    ));

    Param<uint8_t> number;

    auto updatePower = MakeFunction<uint16_t, void>("updatePower", Params(number),
        Switch<void, uint8_t>(number)
            .Case(0, Seq(
                updateValuesTimeout.Call(&TA0CCR0, &TA0CCTL0, &Fields(pwmMotor[0]).pins[0], &TA0R),
                updateValuesTimeout.Call(&TA0CCR1, &TA0CCTL1, &Fields(pwmMotor[0]).pins[1], &TA0R)
            ))
            .Case(1, Seq(
                updateValuesTimeout.Call(&TA0CCR2, &TA0CCTL2, &Fields(pwmMotor[1]).pins[0], &TA0R),
                updateValuesTimeout.Call(&TA0CCR3, &TA0CCTL3, &Fields(pwmMotor[1]).pins[1], &TA0R)
            ))
            .Case(2, Seq(
                updateValuesTimeout.Call(&TA1CCR0, &TA1CCTL0, &Fields(pwmMotor[2]).pins[0], &TA1R),
                updateValuesTimeout.Call(&TA1CCR1, &TA1CCTL1, &Fields(pwmMotor[2]).pins[1], &TA1R)
            ))
            .Case(3, Seq(
                updateValuesTimeout.Call(&TA2CCR0, &TA2CCTL0, &Fields(pwmMotor[3]).pins[0], &TA2R),
                updateValuesTimeout.Call(&TA2CCR1, &TA2CCTL1, &Fields(pwmMotor[3]).pins[1], &TA2R)
            ))
            .Default());
    Local<uint16_t> period, highValue, lowValue;
    Local<uint32_t> p32;
    Param<int8_t> percent;

    // TODO forgotten MAX
    auto setDutyPercent = MakeFunction<uint16_t, void>("setDutyPercent", Params(number, percent), Seq(
            period.Assign(Fields(pwmMotor[number]).period),
            If((percent > 0) && (percent < 100), Seq(
                p32.Assign(period.Cast<uint32_t>()),
                Fields(pwmMotor[number]).percent.Assign(percent),
                highValue.Assign((p32 * percent.Cast<uint32_t>() / 100).Cast<uint16_t>()),
                highValue.Assign(If(highValue > MIN_DUTY_US, highValue, Const<uint16_t>(MIN_DUTY_US))),
                lowValue.Assign(period - highValue),
                setPinPWMRange.Call(&Fields(pwmMotor[number]).pins[0], lowValue, highValue),
                setPinPWMValue.Call(&Fields(pwmMotor[number]).pins[1], Const<uint16_t>(LOGIC_LOW))

            ), If((percent < 0) && (percent > -100), Seq(
                p32.Assign(period.Cast<uint32_t>()),
                Fields(pwmMotor[number]).percent.Assign(percent),
                highValue.Assign((p32 * -percent.Cast<uint32_t>() / 100).Cast<uint16_t>()),
                highValue.Assign(If(highValue > MIN_DUTY_US, highValue, Const<uint16_t>(MIN_DUTY_US))),
                lowValue.Assign(period - highValue),
                setPinPWMRange.Call(&Fields(pwmMotor[number]).pins[1], lowValue, highValue),
                setPinPWMValue.Call(&Fields(pwmMotor[number]).pins[0], Const<uint16_t>(LOGIC_LOW))

            ), If(percent == 0, Seq(
                setPinPWMValue.Call(&Fields(pwmMotor[number]).pins[0], Const<uint16_t>(LOGIC_LOW)),
                setPinPWMValue.Call(&Fields(pwmMotor[number]).pins[1], Const<uint16_t>(LOGIC_LOW))

            ), If(percent == 100, Seq(
                p32.Assign(period.Cast<uint32_t>()),
                setPinPWMValue.Call(&Fields(pwmMotor[number]).pins[0], Const<uint16_t>(LOGIC_HIGH)),
                setPinPWMValue.Call(&Fields(pwmMotor[number]).pins[1], Const<uint16_t>(LOGIC_LOW))

            ), If(percent == -100, Seq(
                setPinPWMValue.Call(&Fields(pwmMotor[number]).pins[1], Const<uint16_t>(LOGIC_HIGH)),
                setPinPWMValue.Call(&Fields(pwmMotor[number]).pins[0], Const<uint16_t>(LOGIC_LOW))
            )))))),
            updatePower.Call(number)
    ));

    log(setPinPWMRange.getFunctionModuleIR());
    setPinPWMRange.compile(cg, memMgr);
    dumpBufferForDisasm(setPinPWMRange.Binary(), std::cout);

    log(setPinPWMValue.getFunctionModuleIR());
    setPinPWMValue.compile(cg, memMgr);
    dumpBufferForDisasm(setPinPWMValue.Binary(), std::cout);

    log(updateValueCTLx.getFunctionModuleIR());
    updateValueCTLx.compile(cg, memMgr);
    dumpBufferForDisasm(updateValueCTLx.Binary(), std::cout);

    log(updateValuesTimeout.getFunctionModuleIR());
    updateValuesTimeout.compile(cg, memMgr);
    dumpBufferForDisasm(updateValuesTimeout.Binary(), std::cout);

    log(updatePower.getFunctionModuleIR());
    updatePower.compile(cg, memMgr);
    dumpBufferForDisasm(updatePower.Binary(), std::cout);

    log(setDutyPercent.getFunctionModuleIR());
    setDutyPercent.compile(cg, memMgr);
    dumpBufferForDisasm(setDutyPercent.Binary(), std::cout);

    return std::make_tuple<>;
}

#endif
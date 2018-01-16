#include "gtest/gtest.h"

#include <string_view>
#include "../new/utils.h"

#include "../new/dsl/IDSLCallable.h"
#include "../new/dsl/IRTranslator.h"
#include "../new/dsl/sequence.h"


using namespace hetarch;
using namespace hetarch::dsl;


struct IRTranslatorTest: public ::testing::Test {
    explicit IRTranslatorTest()
    : irt{}
    {}

    IRTranslator irt;

    // variables should have static storage duration to be used later in
    static constexpr Var<char> a{'a', make_bsv("char1")};
    static constexpr Var<char> b{'b', make_bsv("char2")};
    static constexpr Param<int> x{69}, y{42}, tmp{}; // default parameters
//    static constexpr Param x(69), y(42), tmp(0); // why auto type deduction doesn't work??
};


TEST_F(IRTranslatorTest, compileTimeDSL) {
//    constexpr std::string_view name2{"some_string"}; // fails!
//    constexpr std::string_view name = make_bsv("some_string"); // succeeds.

    constexpr const Var<char>& a_ref = a; // fails when 'a' has non-static storage duration

    constexpr auto eassign1 = b.assign(a); // move-construct

    constexpr auto eassign2 = tmp.assign(x);
//    constexpr auto eassign3{std::move(eassign2)}; // fail to copy-construct

    constexpr auto seq = (tmp.assign(x), x.assign(y), y.assign(tmp));

    // this swap_func is not a constant expression and can't be used to initialise constexpr things
    constexpr DSLFunction<void, int, int> swap_func(
            make_bsv("swap"), x, y,
            (tmp.assign(x), x.assign(y), y.assign(tmp))
    );

//    constexpr auto ecall = swap_func.call(tmp.assign(x), tmp.assign(y)); // fails because swap_func isn't a constant expression

//    constexpr DSLFunction<void> call_swap_with_args(
//            swap_func()  // use default parameters
//    );

    static_assert("DSL constructs built succesfully at compile-time.");

    // expected to resolve successfully
    irt.accept(x);
    irt.accept(swap_func);
}
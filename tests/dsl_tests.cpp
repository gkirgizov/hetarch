#include "gtest/gtest.h"


//#include "../new/dsl/IDSLCallable.h"
#include "../new/dsl/IRTranslator.h"

#include <string_view>
#include "../new/utils.h"
#include "test_utils.h"


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
    static constexpr Var<int> x{69, make_bsv("la-la")}, y{42}, tmp{};
};


template<typename TdRet, typename TdBody, typename... Args>
bool translate_verify(IRTranslator& irt, const DSLFunction<TdRet, TdBody, Args...> &f) {
    std::cout << "Translating DSLFunction: " << f.name() << std::endl;
    auto module = irt.translate(f);

    bool verified_ok = utils::verify_module(module);
    std::cout << "verified: " << f.name() << ": " << std::boolalpha << verified_ok << std::endl;
    module.get().dump();
    return verified_ok;
}


TEST_F(IRTranslatorTest, compileDSLCommon) {

//    constexpr std::string_view name2{"some_string"}; // fails!
//    constexpr std::string_view name = make_bsv("some_string"); // succeeds.

    // test in-dsl takeAddr and deref
//    Ptr dsl_ptr = tmp.takeAddr();
//    EDeref derefd = *dsl_ptr;
//    assert(&tmp == &derefd.x);

    DSLFunction empty_test(
            "empty",
            Return()
    );

    DSLFunction pass_through(
            "pass_through",
            MakeFunArgs(tmp),
            Return(tmp)
    );

    DSLFunction assign_test(
            "swap",
            MakeFunArgs(x, y),
            (tmp = x, x = y, y = tmp),
            Return()
    );

    DSLFunction call_test(
            "call_thing",
            MakeFunArgs(x),
            tmp = pass_through(x),
            Return(tmp)
    );

    EXPECT_TRUE(translate_verify(irt, empty_test));
    EXPECT_TRUE(translate_verify(irt, pass_through));
    EXPECT_TRUE(translate_verify(irt, assign_test));
    EXPECT_TRUE(translate_verify(irt, call_test));
}


TEST_F(IRTranslatorTest, compileDSLOperations) {
    Var<int32_t> xi{1}, yi{3};
    Var<uint64_t> xui{1}, yui{3};
    Var xf{2.0}, yf{4.0};

    DSLFunction simple_add_test(
            "add2",
            MakeFunArgs(x, y),
            Return(x + y)
    );

    DSLFunction cmp_test(
            "",
            MakeFunArgs(x, y),
            (
//                    xi == yi, xi == yf, xf == yi, xf == yf,
                    xui == yi, xi == yui, xf == yui, xf == yf
//                    xi < yi, xi < yf, xf < yi, xf < yf
            ),
            Return()
    );


    EXPECT_TRUE(translate_verify(irt, simple_add_test));
    EXPECT_TRUE(translate_verify(irt, cmp_test));
}


TEST_F(IRTranslatorTest, compileDSLArray) {
    std::array arr_init = {1, 2, 3, 5};
    Array<int, 4> arr{arr_init};

    DSLFunction array_access_test(
            "try_arr",
            MakeFunArgs(x, y), (
                tmp = arr[x] + arr[y]
            ), Return(tmp)
    );

    DSLFunction array_assign_test(
            "try_arr",
            MakeFunArgs(x, y),
            arr[x] = arr[y]
            , Return()
    );

    EXPECT_TRUE(translate_verify(irt, array_access_test));
    EXPECT_TRUE(translate_verify(irt, array_assign_test));
}


TEST_F(IRTranslatorTest, compileDSLControlFlow) {
    Var<bool> cond{false};
    Var<const int> zero{0};
    Var<const int> one{1};

    DSLFunction if_else_test(
            "max",
            MakeFunArgs(x, y),
            tmp = If(cond, x, y),
            Return(tmp)
    );

    DSLFunction while_test(
            "count",
            MakeFunArgs(x, y),
            While(x == zero,
                  x = x - one
            ),
            Return()
    );

    EXPECT_TRUE(translate_verify(irt, if_else_test));
    EXPECT_TRUE(translate_verify(irt, while_test));
}

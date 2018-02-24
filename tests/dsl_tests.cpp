#include "gtest/gtest.h"

#include <cstdint>

//#include "../new/dsl/IDSLCallable.h"
#include "../new/dsl/IRTranslator.h"
#include "../new/dsl/dsl_meta.h"

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

//    static constexpr Var<char> a{'a', make_bsv("char1")};
//    static constexpr Var<char> b{'b', make_bsv("char2")};
    Var<int> x{69, make_bsv("la-la")}, y{42}, tmp{};
};


template<typename TdBody, typename... Args>
bool translate_verify(IRTranslator& irt, const DSLFunction<TdBody, Args...> &f) {
    std::cerr << "Translating DSLFunction: " << f.name() << std::endl;
    auto module = irt.translate(f);

    bool verified_ok = utils::verify_module(module);
    std::cerr << "verified: " << f.name() << ": " << std::boolalpha << verified_ok << std::endl;
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
            (tmp = x, x = y, y = tmp, Return())
    );

    DSLFunction call_test(
            "call_thing",
            MakeFunArgs(x),
            (tmp = pass_through(x), Return(tmp))
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
                    , Return()
            )
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
            , Return(tmp))
    );

    DSLFunction array_assign_test(
            "try_arr",
            MakeFunArgs(x, y),
            (arr[x] = arr[y]
            , Return())
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
            (tmp = If(cond, x, y),
            Return(tmp))
    );

    DSLFunction while_test(
            "count",
            MakeFunArgs(x, y),
            (While(x == zero,
                   x = x - one
            ), Return())
    );

    EXPECT_TRUE(translate_verify(irt, if_else_test));
    EXPECT_TRUE(translate_verify(irt, while_test));
}

TEST_F(IRTranslatorTest, genericDSLReferenceConsistency) {

//    auto test_glambda = [](auto x, auto y) { return x+y; };
//    auto test_lambda = [](int x, int y) { return x+y; };
//    make_dsl_callable_test(test_glambda);

    auto test_code_generator = [](auto&& x, auto&& y){
//        return (x, (x, y));
        return (x, y);
    };
    auto generic_test = make_generic_dsl_fun(test_code_generator);

    auto call_generic_generator = [&](auto&& x, auto&& y){
        return generic_test(x, y);
    };
    DSLFunction call_generic_fun = make_dsl_fun_from_arg_types<Var<int>, Var<int>>(call_generic_generator);

    auto dbg_printer1 = [&](const auto& args, const auto& body) {
        std::cerr << std::endl;
        PR_CERR_VAL_TY(args);
        PR_CERR_VAL_TY(body);
        auto r00 = &std::get<0>(args);
        auto r01 = &std::get<1>(args);
        auto r10 = &body.returnee.e1;
        auto r11 = &body.returnee.e2;
        std::cerr << std::hex << r00 << "; " << r01 << std::endl;
        std::cerr << std::hex << r10 << "; " << r11 << std::endl;
        EXPECT_TRUE(r10 == r00 && r11 == r01) << "args used in f.body aren't the f.args! dangling references in f.body!";
    };

    auto& cf1 = call_generic_fun.body.returnee.callee;
    dbg_printer1(cf1.args, cf1.body);

    DSLFunction cf2 = make_dsl_fun_from_arg_types<Var<int>, Var<int>>(test_code_generator);
    dbg_printer1(cf2.args, cf2.body);
    std::cerr << "...how it must be (same addresses in both lines)." << std::endl;

    ECall call1 = makeCall(cf2, x, y);
    auto& cf3 = call1.callee;
    dbg_printer1(cf3.args, cf3.body);

    // TODO: HERE IS THE PROBLEM --
    //  when moving DSLFunction its 'args' are copied/moved whatever;
    //  the problem is when DSLFunction OWNS its args (i.e. stores by value; not by ref)
    //      check this.
    //  but body still refers to the old 'args'
//    ECall call2 = makeCall(std::move(cf2), x, y);
//    auto& cf4 = call2.callee;
//    dbg_printer1(cf4.args, cf4.body);

    ECall call3 = generic_test(x, y);
    auto& cf5 = call3.callee;
    dbg_printer1(cf5.args, cf5.body);
}


TEST_F(IRTranslatorTest, genericDSLFunctions) {

    auto id_generator = [](auto&& x) { return x; };

    auto max_code_generator = [](auto&& x, auto&& y) {
//        auto&& c = x > y; // goes out of scope
        auto m = If(x > y, x, y);
        return m;
    };

    auto tst_generator = [](auto&& x) {
        return If(!x, DSLConst(false), DSLConst(true));
    };

    auto dsl_metagenerator = [](bool need_complex_logic) {
        return [=](auto&& dsl_var1, Var<int> dsl_var2) {
            if (need_complex_logic) {

                auto e = If(dsl_var1 == DSLConst(0),
                            dsl_var1 = dsl_var1 - DSLConst(1),
                            (dsl_var2 = dsl_var1, dsl_var2)
                );
                return e;

            } else {
                return DSLConst(42);
            }
        };
    };


//    constexpr auto dsl_id = make_dsl_fun_from_arg_types< DSLConst<int> >(id_generator);
    auto dsl_id = make_dsl_fun_from_arg_types< DSLConst<int> >(id_generator);
    PR_CERR_VAL_TY(dsl_id);
    constexpr auto tmp_sum = Var{10} + Var{11};
    auto dsl_fun_max = make_dsl_fun_from_arg_types< Var<int>, decltype(tmp_sum) >(max_code_generator);
    PR_CERR_VAL_TY(dsl_fun_max)
//    auto ecall2 = generic_dsl_max(Var{10}, Var{2} + Var{3});
//    PR_CERR_VAL_TY(ecall2)

    auto generic_id = make_generic_dsl_fun(id_generator);
    auto generic_tst = make_generic_dsl_fun(tst_generator);
    auto generic_dsl_max = make_generic_dsl_fun(max_code_generator);

    auto call_generic_generator = [&](auto&& x, auto&& y){
        return generic_id(x);
//        return generic_tst(y);
//        return generic_dsl_max(x, y);
    };
    DSLFunction call_generic_fun = make_dsl_fun_from_arg_types<Var<int>, Var<int>>(call_generic_generator);

    std::cerr << std::endl << std::endl;
    EXPECT_TRUE(translate_verify(irt, dsl_id));
    EXPECT_TRUE(translate_verify(irt, dsl_fun_max));
    EXPECT_TRUE(translate_verify(irt, call_generic_fun));

    // call generic inside usual
    // call generic inside lambda; isntantiated directly
    // generator composition
        // oh shit; i can't use Return in generators: they will be fucked up
}




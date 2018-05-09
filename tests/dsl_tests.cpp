#include "gtest/gtest.h"

#include <cstdint>

#include "../new/dsl.h"
#include "../new/dsl/IRTranslator.h"
//#include "../new/dsl/switch.h"

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

    Var<int> x{69, make_bsv("x")}, y{42, "y"}, tmp{0, "tmp"};
};


template<typename TdBody, typename... Args>
bool translate_verify(IRTranslator& irt, const Function<TdBody, Args...> &f) {
    std::cerr << "Translating Function: " << f.name() << std::endl;
    auto module = irt.translate(f);

    bool verified_ok = utils::verify_module(module);
    std::cerr << "verified: " << f.name() << ": " << std::boolalpha << verified_ok << std::endl;
    module.get().dump();
    return verified_ok;
}


TEST_F(IRTranslatorTest, compileDSLCommon) {
    Function empty_test(
            "empty",
            VoidExpr{}
    );

    Function pass_through(
            "pass_through",
            MakeFunArgs(tmp),
            (tmp, tmp)
    );

    Function simple_assign_test(
            "swap",
            MakeFunArgs(x, y),
            (tmp = x, x = y, y = tmp, Unit)
    );

    Function call_test(
            "call_thing",
            MakeFunArgs(x),
            (tmp = pass_through(x), tmp)
    );

    Ptr<Var<int>> xi_ptr{};
    Function invalid_call_test(
            "invalid_call",
            MakeFunArgs(),
            (
//                    pass_through(xi_ptr), // this must fail at compilation
                    Unit
            )
    );

    EXPECT_TRUE(translate_verify(irt, empty_test));
    EXPECT_TRUE(translate_verify(irt, pass_through));
    EXPECT_TRUE(translate_verify(irt, simple_assign_test));
    EXPECT_TRUE(translate_verify(irt, call_test));
}

TEST_F(IRTranslatorTest, compileDSLOperations) {
    Var<int32_t> xi{1}, yi{3};
    Var<uint64_t> xui{1}, yui{3};
    Var xf{2.0}, yf{4.0};
    Var<const int32_t> cxi{1}, cyi{3};

    Function simple_ops_test(
            "simple_ops",
            MakeFunArgs(x, y),
            (
                    x + y, x - y, x * y,
                    Unit
            )
    );

    Function assign_ops_test(
            "assign_ops",
            MakeFunArgs(x, y),
            (
                    x += y, x -= y, y *= x, ++x, --y,
                    Unit
            )
    );

    Function cmp_test(
            "cmp_test",
            MakeFunArgs(x, y),
            (
//                    xi == yi, xi == yf, xf == yi, xf == yf,
                    xui == yi, xi == yui, xf == yui, xf == yf
//                    xi < yi, xi < yf, xf < yi, xf < yf
                    , Unit
            )
    );

    Function assign_test(
            "assign_test",
            MakeFunArgs(x, y),
            (
                    xi = yi, xui = yi, xi = yui,
//                    cxi = cyi, // fails at compilation
                    xf = yf, xf = yui, xui = yf,
                    Unit
            )
    );

    // division and remainder (not modulo)
    Function div_rem_test(
            "div_rem",
            MakeFunArgs(),
            (
                            xi / yi, xi % yi,
                            xui / yui, xui % yui,
                            xui / yi, xui % yi,
                            xi / yui, xi % yui,

                            xf / yf, xf % yf,
                            xf / yi, xf % yi,
                            xi / yf, xi % yf,
                            xf / yui, xf % yui,
                            xui / yf, xui % yf,

                            Unit
            )
    );

    EXPECT_TRUE(translate_verify(irt, simple_ops_test));
    EXPECT_TRUE(translate_verify(irt, assign_ops_test));
    EXPECT_TRUE(translate_verify(irt, cmp_test));
    EXPECT_TRUE(translate_verify(irt, assign_test));
    EXPECT_TRUE(translate_verify(irt, div_rem_test));
}

TEST_F(IRTranslatorTest, compilePtr) {
    Var<int32_t> xi{1, "xi"}, yi{3, "yi"};
    Var<uint16_t> zi{9, "zi"};
//    Ptr p_xi{xi};
//    Ptr p_yi{yi};
//    Ptr pp_xi{p_xi};
    Ptr<decltype(xi)> p_xi{};
    Ptr<decltype(yi)> p_yi{};
    Ptr<decltype(p_xi)> pp_xi{};

    Function ptr_test(
            "ptr_test",
            MakeFunArgs(yi),
            (
                    p_xi = xi.takeAddr(),
                    xi == *p_xi,
                    *p_xi = yi,
                    xi == yi,
                    p_xi = p_yi,
                    *p_xi == *p_yi,
                    p_xi
//                    Unit
            )
    );

    Function ptr_arith_test(
            "ptr_arithmetic",
            MakeFunArgs(p_yi, pp_xi),
            (
                    p_yi = xi.takeAddr(),
//                    p_yi == p_xi,
                    p_yi = p_xi + Lit(5),
                    p_yi += Lit(5),
                    p_yi = p_xi - Lit(5),
                    p_yi -= Lit(5),
                    ++p_yi, --p_yi,
                    Unit
            )
    );

//    static_assert( std::is_convertible_v< uint16_t*, uint32_t* > );

    Ptr<Var<volatile uint32_t>> rp_vui{0x20cff, "rp_vui"};
    Ptr<Var<uint32_t>> rp_vui2{};
    Function raw_ptr_arith_test(
            "raw_ptr_arithmetic",
            MakeFunArgs(rp_vui2),
            (
                    *rp_vui |= Lit(1u),
                    xi = *(rp_vui + Lit(3)),
                    Unit
            )
    );

    EXPECT_TRUE(translate_verify(irt, ptr_test));
    EXPECT_TRUE(translate_verify(irt, ptr_arith_test));
    EXPECT_TRUE(translate_verify(irt, raw_ptr_arith_test));
}

TEST_F(IRTranslatorTest, compileDSLArray) {
    std::array arr_init = {1, 2, 3, 5};
    Array<Var<int>, 4> arr{arr_init};

    Function array_access_test(
            "try_arr",
            MakeFunArgs(x, y), (
                tmp = arr[x] + arr[y]
            , tmp)
    );

    Function array_assign_test(
            "try_arr",
            MakeFunArgs(x, y),
            (arr[x] = arr[y]
            , Unit)
    );

    std::array arr2_init = {0, 22, 33, 55};
    Array<Var<int>, 4> arr2{arr2_init};
    Function array_assign2_test(
            "array_assign2",
            MakeFunArgs(),
            (arr = arr2
            , Unit)
    );

    Function array_return_test(
            "array_return",
            MakeFunArgs(arr),
            (arr2[Lit(1)] = arr[Lit(0)], arr2)
    );

//    std::array< std::array<int, 3>, 2 > nested_init = {{{10, 20, 30}, {11, 22, 33}}};
//    Array<Array<Var<int>, 3>, 2> nested{nested_init};
    Array<Array<Var<int>, 3>, 2> nested{{ {{10, 20, 30}, {11, 22, 33}} }};
    Function array_nested_test(
            "array_nested",
            MakeFunArgs(nested),
            (tmp = nested[Lit(1)][Lit(2)], tmp)
    );

    EXPECT_TRUE(translate_verify(irt, array_access_test));
    EXPECT_TRUE(translate_verify(irt, array_assign_test));
    EXPECT_TRUE(translate_verify(irt, array_assign2_test));
    EXPECT_TRUE(translate_verify(irt, array_return_test));
    EXPECT_TRUE(translate_verify(irt, array_nested_test));
}

TEST_F(IRTranslatorTest, compileDSLStruct) {
    std::tuple s1_init = {1, 2, 3.14159, true};
    std::tuple s2_init = {-11, -22, -2.742, false};

    using StructT = Struct< Var<int>, Var<int>, Var<double>, Var<bool> >;
    StructT s{s1_init};
    StructT s2{s2_init};

    Function struct_access_test(
            "try_struct",
            MakeFunArgs(x, y), (
                    tmp = s.get<0>()
            , tmp)
    );

    Function struct_assign_test(
            "try_struct",
            MakeFunArgs(x, y),
            (s.get<0>() = s.get<1>(), Unit)
    );

    Function struct_assign2_test(
            "struct_assign2",
            MakeFunArgs(),
            (s = s2, Unit)
    );

    Function struct_return_test(
            "struct_return",
            MakeFunArgs(s),
            (s, s)
    );

    using StructComplexT = Struct< Var<bool>, Struct< Var<int>, Var<double> >, Array< Var<int>, 3 > >;
    StructComplexT s3{ {false, {42, 69.69}, {-11, -22, -33}} };
    Function struct_complex_test(
            "struct_complex",
            MakeFunArgs(s3),
            (tmp = s3.get<1>().get<0>(), tmp)
    );

    EXPECT_TRUE(translate_verify(irt, struct_access_test));
    EXPECT_TRUE(translate_verify(irt, struct_assign_test));
    EXPECT_TRUE(translate_verify(irt, struct_assign2_test));
    EXPECT_TRUE(translate_verify(irt, struct_return_test));
    EXPECT_TRUE(translate_verify(irt, struct_complex_test));
}

TEST_F(IRTranslatorTest, compileDSLControlFlow) {
    Var<bool> cond{false};
    Var<const int> zero{0};
    Var<const int> one{1};

    Function if_else_test(
            "max",
            MakeFunArgs(x, y),
            (tmp = If(cond, x, y),
            tmp)
    );

    Function while_test(
            "count",
            MakeFunArgs(x, y),
            (While(x == zero,
                   x = x - one
            ), Unit)
    );

    Function loop_not_run_test(
            "",
            MakeFunArgs(x),
            While(one == zero, x + one)
    );

    EXPECT_TRUE(translate_verify(irt, if_else_test));
    EXPECT_TRUE(translate_verify(irt, while_test));
    EXPECT_TRUE(translate_verify(irt, loop_not_run_test));
}


struct GenericsTest: public ::testing::Test {
    explicit GenericsTest() : irt{} {}

    IRTranslator irt;
    Var<int> x{69, make_bsv("x")}, y{42, "y"}, tmp{0, "tmp"};

};

auto id_generator = [](auto x) { return x; };
//auto id_generator = [](auto x) -> decltype(auto) { return x; };

auto max_gen = [](auto&& x, auto&& y) {
    return If(x > y, x, y);
};

auto get_generic_caller = [](const auto& callee) {
    // return generic caller of generic dsl functions
    return [&](auto&&... args) {
        return callee(args...);
    };
};


/* // leave for historical reasons..
TEST_F(GenericsTest, genericDSLReferenceConsistency) {
    auto test_code_generator = [](auto&& x, auto&& y){
        return (x, y);
    };
    auto generic_test = make_generic_dsl_fun(test_code_generator);

    auto call_generic_generator = [&](auto&& x, auto&& y){
        return generic_test(x, y);
    };
    Function call_generic_fun = make_dsl_fun<Var<int>, Var<int>>(call_generic_generator);

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

    Function cf2 = make_dsl_fun<Var<int>, Var<int>>(test_code_generator);
    dbg_printer1(cf2.args, cf2.body);
    std::cerr << "...how it must be (same addresses in both lines)." << std::endl;

    ECall call1 = makeCall(cf2, x, y);
    auto& cf3 = call1.callee;
    dbg_printer1(cf3.args, cf3.body);

    // TODO: HERE IS THE PROBLEM --
    //  when moving Function its 'args' are copied/moved whatever;
    //  the problem is when Function OWNS its args (i.e. stores by value; not by ref)
    //      check this.
    //  but body still refers to the old 'args'
//    ECall call2 = makeCall(std::move(cf2), x, y);
//    auto& cf4 = call2.callee;
//    dbg_printer1(cf4.args, cf4.body);

    ECall call3 = generic_test(x, y);
    auto& cf5 = call3.callee;
    dbg_printer1(cf5.args, cf5.body);
}
*/


TEST_F(GenericsTest, genericDSLFunctions1) {

    auto tst_generator = [](auto x) {
        return If(!x, Lit(false), Lit(true));
    };

    auto factorial_gen = [](auto n){
        Var<int> res{1};
        return (
                While(n > Lit(0), (
                    res *= n,
                    n -= Lit(1)
                )),
                res
        );
    };

    auto complex_code = [](Ptr<Var<uint32_t>> ptr){
        Var<uint32_t> tmp;
        return tmp = *ptr &= ~(*++ptr ^ Lit(1 << 8));
    };

    // instantiate some functions in a usual way
    auto dsl_id = make_dsl_fun< Lit<int> >(id_generator);
    PR_CERR_VAL_TY(dsl_id);
    Function dsl_factorial = make_dsl_fun< Var<unsigned> >(factorial_gen);
    Function code = make_dsl_fun< Ptr<Var<uint32_t>> >(complex_code);
//    auto dsl_fun_max = make_dsl_fun< Var<int>, Var<int> >(max_gen);
    auto& dsl_fun_max = get_or_make_dsl_fun< Var<int>, Var<int> >(max_gen);

    // try generic functions
    auto generic_dsl_max = make_generic_dsl_fun(max_gen);
    Function call_generic_fun{
            "caller_of_generics",
            MakeFunArgs(x, y),
            generic_dsl_max(x, y) + dsl_id(x)
    };

    // specialise generic caller for specific arguments (and correct number of arguments)
//    Function call_max = make_dsl_fun<Var<int>, Var<int>>(get_generic_caller(generic_dsl_max), "call_max"); // fails!
    Function call_max = make_dsl_fun<Var<int>, Var<int>>(get_generic_caller(generic_dsl_max));

    std::cerr << std::endl << std::endl;
    EXPECT_TRUE(translate_verify(irt, dsl_id));
    EXPECT_EQ(0, free_repo(dsl_id)) << "dsl fun not from repo can't be deleted from template repo!";
    EXPECT_TRUE(translate_verify(irt, dsl_fun_max));
    EXPECT_EQ(1, free_repo(dsl_fun_max)) << "but dsl fun from repo can be deleted from template repo!";

    EXPECT_TRUE(translate_verify(irt, dsl_factorial));
    EXPECT_TRUE(translate_verify(irt, code));
    EXPECT_TRUE(translate_verify(irt, call_generic_fun));
    EXPECT_TRUE(translate_verify(irt, call_max));

    // try instantiate function with invalid arguments (fail at compile time)
//    Function max_for_arrays = make_dsl_fun< Array<Var<int>, 1>, Array<Var<int>, 1> >( max_gen );
}


TEST_F(GenericsTest, genericDSLFunctions2) {
    auto reduce_sum_gen = [&](auto ...xs) {
        // Using C++17 fold expression
        return (... + xs);
    };

    // Reduces arbitrary expressions (e.g. other generators)
    auto get_reducer = [](const auto& binary_op) {
        return [&](auto x1, auto... xs) {
            // Using C++17 fold expression
            return ( (x1 = binary_op(x1, xs)), ... );
            // Redundant assignments will be optimized out later LLVM
        };
    };

    Function sum3 = make_dsl_fun<
            Var<float>, Var<float>, Var<int>
    >(reduce_sum_gen);

    EXPECT_TRUE(translate_verify(irt, sum3));


    auto add2_gen = [](auto&& x1, auto&& x2) { return x1 + x2; };
    auto sum_reducer_generator_v2 = get_reducer(add2_gen);
    Function sum3_v2 = make_dsl_fun<
            Var<float>, Var<float>, Var<int>
    >(sum_reducer_generator_v2);

    EXPECT_TRUE(translate_verify(irt, sum3_v2));


    // todo: check: may not be easily optimised; so might be not quite effective due to extra assignments from generic reducer
    auto max_vararg_gen = get_reducer(max_gen);
    Function max3 = make_dsl_fun<
            Var<float>, Var<float>, Var<float>
    >(max_vararg_gen);

    EXPECT_TRUE(translate_verify(irt, max3));

    auto generic_max = make_generic_dsl_fun(max_gen);
    // Will instantiate 2 max functions: for int and for float
    auto max4_gen = [&](auto x1, auto x2, auto x3, auto x4) {
        return generic_max(
                generic_max(x1, x2),
                Cast<Var<float>>(generic_max(x3, x4))
        );
    };
    Function max4 = make_dsl_fun<
            Var<float>, Var<float>, Var<int>, Var<int>
    >(max4_gen);

    EXPECT_TRUE(translate_verify(irt, max4));
}

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
    static constexpr FunArg<int> x{69}, y{42}, tmp{}; // default parameters
//    static constexpr Param x(69), y(42), tmp(0); // why auto type deduction doesn't work??
};


TEST_F(IRTranslatorTest, compileTimeDSL) {

//    dsl2::test_something();
//    return;

//    constexpr std::string_view name2{"some_string"}; // fails!
//    constexpr std::string_view name = make_bsv("some_string"); // succeeds.

    constexpr const Var<char>& a_ref = a; // fails when 'a' has non-static storage duration

//    constexpr auto eassign1 = b.assign(a); // move-construct
//    constexpr auto eassign2 = tmp.assign(x);
//    constexpr auto eassign3{std::move(eassign2)}; // fail to copy-construct

//    auto seq0 = (tmp.assign(tmp.assign(x)), x.assign(y), y.assign(tmp));
//    Seq seq = (tmp = tmp = x, x = y, y = tmp);
//    const Expr<int> &seq_expr = seq;
//    seq.toIR(irt);

//    DSLFunction void_fun(
//            make_bsv("void_fun"),
//            Return()
//    );

    // test type deduction and tuple ctor
//    std::cout << "tmp  type: " << utils::type_name<decltype(tmp)>() << std::endl;
//    std::tuple test1{tmp};
//    FunArgs<int> test2{tmp};
//    std::cout << "test type: " << utils::type_name<decltype(test1)>() << std::endl;
//    std::cout << "test type: " << utils::type_name<decltype(test2)>() << std::endl;

    // test in-dsl takeAddr and deref
    Ptr dsl_ptr = tmp.takeAddr();
    EDeref derefd = *dsl_ptr;
    assert(&tmp == &derefd.x);


    std::cout << "pass_through def" << std::endl;
    DSLFunction pass_through(
            FunArgs<int>(tmp),
            Return(tmp)
    );
    pass_through.rename(make_bsv("pass_through"));

    std::cout << "test1 def" << std::endl;
    DSLFunction test1(
            FunArgs<int, int, int>(x, y, tmp),
            tmp = pass_through(x),
            Return(tmp)
    );
    test1.rename("test1");

    DSLFunction swap_func(
            FunArgs<int, int>(x, y),
            (tmp = x, x = y, y = tmp, pass_through(tmp)),
            Return(x + y)
//            Return()
    );
    swap_func.rename("swap");

    std::array arr_init = {1, 2, 3, 5};
    Array<int, 4> arr{arr_init};
    std::cout << "try_arr def" << std::endl;
    DSLFunction try_arr(
            FunArgs<int, int>(x, y),
            Return(arr[x] + arr[y])
    );



    std::cout << "end of dsl functions definitions" << std::endl;


//    EBinOp<BinOps::Add, int> e1{x, y};
//    std::cout << &e1.rhs << typeid(e1.rhs).name() << std::endl;
//    EBinOp<BinOps::Add, int> e2 = x + y;
//    std::cout << "some " << typeid(e2.rhs).name() << std::endl;


//    std::cout << "translating " << pass_through.name() << std::endl;
//    irt.translate(pass_through);
    std::cout << "translating " << test1.name() << std::endl;
    irt.translate(test1);
    std::cout << "translating " << swap_func.name() << std::endl;
    irt.translate(swap_func);
    std::cout << "translating " << try_arr.name() << std::endl;
    irt.translate(try_arr);

    std::cout << "end translation" << std::endl;
}
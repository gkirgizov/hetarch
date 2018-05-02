#include "gtest/gtest.h"

#include <memory>
#include <iostream>
#include <fstream>
#include <vector>
#include <utility>
#include <unistd.h>

//#include "llvm/IR/Module.h"
//#include "llvm/IR/LLVMContext.h"

#include "../new/supportingClasses.h"
#include "../new/CodeGen.h"
#include "../new/CodeLoader.h"
#include "../new/MemManager.h"
#include "../new/Executor.h"
#include "../new/conn/IConnection.h"
#include "../new/conn/PosixConn.h"
#include "../new/conn/TCPConnImpl.h"
#include "../new/conn/SerialConnImpl.h"

#include "../new/dsl.h"

#include "test_utils.h"
#include "mocks.h"


using namespace std;
using namespace llvm;
using namespace hetarch;
using namespace hetarch::mem;
using namespace hetarch::dsl;


using addr_t = HETARCH_TARGET_ADDRT;

const string ress{TESTS_RES_DIR};
string main_entry_ll(ress + "main.ll");
string do_nothing_ll(ress + "do_nothing.ll");
string ir0{ress + "self_sufficient.ll"};
string ir1{ress + "part1.ll"};
string ir2{ress + "part2.ll"};


namespace {

struct cg_config {
    std::string triple;
    std::string cpu;
    std::string fpu;
    std::string host;
    std::string port;

    CodeGen::FloatABI float_abi;
    bool is_thumb;
};

CodeGen::FloatABI determine_float_abi(std::string spec) {
    if (spec.find("hard") != std::string::npos) {
        return CodeGen::FloatABI::Hard;
    } else if (spec.find("soft") != std::string::npos) {
        return CodeGen::FloatABI::Soft;
    } else {
        return CodeGen::FloatABI::Default;
    }
}

bool is_thumb(const std::string& triple) {
    return (triple.find("arm") != std::string::npos ||
            triple.find("thumb") != std::string::npos) ;
}

cg_config read_config() {
    std::ifstream test_config{ress + "test_config.txt"};

    cg_config cfg;
    std::string float_abi;

    test_config >> cfg.triple >> cfg.cpu >> cfg.fpu >> float_abi >> cfg.host >> cfg.port;
    cfg.is_thumb = is_thumb(cfg.triple);
    cfg.float_abi = determine_float_abi(float_abi);

    std::cerr << "Tests: using"
              << ": host: " << cfg.host
              << "; port: " << cfg.port
              << "; triple: " << cfg.triple
              << "; cpu: " << cfg.cpu
              << "; fpu: " << cfg.fpu
              << "; float_abi: " << cfg.float_abi
              << "; is_thumb: " << cfg.is_thumb
              << std::endl;
    return cfg;
}

}


TEST(utilsTest, loadModule) {
    ifstream ir_stream(ir1);

    ASSERT_TRUE(ir_stream);

    LLVMContext ctx;
    auto m = utils::loadModule(ir_stream, ctx);

    ASSERT_TRUE(m);
}


TEST(IRModuleTest, constructible) {
    LLVMContext ctx;

    IIRModule irm(utils::loadModule(ir0, ctx));

    ASSERT_TRUE(irm);
}


class CodeGenTest : public ::testing::Test {
public:
    explicit CodeGenTest()
            : config{read_config()}
            , codeGen(config.triple, config.cpu, config.fpu, config.float_abi)
//            , codeGen("x86_64-unknown-linux-gnu")
            , optLvl{CodeGen::OptLvl::O0}
            , main_entry{utils::loadModule(main_entry_ll, ctx), "main"}
            , self_sufficient{utils::loadModule(ir0, ctx), "pow_naive"}
            , part1{utils::loadModule(ir1, ctx), "square"}
            , part2{utils::loadModule(ir2, ctx), "pow_smart"}
            , irt{config.is_thumb}
    {}

    const cg_config config;
    LLVMContext ctx{};
//    IIRModule main_entry;
    IRModule<VoidExpr> main_entry;
    IRModule<Var<int>, Var<int>, Var<int>> self_sufficient;
    IRModule<Var<int>, Var<int>> part1;
    IRModule<Var<int>, Var<int>, Var<unsigned int>> part2;

    IRTranslator irt;
    CodeGen codeGen;
    CodeGen::OptLvl optLvl;
};


TEST_F(CodeGenTest, trivialLink) {
    ASSERT_TRUE(CodeGen::link(self_sufficient));
    ASSERT_TRUE(self_sufficient);
}

TEST_F(CodeGenTest, fewModulesLink) {
    // just to be sure with pointer semantic
    auto m1 = utils::loadModule(ir0, ctx);
    EXPECT_TRUE(m1);
    unique_ptr<Module> m2(move(m1));
    EXPECT_FALSE(m1);
    EXPECT_TRUE(m2);

    vector<IIRModule> deps;
    deps.emplace_back(move(part1));
    deps.emplace_back(move(main_entry));

    EXPECT_TRUE(CodeGen::link(part2, deps));
    EXPECT_FALSE(deps[0]) << "part1 is still valid!!!";
    EXPECT_FALSE(deps[1]) << "main_entry is still valid!!!";
    ASSERT_TRUE(part2);
}

TEST_F(CodeGenTest, trivialCompile) {
    EXPECT_TRUE(codeGen.compile(main_entry, optLvl)) << "main.ll failed to compile";
    EXPECT_TRUE(codeGen.compile(self_sufficient, optLvl)) << "self_sufficient.ll failed to compile";
}

//TEST_F(CodeGenTest, objCodeFailFindSymbol) {
//    auto objCode = codeGen.compile(self_sufficient, optLvl);
//    EXPECT_TRUE(objCode.getSymbol() == llvm::object::SymbolRef()) << "symbol shouldn't be found (due to name mangling!)";
//}

TEST_F(CodeGenTest, dependentCompile) {
    // todo: fix return value to, say, std::optional
//    EXPECT_FALSE(codeGen.compile(part2, optLvl)) << "module with unresolved symbols shouldn't compile!";

    // provide dep-s and compile succesfully
    vector<IIRModule> deps;
    deps.emplace_back(move(part1));
    deps.emplace_back(move(main_entry));
    EXPECT_TRUE(CodeGen::link(part2, deps)) << "but this case is tested by fewModulesLink!";

    auto compiled = codeGen.compile(part2, optLvl);
    EXPECT_TRUE(compiled) << "module with resolved symbols should compile!";
}


namespace hetarch {
namespace utils {

template<typename T
        , typename = typename std::enable_if_t< std::is_arithmetic_v<T> >>
std::string to_string(T x) {
    return std::to_string(x);
}

template<typename T, std::size_t N>
std::string to_string(const std::array<T, N>& x) {
    std::string s{"{"};
    if constexpr (N != 0) {
        for(auto i = 0; i < N-1; ++i) {
            s += utils::to_string(x[i]) + ", ";
        }
        s += utils::to_string(x[N-1]);

    }
    s += "}";
    return s;
}

template<typename ...Ts>
std::string to_string(const std::tuple<Ts...>& x) {
    return std::apply([&](const auto& ...item){
        std::string s = ( "{"s + ... + (utils::to_string(item) + ", ") );
        return s.replace(s.length() - 2, 2, "}");
    }, x);
}

}
}


class CodeLoaderTest : public ::testing::Test {
public:
    explicit CodeLoaderTest()
            : config{read_config()}
            , codeGen(config.triple, config.cpu, config.fpu, config.float_abi)
            , irt{config.is_thumb}
            , ctx{irt.getContext()}
#ifdef HT_ENABLE_STM32
            , conn{new conn::SerialConnImpl<addr_t>{config.port.c_str()}}
#else
            , conn{new conn::TCPConnImpl<addr_t>{ config.host, static_cast<uint16_t>(std::stoul(config.port)) }}
#endif
            , echo_pretest{conn.echo("give me echo")}
            , all_mem{conn.getBuffer(0), 1024, mem::MemType::ReadWrite}
            , memMgr{all_mem}
            , exec{conn, memMgr, irt, codeGen}
            , main_entry{utils::loadModule(main_entry_ll, ctx), "main"}
            , do_nothing{utils::loadModule(do_nothing_ll, ctx), "do_nothing"}
//            , part1{utils::loadModule(ir1, ctx), "square"}
            , self_sufficient{utils::loadModule(ir0, ctx), "pow_naive"}
    {
#ifdef HT_ENABLE_STM32
        std::cerr << "target is stm32" << std::endl;
#elif HT_ENABLE_ARM
        std::cerr << "target is arm" << std::endl;
#else
        std::cerr << "target is likely x86" << std::endl;
#endif

    }

    const cg_config config;

    CodeGen codeGen;
    IRTranslator irt;
    LLVMContext& ctx;

    conn::PosixConn<addr_t> conn;
    const bool echo_pretest;

    const MemRegion<addr_t> all_mem;
    mem::MemManagerBestFit<addr_t> memMgr;

    Executor<addr_t> exec;

    IRModule<VoidExpr> main_entry;
    IRModule<VoidExpr> do_nothing;
//    IRModule<Var<int>, Var<int>> part1;
    IRModule<Var<int>, Var<int>, Var<int>> self_sufficient;


//    auto generic_caller = [&](auto&& dsl_generator, auto... args) {
    template<typename DSLGenerator, typename ...Args>
    auto generic_caller(DSLGenerator&& dsl_generator, Args... args) {
        // Make function from generator
//        dsl::Function dsl_fun = make_dsl_fun(dsl_generator, dsl_args...);
        dsl::Function dsl_fun = make_dsl_fun<to_dsl_t<std::remove_reference_t<decltype(args)>>...>(dsl_generator);

        // Translate, compile and load it
        IRModule translated = irt.translate(dsl_fun);
        bool verified_ok = utils::verify_module(translated);
        EXPECT_TRUE(verified_ok) << "failed verify with fun: " << dsl_fun.name() << std::endl;

        std::cerr << std::endl << "Initial generated IR before any passes:" << std::endl;
        translated.get().dump();

        auto optLvl = CodeGen::OptLvl::O2;
        ObjCode compiled = codeGen.compile(translated, optLvl);
        EXPECT_TRUE(compiled) << "module for fun '" << dsl_fun.name() << "' wasn't compiled okey!" << std::endl;

        std::cerr << std::endl << "IR after codeGen.compile with OptLvl=" << optLvl << ":" << std::endl;
        translated.get().dump();

        ResidentObjCode resident_fun = CodeLoader::load(conn, memMgr, mem::MemType::ReadWrite,
                                                        irt, codeGen, compiled);

        // Call it with some args
        auto result = exec.call(resident_fun, args...);
        return result;
    };

    template<typename DSLFun>
    auto simple_loader(DSLFun&& dsl_fun, CodeGen::OptLvl optLvl = CodeGen::OptLvl::O0, bool dump = false) {
        IRModule translated = irt.translate(dsl_fun);

        bool verified_ok = utils::verify_module(translated);
        EXPECT_TRUE(verified_ok) << "failed verify with fun: " << dsl_fun.name() << std::endl;

        if (dump) {
            std::cerr << std::endl << "Initial generated IR before any passes:" << std::endl;
            translated.get().dump();
        }
        ObjCode compiled = codeGen.compile(translated, optLvl);
        if (dump && optLvl) {
            std::cerr << std::endl << "IR after codeGen.compile with OptLvl=" << optLvl << ":" << std::endl;
            translated.get().dump();
        }

        ResidentObjCode resident_fun = CodeLoader::load(conn, memMgr, mem::MemType::ReadWrite,
                                                        irt, codeGen, compiled);
        return resident_fun;
    };

    template<typename DSLGen, typename ...Args>
    auto generic_loader(DSLGen&& dsl_gen, Args... args) {
        dsl::Function dsl_fun = make_dsl_fun<to_dsl_t<std::remove_reference_t<decltype(args)>>...>(dsl_gen);
        return simple_loader(dsl_fun, CodeGen::OptLvl::O0, true);
    };
};


TEST_F(CodeLoaderTest, echoTest) {
    EXPECT_TRUE(echo_pretest);
    EXPECT_TRUE(conn.echo("hello, slave."));
}

TEST_F(CodeLoaderTest, loadCodeTrivial) {
    auto tester = [&](auto&& irModule) {
        auto objCode = codeGen.compile(irModule, CodeGen::OptLvl::O0);
        return CodeLoader::load(conn, memMgr, mem::MemType::ReadWrite, irt, codeGen, objCode);
    };

    auto emptyCallable = tester(do_nothing);
    EXPECT_TRUE(emptyCallable.callAddr != 0);
    EXPECT_TRUE(conn.call(emptyCallable.callAddr));
    auto c2 = tester(self_sufficient);
    EXPECT_TRUE(c2.callAddr != 0);
}

TEST_F(CodeLoaderTest, loadGlobal) {
    auto load_tester = [&](auto&& g) {
        auto r = CodeLoader::load(g, conn, memMgr, mem::MemType::ReadWrite);

        EXPECT_TRUE(std::is_move_constructible_v<decltype(r)>)
                            << "ResidentGlobal must be move-constructible!"
                            << " (of type T = " << utils::type_name<decltype(r)>() << ")"
                            << std::endl;

        auto mr = r.memRegion();
        std::cerr << "loaded Global: ";
        mem::printMemRegion(std::cerr, mr);
        EXPECT_TRUE(mr.size > 0);

        auto loaded = r.read();
        auto orig = g.x.initial_val();
        std::cerr << "original: " << utils::to_string(orig)
                  << "; loaded-read: " << utils::to_string(loaded)
                  << std::endl;
        EXPECT_TRUE((loaded == orig) || !g.x.initialised());
    };

    load_tester(Global{ Var<int16_t>{42} });
    load_tester(Global{ Var<int64_t>{53, "g1"} });
    load_tester(Global{ Var<float>{3.1415, ""} });
    load_tester(Global{ Var<double>{69e-69, "gd2"} });
    load_tester(Global{ Array<Var<int>, 3>{ {1, 2, 3} } });
    load_tester(Global{ Struct<Var<short>, Var<float>, Var<long>>{ {-42, 2.74, 33333} } });
    // nested types
    using ArrayNestedT = Array<Array<Var<int>, 3>, 2>;
    load_tester(Global{ ArrayNestedT{{ {{10, 20, 30}, {11, 22, 33}} }} });
    using StructComplexT = Struct< Var<bool>, Struct< Var<int>, Var<double> >, Array< Var<int>, 3 > >;
    load_tester(Global{ StructComplexT{ {false, {42, 69.69}, {-11, -22, -33}} } });
}

TEST_F(CodeLoaderTest, loadAndCall) {
    auto max_dsl_generator = [](auto&& x, auto&& y) {
        return If(x > y, x, y);
    };
    int x{22}, y{11};
    EXPECT_EQ(std::max(x, y), generic_caller(max_dsl_generator, x, y));

    auto factorial_gen = [&](auto&& n){
        Var<int> res{1};
        return (While(
                n > Lit(0u),
                (res = res * n, n = n - Lit(1u))
        ), res);
    };
    auto remoteRes = generic_caller(factorial_gen, 5u);
    EXPECT_EQ(120, remoteRes);

    std::cerr << "end of codegen test" << std::endl;
}

TEST_F(CodeLoaderTest, callDslFromDsl) {
    using val_t = int;

    auto max_dsl_generator = [](auto&& x, auto&& y) {
        return If(x > y, x, y);
    };
    dsl::Function dsl_max = make_dsl_fun<Var<val_t>, Var<val_t>>(max_dsl_generator);
//    auto resident_max = simple_loader(dsl_max);

    auto max4_dsl_generator = [&](auto x1, auto x2, auto x3, auto x4) {
        return dsl_max(dsl_max(x1, x2), dsl_max(x3, x4));
    };

    val_t x1{22}, x2{11}, x3{42}, x4{69};
    val_t true_res = 69;
    auto result = generic_caller(max4_dsl_generator, x1, x2, x3, x4);
    EXPECT_EQ(result, true_res);
    std::cerr << "end of codegen test" << std::endl;
}

TEST_F(CodeLoaderTest, callResidentFromDsl) {
    using val_t = int;

    auto max_dsl_generator = [](auto&& x, auto&& y) {
        return If(x > y, x, y);
    };
    dsl::Function dsl_max = make_dsl_fun<Var<val_t>, Var<val_t>>(max_dsl_generator);
    auto resident_max = simple_loader(dsl_max);

    auto max4_dsl_generator = [&](auto x1, auto x2, auto x3, auto x4) {
        return resident_max(resident_max(x1, x2), resident_max(x3, x4));
    };
//    dsl::Function dsl_max4 = make_dsl_fun<Var<addr_t>, Var<addr_t>, Var<addr_t>, Var<addr_t>>(max4_dsl_generator);
//    auto resident_max4 = simple_loader(dsl_max4);

    val_t x1{22}, x2{11}, x3{42}, x4{69};
//    auto true_res = std::max(x1, x2, x3, x4);
    val_t true_res = 69;
    auto result = generic_caller(max4_dsl_generator, x1, x2, x3, x4);
    EXPECT_EQ(result, true_res);
    std::cerr << "end of codegen test" << std::endl;
}

TEST_F(CodeLoaderTest, loadAndCall2) {
    auto max_dsl_generator = [](auto&& x, auto&& y) {
        return If(x > y, x, y);
    };
    dsl::Function dsl_max = make_dsl_fun<Var<addr_t>, Var<addr_t>>(max_dsl_generator);
    auto resident_max = simple_loader(dsl_max);

    // Call it with some args
    addr_t x{22}, y{11};
    auto true_res = std::max(x, y);
    auto result = conn.call2(resident_max.callAddr, x, y);
    EXPECT_EQ(result, true_res);
    std::cerr << "end of codegen test" << std::endl;
}

TEST_F(CodeLoaderTest, readGPIO) {
    // Test only for arm
#ifdef HT_ENABLE_ARM
    // Get gpio configuration register addr
    auto gpio_conf_reg_addr = conn.mmap(0x13400000);
    EXPECT_TRUE(gpio_conf_reg_addr != 0) << "shouldn't return zero mmapped gpio configuration register!";

    // Create global volatile val at specified addr
    // variant 1
//    Global gcr{ Var<const volatile addr_t>{0, "gpio_conf_reg"} };
//    auto remoteGCR = CodeLoader::getLoadedResident(gcr, gpio_conf_reg_addr, conn, memMgr);
    // variant 2
    Global gcr_ptr{ Ptr<Var<volatile uint32_t>>{gpio_conf_reg_addr, "gpio_conf_reg"} };
    auto remoteGCR = CodeLoader::load(gcr_ptr, conn, memMgr);

    std::cerr << "codegen_test: loaded remote at: 0x" << std::hex << remoteGCR.addr << std::dec << std::endl;

//    auto gpio_reader_gen = [&]{
//        return *(remoteGCR + Lit(0x0c20 >> 2));
//    };

    auto gpio_reader_gen = [&]{
        uint32_t bit = 0x1;
        Array<Var<uint32_t>, 5> reg_data{};
        Ptr<Var<volatile uint32_t>> tmp_ptr{};
        return (
                tmp_ptr = remoteGCR + Lit(0x0c20 >> 2), reg_data[Lit(0)] = *tmp_ptr,
                *tmp_ptr |= Lit(bit << 8),              reg_data[Lit(1)] = *tmp_ptr,
                tmp_ptr = remoteGCR + Lit(0x0c24 >> 2),
                *tmp_ptr |= Lit(bit << 2),              reg_data[Lit(2)] = *tmp_ptr,
                *tmp_ptr &= ~Lit(bit << 2),             reg_data[Lit(3)] = *tmp_ptr,
                reg_data
        );
    };

    auto res = generic_caller(gpio_reader_gen);
    std::cerr << "Result: " << std::hex << utils::to_string(res)
              << std::dec << std::endl;

#else
    std::cerr << "Not arm; not running this test" << std::endl;
#endif
}

#ifdef HT_ENABLE_STM32

// Some structs & typedefs
namespace device {

//#include "stm32f4/Drivers/CMSIS/Include/stm32f429xx.h"
#define     __IO    volatile             /*!< Defines 'read / write' permissions */

//#include "stm32f4/Drivers/CMSIS/Device/ST/STM32F4xx/Include/stm32f429xx.h"
typedef struct
{
    __IO uint32_t MODER;    /*!< GPIO port mode register,               Address offset: 0x00      */
    __IO uint32_t OTYPER;   /*!< GPIO port output type register,        Address offset: 0x04      */
    __IO uint32_t OSPEEDR;  /*!< GPIO port output speed register,       Address offset: 0x08      */
    __IO uint32_t PUPDR;    /*!< GPIO port pull-up/pull-down register,  Address offset: 0x0C      */
    __IO uint32_t IDR;      /*!< GPIO port input data register,         Address offset: 0x10      */
    __IO uint32_t ODR;      /*!< GPIO port output data register,        Address offset: 0x14      */
    __IO uint32_t BSRR;     /*!< GPIO port bit set/reset register,      Address offset: 0x18      */
    __IO uint32_t LCKR;     /*!< GPIO port configuration lock register, Address offset: 0x1C      */
    __IO uint32_t AFR[2];   /*!< GPIO alternate function registers,     Address offset: 0x20-0x24 */
} GPIO_TypeDef;
/*!< Peripheral memory map */
#define PERIPH_BASE           0x40000000U /*!< Peripheral base address in the alias region                                */
#define APB1PERIPH_BASE       PERIPH_BASE
#define APB2PERIPH_BASE       (PERIPH_BASE + 0x00010000U)
#define AHB1PERIPH_BASE       (PERIPH_BASE + 0x00020000U)
#define AHB2PERIPH_BASE       (PERIPH_BASE + 0x10000000U)
#define GPIOG_BASE            (AHB1PERIPH_BASE + 0x1800U)

//#include "stm32f4/Drivers/STM32F4xx_HAL_Driver/Inc/stm32f4xx_hal_gpio.h"
#define GPIO_PIN_13                ((uint16_t)0x2000)  /* Pin 13 selected   */
#define GPIO_PIN_14                ((uint16_t)0x4000)  /* Pin 14 selected   */

}

TEST_F(CodeLoaderTest, stm32f4_LED) {
    const auto led_green = GPIO_PIN_13;
    const auto led_red = GPIO_PIN_14;
    const decltype(led_green) both_leds = led_red | led_green;

    const auto gpiog_odr_addr = GPIOG_BASE + offsetof(device::GPIO_TypeDef, ODR);
//    Global { Ptr<Var<volatile uint32_t>>{gpiog_bsrr, "gpiog_bsrr"} };
//    Global { to_dsl_t<volatile uint32_t *> {gpiog_bsrr} };
//    auto r = CodeLoader::load(gpiog_bsrr, conn, memMgr);
//    std::cerr << "codegen_test: loaded remote at: 0x" << std::hex << r.addr << std::dec << std::endl;

    auto toggle_gpiog_pin = [](auto gpio_pin){
        to_dsl_t<volatile uint32_t *> gpiog_odr_ptr{gpiog_odr_addr};
        return (*gpiog_odr_ptr ^= gpio_pin, Unit);
    };

    // try just call with parameter
    auto fr = generic_loader(toggle_gpiog_pin, led_green);
    exec.call(fr, both_leds);
    usleep(1000 * 1000); // sleep for N microseconds
    exec.call(fr, both_leds);

    // try schedule
    auto toggle_green_led = [&]{ return toggle_gpiog_pin(Lit{led_green}); };
    auto fr2 = generic_loader(toggle_green_led);
    EXPECT_TRUE(conn.schedule(fr2.callAddr, 2000));
}
#endif

// todo: various kinds of symbol conflicts related tests
TEST_F(CodeLoaderTest, pid_ctrl) {
    typedef int32_t ctrl_t;
    typedef float coef_t;

    Lit<ctrl_t> sp{42}; // Setpoint
    const ctrl_t ms_delay = 100; // Control cycle duration in ms
    Lit<float> dt{ms_delay / 1000.0};

/*    // Ziegler-Nichols method
    auto compute_coefs = [](float Ku, float Tu){
        return std::tuple{
                static_cast<int32_t>( 0.6*Ku ),
                static_cast<int32_t>( 1.2*Ku/Tu ),
                ( 3*Ku*Tu/40 )
        };
    };

    // Case 2: some, let's say, optimized coefficients
    auto coefs = compute_coefs(5, 1);
    auto Kp = Lit{std::get<0>(coefs)};
    auto Ki = Lit{std::get<1>(coefs)};
    auto Kd = Lit{std::get<2>(coefs)};
    std::cerr << "Got coefficients:\n"
              << "Kp=" << Kp.val << " (type=" << utils::type_name<decltype(Kp)>() << ")\n"
              << "Ki=" << Ki.val << " (type=" << utils::type_name<decltype(Ki)>() << ")\n"
              << "Kd=" << Kd.val << " (type=" << utils::type_name<decltype(Kd)>() << ")\n"
              << std::endl;*/


    auto perr = CodeLoader::load(Global{ Var<ctrl_t>{0, "perr"} }, conn, memMgr);
    auto ierr = CodeLoader::load(Global{ Var<ctrl_t>{0, "ierr"} }, conn, memMgr);

    auto pid_ctrl = [&](Var<ctrl_t> pv){
        Lit<float> Kp{4};
        Lit<float> Ki{6};
        Lit<float> Kd{0.5};

        // Local variables
        // pv -- process variable
        // cv -- control variable
//        Var<ctrl_t> pv(0, "pv"), cv(0, "cv"), prev_perr(0, "prev_err"), derr(0, "derr");
        Var<ctrl_t> cv(0, "cv"), prev_perr(0, "prev_err"), derr(0, "derr");

        // read_pv and write_cv are some dsl generators
        //  that perform actual input/output
        return (
//                pv = read_pv(),
                prev_perr = perr,
                perr = sp - pv,
                ierr += perr,
                derr = perr - prev_perr,
                cv = Kp*perr + Kd*derr/dt + Ki*ierr*dt,
//                write_cv(cv)
                cv
        );
    };

    auto opt_pid_ctrl = make_dsl_fun<Var<ctrl_t>>(pid_ctrl);
    auto r = simple_loader(opt_pid_ctrl, CodeGen::OptLvl::O2, true);
}


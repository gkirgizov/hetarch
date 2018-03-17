#include "gtest/gtest.h"

#include <memory>
#include <iostream>
#include <fstream>
#include <vector>
#include <utility>

//#include "llvm/IR/Module.h"
//#include "llvm/IR/LLVMContext.h"

#include "../new/supportingClasses.h"
#include "../new/CodeGen.h"
#include "../new/CodeLoader.h"
#include "../new/TCPConnection.h"
#include "../new/MemoryManager.h"
#include "../new/Executor.h"

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
            : codeGen("x86_64-unknown-linux-gnu")
            , optLvl{CodeGen::OptLvl::O0}
            , main_entry{utils::loadModule(main_entry_ll, ctx), "main"}
            , self_sufficient{utils::loadModule(ir0, ctx), "pow_naive"}
            , part1{utils::loadModule(ir1, ctx), "square"}
            , part2{utils::loadModule(ir2, ctx), "pow_smart"}
            , irt{}
    {}

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

}
}

struct cg_config {
    std::string triple;
    std::string host;
    uint16_t port;
};

cg_config read_config() {
    std::ifstream test_config{ress + "test_config.txt"};
    std::string triple;
    std::string host;
    uint16_t port;

    test_config >> triple >> host >> port;
    std::cerr << "Tests: using: host: " << host << "; port: " << port << "; triple: " << triple << std::endl;
    return {triple, host, port};
}


class CodeLoaderTest : public ::testing::Test {
public:
    explicit CodeLoaderTest()
            : config{read_config()}
            , codeGen(config.triple)
            , irt{}
            , ctx{irt.getContext()}
            , tr{new conn::TCPTrans<addr_t>{config.host, config.port}}
            , conn{*tr}
            , all_mem{conn.getBuffer(0), 1024 * 1024, mem::MemType::ReadWrite}
            , memMgr{all_mem}
            , exec{conn, memMgr, irt, codeGen}
            , main_entry{utils::loadModule(main_entry_ll, ctx), "main"}
            , do_nothing{utils::loadModule(do_nothing_ll, ctx), "do_nothing"}
//            , part1{utils::loadModule(ir1, ctx), "square"}
            , self_sufficient{utils::loadModule(ir0, ctx), "pow_naive"}
    {}

    const cg_config config;

    CodeGen codeGen;
    IRTranslator irt;
    LLVMContext& ctx;

    std::unique_ptr< conn::ITransmission<addr_t> > tr;
    conn::CmdProtocol<addr_t> conn;
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
//        DSLFunction dsl_fun = make_dsl_fun_from_arg_types(dsl_generator, dsl_args...);
        DSLFunction dsl_fun = make_dsl_fun_from_arg_types< to_dsl_t<std::remove_reference_t<decltype(args)>>... >(dsl_generator);

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
};


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
        std::cerr << "loaded DSLGlobal: ";
        mem::printMemRegion(std::cerr, mr);
        EXPECT_TRUE(mr.size > 0);

        auto loaded = r.read();
        auto orig = g.x.initial_val();
        std::cerr << "original: " << utils::to_string(orig)
                  << "; loaded-read: " << utils::to_string(loaded)
                  << std::endl;
        EXPECT_TRUE((loaded == orig) || !g.x.initialised());
    };

    load_tester( DSLGlobal{Var<int16_t>{42}} );
    load_tester( DSLGlobal{Var<int64_t>{53, "g1"}} );
    load_tester( DSLGlobal{Var<float>{3.1415, ""}} );
    load_tester( DSLGlobal{Var<double>{69e-69, "gd2"}} );
    load_tester( DSLGlobal{Array<Var<int>, 3>{ {1, 2, 3} }} );
}

TEST_F(CodeLoaderTest, loadAndCall) {
    auto max_dsl_generator = [](auto&& x, auto&& y) {
        return If(x > y, x, y);
    };
    int x{22}, y{11};
    EXPECT_TRUE(std::max(x, y) == generic_caller(max_dsl_generator, x, y));

    auto factorial_gen = [&](auto&& n){
        Var<int> res{1};
        return (While(
                n > DSLConst(0u),
                (res = res * n, n = n - DSLConst(1u))
        ), res);
    };
    auto remoteRes = generic_caller(factorial_gen, 5u);
    EXPECT_TRUE(120 == remoteRes) << "invalid result: " << remoteRes << std::endl;

    std::cerr << "end of codegen test" << std::endl;
}

TEST_F(CodeLoaderTest, readGPIO) {
    // Test only for arm
    if (config.triple.find("arm") != std::string::npos) {

        // Get gpio configuration register addr
        auto gpio_conf_reg_addr = conn.mmap(0x13400000);
        EXPECT_TRUE(gpio_conf_reg_addr != 0) << "shouldn't return zero mmapped gpio configuration register!";

//      auto name1 = "read_gpx1con_reg",

        // Create global volatile val at specified addr
        // variant 1
/*        DSLGlobal gcr{ Var<const volatile addr_t>{0, "gpio_conf_reg"} };
        EXPECT_TRUE(gcr.x.const_q && gcr.x.volatile_q) << "dsl inconsistency: tried to create cv, got not-cv!";
        auto remoteGCR = CodeLoader::getLoadedResident(gcr, gpio_conf_reg_addr, conn, memMgr);

        RawPtr<decltype(gcr)> tmp_ptr{};
        auto gpio_reader_gen = [&]{
            return (tmp_ptr = remoteGCR.takeAddr() + DSLConst(0x0c20 >> 2), *tmp_ptr);
        };*/

        // variant 2
        DSLGlobal gcr_ptr{ RawPtr<Var<const volatile uint32_t>>{gpio_conf_reg_addr, "gpio_conf_reg"} };
        EXPECT_TRUE(gcr_ptr.x.elt_const_q && gcr_ptr.x.elt_volatile_q) << "dsl inconsistency: tried to create cv, got not-cv!";
        auto remoteGCR = CodeLoader::load(gcr_ptr, conn, memMgr);
        std::cerr << "codegen_test: loaded remote at: 0x" << std::hex << remoteGCR.addr << std::dec << std::endl;

/*        auto gpio_reader_gen = [&]{
//            RawPtr<Var<uint32_t>> tmp_ptr{};
            return *(remoteGCR + DSLConst(0x0c20 >> 2));
        };*/

        auto gpio_reader_gen = [&]{
            uint32_t bit = 0x1;
            Array<Var<uint32_t>, 5> reg_data{};
            RawPtr<Var<uint32_t>> tmp_ptr{};
            return (
                    tmp_ptr = remoteGCR + DSLConst(0x0c20 >> 2), reg_data[DSLConst(0)] = *tmp_ptr,
                    *tmp_ptr |= DSLConst(bit << 8),              reg_data[DSLConst(1)] = *tmp_ptr,
                    tmp_ptr = remoteGCR + DSLConst(0x0c24 >> 2),
                    *tmp_ptr |= DSLConst(bit << 2),              reg_data[DSLConst(2)] = *tmp_ptr,
                    *tmp_ptr &= ~DSLConst(bit << 2),             reg_data[DSLConst(3)] = *tmp_ptr,
                    reg_data
            );
        };

        auto res = generic_caller(gpio_reader_gen);
        std::cerr << "Result: " << std::hex << utils::to_string(res)
                  << std::dec << std::endl;

    } else {
        std::cerr << "Not arm; not running this test" << std::endl;
    }
}

// todo: various kinds of symbol conflicts related tests


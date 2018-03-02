#include "gtest/gtest.h"

#include <memory>
#include <iostream>
#include <fstream>
#include <vector>

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
    EXPECT_TRUE(codeGen.compile(main_entry)) << "main.ll failed to compile";
    EXPECT_TRUE(codeGen.compile(self_sufficient)) << "self_sufficient.ll failed to compile";
}

TEST_F(CodeGenTest, objCodeFailFindSymbol) {
    auto objCode = codeGen.compile(self_sufficient);
    EXPECT_TRUE(objCode.getSymbol() == llvm::object::SymbolRef()) << "symbol shouldn't be found (due to name mangling!)";
}

TEST_F(CodeGenTest, dependentCompile) {
    EXPECT_FALSE(codeGen.compile(part2)) << "module with unresolved symbols shouldn't compile!";

    // provide dep-s and compile succesfully
    vector<IIRModule> deps;
    deps.emplace_back(move(part1));
    deps.emplace_back(move(main_entry));
    EXPECT_TRUE(CodeGen::link(part2, deps)) << "but this case is tested by fewModulesLink!";

    auto compiled = codeGen.compile(part2);
    EXPECT_TRUE(compiled) << "module with resolved symbols should compile!";
}


class CodeLoaderTest : public ::testing::Test {
public:
    explicit CodeLoaderTest()
            : codeGen("x86_64-unknown-linux-gnu")
            , irt{}
            , ctx{irt.getContext()}
            , host{"localhost"}
            , port{13334}
            , conn{host, port}
            , all_mem{conn.getBuffer(0), 1024 * 1024, mem::MemType::ReadWrite}
            , memMgr{all_mem}
            , exec{conn, memMgr, irt, codeGen}
            , main_entry{utils::loadModule(main_entry_ll, ctx), "main"}
            , do_nothing{utils::loadModule(do_nothing_ll, ctx), "do_nothing"}
//            , part1{utils::loadModule(ir1, ctx), "square"}
            , self_sufficient{utils::loadModule(ir0, ctx), "pow_naive"}
    {}

    CodeGen codeGen;
    IRTranslator irt;
    LLVMContext& ctx;

//    mock::MockConnection<addr_t> conn{};
//    mock::MockMemManager<addr_t> memMgr{};

    const std::string host;
    const uint16_t port;
    conn::TCPConnection<addr_t> conn;

    const MemRegion<addr_t> all_mem;
    mem::MemManagerBestFit<addr_t> memMgr;

    Executor<addr_t> exec;

    IRModule<VoidExpr> main_entry;
    IRModule<VoidExpr> do_nothing;
//    IRModule<Var<int>, Var<int>> part1;
    IRModule<Var<int>, Var<int>, Var<int>> self_sufficient;
};


TEST_F(CodeLoaderTest, loadCodeTrivial) {
//    auto objCode = codeGen.compile(main_entry);
    auto tester = [&](auto&& irModule) {
        auto objCode = codeGen.compile(irModule);
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
        ResidentVar r = CodeLoader::load(conn, memMgr, mem::MemType::ReadWrite, g);

        EXPECT_TRUE(std::is_move_constructible_v<decltype(r)>)
                            << "ResidentGlobal must by constructible! (of type T = "
                            << utils::type_name<decltype(r)>() << ")" << std::endl;

        auto mr = r.memRegion();
        std::cerr << "loaded DSLGlobal: ";
        mem::printMemRegion(std::cerr, mr);
        EXPECT_TRUE(mr.size > 0);

        auto loaded = r.read();
        auto orig = g.x.initial_val();
        std::cerr << "original: " << orig << "; loaded-read: " << loaded << std::endl;
        EXPECT_TRUE((loaded == orig) || !g.x.initialised());
    };

    load_tester( DSLGlobal{Var<int16_t>{42}} );
    load_tester( DSLGlobal{Var<int64_t>{53, "g1"}} );
    load_tester( DSLGlobal{Var<float>{3.1415, ""}} );
    load_tester( DSLGlobal{Var<double>{69e-69, "gd2"}} );
}

TEST_F(CodeLoaderTest, loadAndCall) {

    auto generic_caller = [&](auto&& dsl_generator, auto... args) {
        // Make function from generator
//        DSLFunction dsl_fun = make_dsl_fun_from_arg_types(dsl_generator, dsl_args...);
        DSLFunction dsl_fun = make_dsl_fun_from_arg_types< Var<decltype(args)>... >(dsl_generator);

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

    // Define some dsl functions
    auto max_dsl_generator = [](auto&& x, auto&& y) {
        return If(x > y, x, y);
    };
    int x{22}, y{11};
    EXPECT_TRUE(std::max(x, y) == generic_caller(max_dsl_generator, x, y));

    Var<int> res{1};
    auto factorial_gen = [&](auto&& n){
        return (While(
                n > DSLConst(0u),
                (res = res * n, n = n - DSLConst(1u))
        ), res);
    };
    auto remoteRes = generic_caller(factorial_gen, 5u);
    EXPECT_TRUE(120 == remoteRes) << "invalid result: " << remoteRes << std::endl;
}


// todo: various kinds of symbol conflicts related tests


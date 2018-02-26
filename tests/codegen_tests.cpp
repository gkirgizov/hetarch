#include "gtest/gtest.h"

#include <memory>
#include <iostream>
#include <fstream>
#include <vector>

#include "llvm/IR/Module.h"
#include "llvm/IR/LLVMContext.h"

#include "../new/supportingClasses.h"
#include "../new/CodeGen.h"
#include "../new/CodeLoader.h"
#include "../new/TCPConnection.h"
#include "../new/MemoryManager.h"

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
            , main_entry{utils::loadModule(main_entry_ll, ctx), "main"}
            , part1{utils::loadModule(ir1, ctx), "square"}
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

    IRModule<VoidExpr> main_entry;
    IRModule<Var<int>, Var<int>> part1;
};


TEST_F(CodeLoaderTest, loadCodeTrivial) {
//    auto objCode = codeGen.compile(main_entry);
    auto objCode = codeGen.compile(part1);
    auto resident = CodeLoader::load(&conn, &memMgr, mem::MemType::ReadWrite, objCode);
}

TEST_F(CodeLoaderTest, loadGlobal) {

    auto load_tester = [&](auto&& g) {
    //    auto r = CodeLoader::load(conn, memMgr, mem::MemType::ReadWrite, irt, codeGen, g);
        ResidentGlobal r = CodeLoader::load(conn, memMgr, mem::MemType::ReadWrite, g);

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


// todo: various kinds of symbol conflicts related tests


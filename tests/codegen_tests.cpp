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

#include "utils.h"
#include "mocks.h"


using namespace std;
using namespace llvm;
using namespace hetarch;
using namespace hetarch::mem;


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
            , conn{}
            , memMgr{}
    {}

    LLVMContext ctx{};
//    IIRModule main_entry;
    IRModule<void> main_entry;
    IRModule<int, int, int> self_sufficient;
    IRModule<int, int> part1;
    IRModule<int, int, unsigned int> part2;

    CodeGen codeGen;

    mock::MockConnection<addr_t> conn;
    mock::MockMemManager<addr_t> memMgr;

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

TEST_F(CodeGenTest, codeLoaderTest1) {
    auto objCode = codeGen.compile(part1);
    auto resident = CodeLoader::load(&conn, &memMgr, mem::MemType::ReadWrite, objCode);
}


// todo: various kinds of symbol conflicts related tests


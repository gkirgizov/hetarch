#include "gtest/gtest.h"

#include <memory>
#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <set>

#include <csignal>

#include "llvm/IR/Module.h"
#include "llvm/IR/LLVMContext.h"

#include "../new/supportingClasses.h"
#include "../new/CodeGen.h"
#include "../new/CodeLoader.h"
#include "../new/MemoryManager.h"
#include "../new/MemResident.h"

#include "utils.h"
#include "mocks.h"


using namespace std;
using namespace llvm;
using namespace hetarch;
using namespace hetarch::mem;

namespace {

using addr_t = decltype(sizeof(void *));

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


// todo: parametrize by MemManager subtype?
class MemManagerTest: public ::testing::Test {
public:
    explicit MemManagerTest()
    : dmt{mem::MemType::ReadWrite}
    , allMem{0, 1024, dmt}
    , memMgr{allMem}
//    , memMgr{mem::MemManagerBestFit<addr_t>{allMem}}
    {
        // establish some initial fragmentation
        for(addr_t i = 4; i <= 9; ++i) {
            auto mr = memMgr.alloc(1 << i, dmt);
            memMgr.unsafeFree(mr);
        }

//        memMgr.printMemRegions(cout);
    }

    const mem::MemType dmt; // default mem type
    const mem::MemRegion<addr_t> allMem;
//    const std::vector<mem::MemRegion<addr_t>> allMem;
    mem::MemManagerBestFit<addr_t> memMgr;
};

TEST_F(MemManagerTest, alignTest) {
    addr_t al = 8;
    auto mr1 = memMgr.alloc(69, dmt, al);
    EXPECT_TRUE(mr1.size == 72);

    auto mr2 = memMgr.alloc(69, dmt, al);
    EXPECT_TRUE(memMgr.aligned(mr2.start, al));
    EXPECT_TRUE(memMgr.aligned(mr2.end, al));
    EXPECT_TRUE(memMgr.aligned(mr2.size, al));
}

TEST_F(MemManagerTest, unmapMultisetTest) {
    // just to be sure in how containers work
    unordered_map<int, multiset<int>> ct;
    ct[0] = {1, 2, 2, 3};
    auto& set_ref = ct[0];  // 'auto&' is crucial !!! (not just 'auto')
    assert(set_ref.size() == 4);

//    auto it = set_ref.lower_bound(2);
//    assert(it != set_ref.end());
    auto it = set_ref.begin();
    set_ref.erase(it);

    EXPECT_TRUE(set_ref.size() == 3) << "set returned by ref from unordered_map has to be modified!";
    EXPECT_TRUE(ct[0].size() == 3) << "actual set wasn't modified!";
}

TEST_F(MemManagerTest, allocOutOfMemTest) {
    auto mr1 = memMgr.alloc(512, dmt);
    EXPECT_TRUE(mr1.size == 512) << "trivial alloc with enough mem should be satisfied!";
    auto mr2 = memMgr.alloc(513, dmt);
    EXPECT_TRUE(mr2.size == 0) << "alloc with not enough mem shouldn't be satisfied!";
}

TEST_F(MemManagerTest, allocTrivialTest) {
    EXPECT_FALSE(memMgr.alloc(513, dmt)) << "alloc with not enough mem in biggest fragment shouldn't be satisfied!";
    for(addr_t i = 4; i <= 9; ++i) {
        auto mr = memMgr.alloc(1 << i, dmt);
        EXPECT_TRUE(mr) << "alloc with exact fragments should be staisifed";
    }
}

TEST_F(MemManagerTest, allocBestFitTest) {
    auto mr1 = memMgr.alloc(512, dmt);
    EXPECT_TRUE(mr1.size == 512) << "alloc with exact mem should be satisfied!";
    memMgr.free(mr1);

    auto mr2 = memMgr.alloc(257, dmt);
    EXPECT_TRUE(mr2.size != 0) << "alloc with enough mem in biggest fragment should be satisfied!";

    // although there're enough memory, fragmentation is bad
    EXPECT_FALSE(memMgr.alloc(512, dmt)) << "alloc with not enough mem in biggest fragment shouldn't be satisfied!";

}

//TEST_F(MemManagerTest, tryAllocTest) {
//}

//TEST_F(MemManagerTest, freeTest) {
//}



} // namespace


int main(int argc, char *argv[]) {
    ::testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}

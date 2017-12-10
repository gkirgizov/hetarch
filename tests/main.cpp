#include "gtest/gtest.h"

#include <memory>
#include <iostream>
#include <fstream>
#include <vector>

#include "llvm/IR/Module.h"
#include "llvm/IR/LLVMContext.h"

#include "../new/supportingClasses.h"
#include "../new/CodeGen.h"

#include "utils.h"


using namespace std;
using namespace llvm;
using namespace hetarch;

namespace {

// todo: somehow change it
const string project_root{"/home/hades/projects/hetarch/"};
const string ress{project_root + "tests/test_resources/"};
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


class LinkTest : public ::testing::Test {
public:
    explicit LinkTest()
    : self_sufficient{utils::loadModule(ir0, ctx)}
    , part1{utils::loadModule(ir1, ctx)}
    , part2{utils::loadModule(ir2, ctx)}
    {}

    LLVMContext ctx{};
    IIRModule self_sufficient;
    IIRModule part1;
    IIRModule part2;
};


TEST_F(LinkTest, triviallyLinkable) {
    ASSERT_TRUE(CodeGen::link(self_sufficient));
    ASSERT_TRUE(self_sufficient);
}

TEST_F(LinkTest, twoModulesLinkable) {
    // fail link intentionally
    EXPECT_FALSE(CodeGen::link(part2));
    ASSERT_TRUE(part2);

    // provide dependency and make successfull link
    vector<IIRModule> deps;
    deps.emplace_back(move(part1));
    EXPECT_TRUE(CodeGen::link(part2, deps));
    ASSERT_TRUE(part2);
}

//TEST_F(LinkTest, linearDependencyLinkable) {
//
//}

// todo: various kinds of symbol conflicts related tests


} // namespace


int main(int argc, char *argv[]) {
    ::testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}

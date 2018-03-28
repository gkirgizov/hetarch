#include "gtest/gtest.h"

#include "../new/MemManager.h"
#include "../new/MemResident.h"
//
#include "mocks.h"


using namespace std;
using namespace hetarch;
using namespace hetarch::mem;


using addr_t = HETARCH_TARGET_ADDRT;


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


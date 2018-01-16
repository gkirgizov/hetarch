#include "gtest/gtest.h"


namespace {

//using addr_t = decltype(sizeof(void *));
using addr_t = HETARCH_TARGET_ADDRT;

} // namespace


int main(int argc, char *argv[]) {
    ::testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}

#include "gtest/gtest.h"
#include <networkMapper/Tracer.h>

TEST(MethodChecking, customSeqNum) {
    uint16_t numOfHops {64};
    for (uint16_t ttl = 1; ttl < numOfHops; ttl++) {
        for (uint16_t retryNum = 0; retryNum < NUM_OF_PINGS; retryNum++) {
            uint16_t encoded {Tracer::getCustomSeqNum(ttl, retryNum)};
            std::pair<uint16_t, uint16_t> unencoded {TraceRouteResult::getValuesFromCustomSeqNum(encoded)};
            EXPECT_EQ(ttl, unencoded.first);
            EXPECT_EQ(retryNum, unencoded.second);
        }
    }
}

TEST(MethodChecking, trace) {
    std::string destinationIP {"1.1.1.1"};
    auto res = Tracer::trace(destinationIP, 64);
    std::cout << res;
}

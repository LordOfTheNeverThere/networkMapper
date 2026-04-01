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

TEST(MethodChecking, multipleTraces) {
    std::vector<std::string> destinations {};
    for (int i = 0; i < 4; ++i) {
        destinations.push_back("1.1.1.1");
    }
    auto results = Tracer::multipleTraces(destinations, 64);

    for (auto res: results) {
        for (const auto& [ttl, hop]: res.getPath()) {
            EXPECT_GT(ttl, 0);
            for (auto retry: hop) {
                EXPECT_GE(ttl, 0);
            }
        }
    }
}

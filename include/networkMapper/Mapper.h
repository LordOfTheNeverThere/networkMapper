
#ifndef NETWORKMAPPER_MAPPER_H
#define NETWORKMAPPER_MAPPER_H
#include "../../cmake-build-debug/_deps/googletest-src/googletest/include/gtest/gtest_prod.h"
#include "socks/RawSocket.h"
#include "socks/LocalHost.h"
#include "socks/ExternalInterface.h"

class Mapper {
    FRIEND_TEST(MethodChecking, simpleThreadChecking);
    sa_family_t m_ipVersion {};
    std::vector<ExternalInterface> m_neighbours{};

public:
    static std::vector<ExternalInterface> mapLocalNetwork(const std::unordered_map<std::string, std::vector<uint32_t>>& localIPsToMap,
        const LocalHost& machine = LocalHost(true));

    static std::vector<ExternalInterface> mapNonLocalNetwork(const std::vector<uint32_t>& nonLocalIPsToMap);

    Mapper(const sa_family_t ipVersion);
    std::vector<ExternalInterface> mapNetwork(const std::vector<uint32_t>& nonLocalIPsToMap,
        const std::unordered_map<std::string, std::vector<uint32_t>>& localIPsToMap,
        LocalHost myMachine = LocalHost(true));
    void getTraceRoute(const std::string& destIPAddr, const Int& hops = 64);


#define MAX_RETRIES 5
#define PING_SEND_BATCH_SIZE 64
#define PING_SEND_BATCH_SLEEP_MS 1
#define ARP_SEND_BATCH_SIZE 64
#define ARP_SEND_BATCH_SLEEP_MS 1
#define RCV_BUFFER_SIZE 4*1024*1024
#define SND_BUFFER_SIZE 4*1024*1024
};
#endif //NETWORKMAPPER_MAPPER_H
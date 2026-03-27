#ifndef NETWORKMAPPER_IPV4RANGE_H
#define NETWORKMAPPER_IPV4RANGE_H


#include <string>
#include <vector>
#include <unordered_map>
#include "../../socks/include/socks/LocalHost.h"
#include "../../socks/include/socks/Exceptions.h"
#include "../../cmake-build-debug/_deps/googletest-src/googletest/include/gtest/gtest_prod.h"


class IPv4OnlyException : public ConfigException {

public:
    IPv4OnlyException()
        : ConfigException("IPv4Range can only be constructed using IPv4 addresses."){}
};

class IPv4OutOfRangeException : public ConfigException {
public:
    IPv4OutOfRangeException()
        : ConfigException("The smallest network mask possible is 255.255.0.0 aka /16"){}
};



class IPv4Range {
    FRIEND_TEST(MethodChecking, dualSenderMapLocalNetworkCheck);
    FRIEND_TEST(MethodChecking, checkIfAlgoHandlesAllReplies);
    std::vector<uint32_t> m_ipsRangeNonLocal{};
    std::unordered_map<std::string, std::vector<uint32_t>> m_ipsRangeLocal{};

public:
    IPv4Range(const std::string& ip, const std::string& mask, const LocalHost& localhost);

    std::vector<uint32_t> getIPsNonLocal() {
        return m_ipsRangeNonLocal;
    }

    std::unordered_map<std::string, std::vector<uint32_t>> getIPsLocal() {
        return m_ipsRangeLocal;
    }
};
#endif //NETWORKMAPPER_IPV4RANGE_H
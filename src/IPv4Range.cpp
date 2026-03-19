#include <string>
#include <system_error>
#include <vector>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "socks/GenericException.h"
#include "socks/Tools.h"
#include "socks/types.h"

class IPv4Range {
    std::vector<uint32_t> m_ipsRange{};
public:
    IPv4Range(const std::string& ip, const std::string& mask) {
        if (ip.size() > 15 || ip.size() < 7 || mask.size() > 15 || mask.size() < 7) {
            throw GenericException("IPv4Range can only be constructed using IPv4 addresses.");
        }

        Int result {0};
        sockaddr_in ipSockAddr {};
        sockaddr_in maskSockAddr {};

        result = inet_pton(AF_INET, ip.c_str(), &ipSockAddr.sin_addr);
        if (result != 1) {
            throw GenericException("It was not possible to convert the ip addresses into its binary form from string \nReason:\n " + std::system_category().message(errno));
        }
        result = inet_pton(AF_INET, mask.c_str(), &maskSockAddr.sin_addr);
        if (result != 1) {
            throw GenericException("It was not possible to convert the mask into its binary form from string \nReason:\n " + std::system_category().message(errno));
        }

        uint32_t numberOfIPs {Tools::getNumberOfIPsInMaskIPv4(maskSockAddr.sin_addr.s_addr)};
        m_ipsRange.reserve(numberOfIPs);
        Tools::generateIPv4Range(ipSockAddr.sin_addr.s_addr, maskSockAddr.sin_addr.s_addr, m_ipsRange);
    }

};

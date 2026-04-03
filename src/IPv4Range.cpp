#include <string>
#include <system_error>
#include <vector>
#include <unordered_map>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "socks/LocalHost.h"
#include "socks/Tools.h"
#include "socks/types.h"
#include "networkMapper/IPv4Range.h"

#include <cmath>


IPv4Range::IPv4Range(const std::string& ip, const std::string& mask, const LocalHost& localhost) {

    if (!Tools::isValidIPv4(ip) || !Tools::isValidIPv4(mask)) {
        throw IPv4OnlyException();
    }

    Int result {0};
    sockaddr_in ipSockAddr {};
    sockaddr_in maskSockAddr {};

    result = inet_pton(AF_INET, ip.c_str(), &ipSockAddr.sin_addr);
    if (result != 1) {
        throw ConversionToIPBinaryException(ip);
    }
    result = inet_pton(AF_INET, mask.c_str(), &maskSockAddr.sin_addr);
    if (result != 1) {
        throw ConversionToIPBinaryException(mask);
    }
    if (!Tools::checkIfNetworkMaskIsValid(maskSockAddr.sin_addr.s_addr)) {
        throw InvalidNetworkMaskException(mask);
    }

    uint32_t minNetworkMask {Tools::getNumericValueOfIPAddr(255,255,0,0)};
    if (ntohl(maskSockAddr.sin_addr.s_addr) < minNetworkMask) {
        throw IPv4OutOfRangeException();
    }

    std::vector<uint32_t> ipsInRange {Tools::generateIPv4RangeHostEndiness(ipSockAddr.sin_addr.s_addr, maskSockAddr.sin_addr.s_addr)};

    auto partitionPoint
    = std::partition(ipsInRange.begin(), ipsInRange.end(), [localhost](uint32_t ip) {
        InternalInterface localInterface {localhost.getInterfaceFromSubnetIPv4(ip)};
        return (localInterface.getInterfaceName().empty());
    });// This partitions the vector and puts all IPs which are non-local at the front and the local ones at the back.

    m_ipsRangeNonLocal.insert(m_ipsRangeNonLocal.end(),
                        std::make_move_iterator(ipsInRange.begin()),
                        std::make_move_iterator(partitionPoint));
    // Move all elements from ipsInRange start to the partitionPoint, non-local IPs, to the end of the ipsRangeNonLocal Vector

    for (auto it = partitionPoint; it != ipsInRange.end(); ++it) {
        InternalInterface localInterface {localhost.getInterfaceFromSubnetIPv4(*it)};
        m_ipsRangeLocal[localInterface.getInterfaceName()].push_back(*it);
    }
    //Store the remainder of the IPs in the vector pertaining to the interface local to that IP
}




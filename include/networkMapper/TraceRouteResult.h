
#ifndef NETWORKMAPPER_TRACEROUTERESULT_H
#define NETWORKMAPPER_TRACEROUTERESULT_H
#include <map>
#include <netinet/ip_icmp.h>
#include "socks/ExternalInterface.h"
#include "networkMapper/TraceRouteHop.h"

#define NUM_OF_PINGS 3
#define MAX_NUM_OF_HOPS 255



class TraceRouteResult {
    std::map<uint16_t, std::array<TraceRouteHop, NUM_OF_PINGS>> m_path {};

public:
    TraceRouteResult();

    std::map<uint16_t, std::array<TraceRouteHop, NUM_OF_PINGS>> getPath() {
        return m_path;
    }

    uint16_t findFirstHopWithIP(const std::string& targetIp) const;

    static std::pair<uint16_t, uint16_t> getValuesFromCustomSeqNum(uint16_t seqNum);

    TraceRouteHop addEntryFromEchoReply(const uint8_t * reply, const uint16_t ttlUsed, uint16_t traceID, const std::string& destination);
};




inline std::ostream& operator<<(std::ostream& os, const TraceRouteResult& result) {
    auto path = const_cast<TraceRouteResult&>(result).getPath();
    Int lastTTL {path.rbegin()->first};

    for (int ttl = 1; ttl <= lastTTL; ++ttl) {
        os << "Hop: " << std::setw(2) << ttl << "  ";

        if (path.find(ttl) == path.end()) { // no data on this hop
            for (int i = 0; i < NUM_OF_PINGS; ++i) {
                os << std::left << std::setw(15) << "*" << "  ";
            }
            os << '\n';
        } else { // At least one data point on this hop
            for (const auto& hop : path.at(ttl)) {
                os << std::left << std::setw(15) << hop.getIPAddress() << "  ";
            }
            os << "Time:" << " ";
            for (const auto& hop : path.at(ttl)) {
                std::string timelapse {(hop.getTimelapseInMillis() == 0) ? "***" : std::to_string(hop.getTimelapseInMillis())};
                os << std::left << std::setw(12) << timelapse << "  ";
            }
            os << "ms\n";
        }
    }


    return os;
}

#endif //NETWORKMAPPER_TRACEROUTERESULT_H
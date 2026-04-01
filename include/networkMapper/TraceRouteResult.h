
#ifndef NETWORKMAPPER_TRACEROUTERESULT_H
#define NETWORKMAPPER_TRACEROUTERESULT_H
#include <map>
#include <netinet/ip_icmp.h>
#include "socks/ExternalInterface.h"

#define NUM_OF_PINGS 3
#define MAX_NUM_OF_HOPS 255
class TraceRouteHop {

private:
    std::string m_ipAddress {"*"};
    uint64_t m_timelapse {0};

public:
    [[nodiscard]] std::string getIPAddress() const {
        return m_ipAddress;
    }

    void setIPAddress(const std::string &ip) {
        m_ipAddress = ip;
    }

    [[nodiscard]] uint64_t getTimelapse() const {
        return m_timelapse;
    }

    [[nodiscard]] double getTimelapseInMillis() const {
        return static_cast<double>(m_timelapse)/1000000;
    }

    void setTimelapse(const uint64_t timelapse) {
        m_timelapse = timelapse;
    }

};

inline std::ostream& operator<<(std::ostream& os, const TraceRouteHop& hop) {
    os << std::setw(15) << hop.getIPAddress();

    if (hop.getTimelapse() == 0) {
        os << " Unknown timelapse!";
    } else {
        os << " (" << hop.getTimelapseInMillis() << " ms)";
    }

    return os;
}



class TraceRouteResult {
    std::map<uint16_t, std::array<TraceRouteHop, NUM_OF_PINGS>> m_path {};

public:
    TraceRouteResult() {}

    std::map<uint16_t, std::array<TraceRouteHop, NUM_OF_PINGS>> getPath() {
        return m_path;
    }

    uint16_t findFirstHopWithIP(const std::string& targetIp) const {
        // std::map is sorted by key (TTL), so we can just iterate linearly
        for (const auto& [ttl, pings] : m_path) {
            for (const auto& hop : pings) {
                if (hop.getIPAddress() == targetIp) {
                    return ttl;
                }
            }
        }
        return 0;
    }

    static std::pair<uint16_t, uint16_t> getValuesFromCustomSeqNum(uint16_t seqNum) {

        seqNum = seqNum - 100;
        uint16_t ttl {1};
        while (seqNum >= NUM_OF_PINGS) {
            seqNum = seqNum - NUM_OF_PINGS;
            ttl++;
        }
        uint16_t retry {seqNum};

        return std::pair<uint16_t, uint16_t> {ttl, retry};
    }
    TraceRouteHop addEntryFromEchoReply(const uint8_t * reply, const uint16_t ttlUsed, uint16_t traceID, std::string& destination) {
        Int sizeOfIpHeader = (reply[0] & 0x0F) * 4;
        Int sizeOfICMPHeader {8};

        ip ipHeaderReceiveBuffer {};
        std::memcpy(&ipHeaderReceiveBuffer, reply, sizeOfIpHeader);
        IPv4Header ipHeaderReceive {&ipHeaderReceiveBuffer};


        icmphdr ICMPHeaderReceive {};
        std::memcpy(&ICMPHeaderReceive,reply + sizeOfIpHeader, 8);
        uint64_t timelapse {};
        uint64_t currNanosecs = std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now()).time_since_epoch().count();
        if (ICMPHeaderReceive.type == ICMP_ECHOREPLY) {
            std::memcpy(&timelapse, reply + sizeOfIpHeader + sizeOfICMPHeader, sizeof(timelapse));
        } else if (ICMPHeaderReceive.type == ICMP_TIME_EXCEEDED && ICMPHeaderReceive.code == ICMP_EXC_TTL) {
            std::memcpy(&ICMPHeaderReceive,reply + sizeOfIpHeader + sizeOfICMPHeader + sizeOfIpHeader, sizeOfICMPHeader); // Fetch The original ICMP Header from error ICMP
            std::memcpy(&timelapse, reply + sizeOfIpHeader + sizeOfICMPHeader + sizeOfIpHeader + sizeOfICMPHeader, sizeof(timelapse));
        }

        if (traceID != ntohs(ICMPHeaderReceive.un.echo.id)) {
            std::cerr << "Tracer caught an ICMP packet with incorrect ID.\nIgnoring it...\n";
            return TraceRouteHop{};
        }
        TraceRouteHop hop {};
        auto [ttl, retryNum] = getValuesFromCustomSeqNum(ntohs(ICMPHeaderReceive.un.echo.sequence));
        if (ttl > ttlUsed || retryNum > NUM_OF_PINGS) { // Check if seq is correct, id was checked at RawSocket.receive
            std::cerr << "Tracer caught an ICMP packet with incorrect ttl and retry number, respectfully: " << ttl << " and " << retryNum << '\n' << "Ignoring it...\n";
            return TraceRouteHop{};
        }

        Int firstHop {findFirstHopWithIP(destination)};
        if (firstHop != 0 && ttl > firstHop) {// if we already have the final destination in a lower hop we can reject this new addition
            return TraceRouteHop{};
        }

        hop.setIPAddress(ipHeaderReceive.getSourceStr());
        if (timelapse != 0) {// Some routers do not reply back with the payload.
            hop.setTimelapse(currNanosecs - be64toh(timelapse));

        }
        m_path[ttl][retryNum] = hop;
        return hop;
    }
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
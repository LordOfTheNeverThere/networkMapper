
#ifndef NETWORKMAPPER_TRACEROUTERESULT_H
#define NETWORKMAPPER_TRACEROUTERESULT_H
#include <map>
#include <netinet/ip_icmp.h>

#include "socks/ExternalInterface.h"

#define NUM_OF_PINGS 3
#define MAX_NUM_OF_HOPS 255
class TraceRouteHop {

private:
    std::string m_ipAddress {"***"};
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

    void setTimelapse(const uint64_t timelapse) {
        m_timelapse = timelapse;
    }

};

class TraceRouteResult {
    std::map<uint16_t, std::array<TraceRouteHop, NUM_OF_PINGS>> m_path {};

public:
    TraceRouteResult() {}

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

    void addEntryFromEchoReply(const uint8_t* reply, const uint16_t ttlUsed) {

        uint8_t ipHeaderReceiveBuffer[sizeof(ip)] {};
        std::memcpy(ipHeaderReceiveBuffer, reply, sizeof(ip));
        IPv4Header ipHeaderReceive {ipHeaderReceiveBuffer};
        icmphdr ICMPHeaderReceive {};
        std::memcpy(&ICMPHeaderReceive,reply + sizeof(ip), sizeof(icmphdr));
        uint64_t timelapse {};
        TraceRouteHop hop {};
        uint64_t currNanosecs = std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now()).time_since_epoch().count();
        auto [ttl, retryNum] = getValuesFromCustomSeqNum(ntohs(ICMPHeaderReceive.un.echo.sequence));

        if (ttl > ttlUsed || retryNum > NUM_OF_PINGS) {
            std::cerr << "Tracer caught an ICMP packet with incorrect ttl and retry number, respectfully: " << ttl << " and " << retryNum << '\n' << "Ignoring it...\n";
            return;
        }

        if (ICMPHeaderReceive.type == ICMP_ECHOREPLY) {
            std::memcpy(&timelapse, reply + sizeof(ip) + sizeof(icmphdr), sizeof(timelapse));
        } else if (ICMPHeaderReceive.type == ICMP_TIME_EXCEEDED && ICMPHeaderReceive.code == ICMP_EXC_TTL) {
            std::memcpy(&timelapse, reply + sizeof(ip) + sizeof(icmphdr) + sizeof(ip) + sizeof(icmphdr), sizeof(timelapse));
        }

        hop.setIPAddress(ipHeaderReceive.getSourceStr());
        hop.setTimelapse(currNanosecs - be64toh(timelapse));
        m_path[ttl][retryNum] = hop;
    }
};


#endif //NETWORKMAPPER_TRACEROUTERESULT_H
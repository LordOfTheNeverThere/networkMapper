#include "networkMapper/TraceRouteResult.h"

TraceRouteResult::TraceRouteResult() {
}


uint16_t TraceRouteResult::findFirstHopWithIP(const std::string& targetIp) const{
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

std::pair<uint16_t, uint16_t> TraceRouteResult::getValuesFromCustomSeqNum(uint16_t seqNum) {

    seqNum = seqNum - 100;
    uint16_t ttl {1};
    while (seqNum >= NUM_OF_PINGS) {
        seqNum = seqNum - NUM_OF_PINGS;
        ttl++;
    }
    uint16_t retry {seqNum};

    return std::pair<uint16_t, uint16_t> {ttl, retry};
}



    TraceRouteHop TraceRouteResult::addEntryFromEchoReply(const uint8_t * reply, const uint16_t ttlUsed, uint16_t traceID, const std::string& destination) {
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
            //std::cerr << "Tracer caught an ICMP packet with incorrect ID.\nIgnoring it...\n";
            return TraceRouteHop{};
        }
        TraceRouteHop hop {};
        auto [ttl, retryNum] = getValuesFromCustomSeqNum(ntohs(ICMPHeaderReceive.un.echo.sequence));
        if (ttl > ttlUsed || retryNum > NUM_OF_PINGS) { // Check if seq is correct, id was checked at RawSocket.receive
            //std::cerr << "Tracer caught an ICMP packet with incorrect ttl and retry number, respectfully: " << ttl << " and " << retryNum << '\n' << "Ignoring it...\n";
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
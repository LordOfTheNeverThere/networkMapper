//
// Created by miguel on 3/28/26.
//

#ifndef NETWORKMAPPER_TRACER_H
#define NETWORKMAPPER_TRACER_H
#include "TraceRouteResult.h"
#include "socks/RawSocket.h"

#define TR_SEND_SLEEP_MS 3
#define RCV_BUFFER_SIZE_TR 1*1024*1024
#define SND_BUFFER_SIZE_TR 1*1024*1024

class InvalidTTLException : public ConfigException{
public:
    InvalidTTLException(Int ttl) : ConfigException("The Time To Live Number is Invalid : " + std::to_string(ttl)) {};
};


class Tracer {

    static void tracing(RawSocket& socket, const std::string& destination,
    uint16_t numOfHops, uint16_t traceID, TraceRouteResult& result, epoll_event ev, Int epollFD);

    static void sendTraceroutePing(RawSocket& socket, uint16_t ttl, const std::string& origin, const std::string& destination, const uint16_t traceID);
    static TraceRouteResult trace(const std::string& destination, uint16_t numOfHops, RawSocket& socket);

public:

    static uint16_t getCustomSeqNum(uint16_t ttl, uint16_t retry) {
        return NUM_OF_PINGS*(ttl-1) + retry + 100;
    }
    static std::vector<TraceRouteResult> multipleTraces(const std::vector<std::string>& destinations, Int numOfHops = 64);
};
#endif //NETWORKMAPPER_TRACER_H
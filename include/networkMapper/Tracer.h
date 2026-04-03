//
// Created by miguel on 3/28/26.
//

#ifndef NETWORKMAPPER_TRACER_H
#define NETWORKMAPPER_TRACER_H
#include "TraceRouteResult.h"
#include "socks/RawSocket.h"

#define TR_SEND_SLEEP_MS 3
#define TR_FINAL_PACKETS_WAIT_MS 30
#define RCV_BUFFER_SIZE_TR 1*1024*1024
#define SND_BUFFER_SIZE_TR 1*1024*1024
#define DEFAULT_MAX_HOP 64

class InvalidTTLException : public ConfigException{
public:
    InvalidTTLException(std::string& ttl) : ConfigException("The Time To Live Number is Invalid : " + ttl) {};
    InvalidTTLException() : ConfigException("The Time To Live Number is Invalid, it should be a positive number within 8 bits.") {};
};


class Tracer {

    static void tracing(RawSocket& socket, const std::string& destination,
    uint8_t numOfHops, uint16_t traceID, TraceRouteResult& result, epoll_event ev, Int epollFD);

    static void sendTraceroutePing(RawSocket& socket, uint8_t ttl, const std::string& origin, const std::string& destination, const uint16_t traceID);
    static TraceRouteResult trace(const std::string& destination, uint8_t numOfHops, RawSocket& socket, uint16_t traceID);

public:

    static uint16_t getCustomSeqNum(uint16_t ttl, uint16_t retry) {
        return NUM_OF_PINGS*(ttl-1) + retry + 100;
    }
    static std::vector<TraceRouteResult> multipleTraces(const std::vector<std::string>& destinations, uint8_t numOfHops = DEFAULT_MAX_HOP);
};
#endif //NETWORKMAPPER_TRACER_H
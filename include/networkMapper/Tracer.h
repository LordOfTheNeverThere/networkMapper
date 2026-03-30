//
// Created by miguel on 3/28/26.
//

#ifndef NETWORKMAPPER_TRACER_H
#define NETWORKMAPPER_TRACER_H
#include <vector>

#include "TraceRouteResult.h"
#define NUM_OF_PINGS 3
#define MAX_NUM_OF_HOPS 255
#define RCV_BUFFER_SIZE_TR 1*1024*1024
#define SND_BUFFER_SIZE_TR 1*1024*1024

class InvalidTTLException : public ConfigException{
public:
    InvalidTTLException(uint16_t ttl) : ConfigException("The Time To Live Number is Invalid : " + std::to_string(ttl)) {};
};


class Tracer {

public:

    static uint16_t getCustomSeqNum(uint16_t ttl, uint16_t retry) {
        return NUM_OF_PINGS*(ttl-1) + retry + 100;
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
    static std::vector<TraceRouteResult> trace(std::string& destination, uint16_t numOfHops = 64);
};
#endif //NETWORKMAPPER_TRACER_H
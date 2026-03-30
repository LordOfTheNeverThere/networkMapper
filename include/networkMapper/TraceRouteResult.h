//
// Created by miguel on 3/27/26.
//

#ifndef NETWORKMAPPER_TRACEROUTERESULT_H
#define NETWORKMAPPER_TRACEROUTERESULT_H
#include "socks/ExternalInterface.h"

#define TRACEROUTE_HOP_NUM 64
class TraceRouteResult {
    ExternalInterface m_interface {};
    std::vector<ExternalInterface> m_path {};
public:
    TraceRouteResult(ExternalInterface& interface, Int numOfHops = TRACEROUTE_HOP_NUM) : m_interface(interface) {
        m_path.reserve(numOfHops);
    }
};


#endif //NETWORKMAPPER_TRACEROUTERESULT_H
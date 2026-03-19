#include <string>
#include <vector>


#include "socks/LocalHost.h"
#include "socks/RawSocket.h"
#include "socks/types.h"

class Mapper {

    std::string m_ipAddress{};
    std::string m_networkMask{};
    sa_family_t m_ipVersion {};
    std::vector<ExternalInterface> m_neighbours{};
    std::vector<std::string> ipsToTrace{};

    //TODO: This is just a plain structure, maybe the we can put this in the caller directly
    void arpMappingSending(RawSocket& socket, InternalInterface& originInterface) {

        socket.sendArpEchoRequest(m_ipAddress, originInterface.getMacAddress(),
            originInterface.getIPAddress(), originInterface.getInterfaceName());
    }

    void arpMappingReceiving(RawSocket& socket) {
        uint8_t recvBuffer[ETH_FRAME_LEN] {};
        socket.receiveArpEchoReply(recvBuffer);
        ExternalInterface neighbour {};
        neighbour.populateFromARPEchoReply(recvBuffer);
        m_neighbours.push_back(neighbour);
    }

    void mapLocalNetwork() {

        LocalHost m_myMachine{LocalHost(true)};


        // Do ARP Mapping


    }
public:

    Mapper(const std::string& ipAddress, const std::string& networkMask, const sa_family_t ipVersion)
    : m_ipAddress{ipAddress}, m_networkMask{networkMask}, m_ipVersion{ipVersion} {}


    void getTraceRoute(const std::string& destIPAddr, const Int& hops = 64) {

    }
};

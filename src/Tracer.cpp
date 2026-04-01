
#include <deque>
#include <random>
#include "networkMapper/Mapper.h"
#include "networkMapper/Tracer.h"
#include "networkMapper/TraceRouteResult.h"
#include "socks/RawSocket.h"


void Tracer::sendTraceroutePing(RawSocket& socket, uint16_t ttl, const std::string& origin,
    const std::string& destination, const uint16_t traceID) {
    uint8_t ipHeaderSendBuffer[sizeof(ip)] {};
    IPv4Header ipHeader{ipHeaderSendBuffer, origin.c_str(), destination.c_str(),0, 0};

    ipHeader.setTTL(ttl);
    for (uint16_t retryNum = 0; retryNum < NUM_OF_PINGS; retryNum++) {
        uint16_t seqNum {Tracer::getCustomSeqNum(ttl, retryNum)};
        try {
            size_t size = socket.sendPing(destination, seqNum, traceID, ipHeader);
            std::this_thread::sleep_for(std::chrono::milliseconds(TR_SEND_SLEEP_MS));
        } catch (SystemCallException& e) {
            std::cerr << e.what() << '\n';
        }
    }

}


void Tracer::tracing(RawSocket& socket, std::string& destination,
    uint16_t numOfHops, uint16_t traceID, TraceRouteResult& result, epoll_event ev, Int epollFD) {

    std::deque<std::array<uint8_t, IP_MAXPACKET>> bufferVector {};
    LocalHost myMachine {true};
    InternalInterface outboundInterface {myMachine.getInterfaceFromSubnet(Tools::getDefaultGateway(), AF_INET)};
    std::string origin {outboundInterface.getIPAddress()};


    bool destinationReached {false};
    bool finished {false};
    for (uint16_t ttl = 1 ; !finished && ttl <= numOfHops; ttl++) { // Allow N empty buffers before formally stopping

        //sending until destination is reached
        if (!destinationReached) {
            sendTraceroutePing(socket, ttl, origin, destination, traceID);
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(30)); // wait for all packets to arrive
            finished = true;
        }

        while (true) { // Collect all packets at the buffer for posterior processing
            Int epollRes = epoll_wait(epollFD, &ev, 1,10000); // wait for their arrival
            if (epollRes == -1) {
                throw EpollWaitException();
            } else if (epollRes == 0) {
                //timeout The intermediary router might be blocking ICMP go the next TTL
                break;
            } else if (ev.data.fd != socket.m_socket || !(ev.events & EPOLLIN)) {
                 // Double-checking if epoll is functioning properly
                 continue;
            }
            try {
                int64_t numBytesRecv {};
                while (true) {
                    std::array<uint8_t, IP_MAXPACKET> recvBuffer {};
                    // Will not filter by seq or ID yet since the reply might be and ICMP error message with these fields blank

                    numBytesRecv = socket.receivePing(recvBuffer.data(), "", 0, 0, ICMP_TIME_EXCEEDED);
                    if (numBytesRecv == 0) {
                        continue; //Rare: Client Closed connection or empty packet
                    }
                    bufferVector.push_back(recvBuffer);
                }
            } catch (std::exception& e) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    break; //EWOULDBLOCK or EAGAIN, buffer is empty
                } else {
                    std::cerr << e.what() << '\n';
                }
            }
        }
        while (!bufferVector.empty()) { // process packets
            std::array<uint8_t, IP_MAXPACKET> packet {std::move(bufferVector.front())};// transfer ownership of the data
            bufferVector.pop_front();// delete empty entry
            TraceRouteHop hop = result.addEntryFromEchoReply(packet.data(), numOfHops, traceID, destination);
            if (hop.getIPAddress() == destination) {
                destinationReached = true;
            }
        }
    }

}


TraceRouteResult Tracer::trace(std::string& destination, uint16_t numOfHops) {
    TraceRouteResult result {};

    RawSocket socket {AF_INET, IPPROTO_ICMP};
    socket.setSocketAsNonBlock();
    socket.setSocketIPHeaderManually(1);

    auto traceID = static_cast<uint16_t>(socket.m_socket & 0xFFFF);

    Int epollFD = epoll_create1(0);
    if (epollFD == -1) {
        throw EpollCreationException();
    }
    try {
        epoll_event ev {};
        ev.events = EPOLLIN;
        ev.data.fd = socket.m_socket;
        Int epollRes = epoll_ctl(epollFD, EPOLL_CTL_ADD, socket.m_socket, &ev);
        if (epollRes == -1) {
            throw EpollControllerException(EPOLL_CTL_ADD);
        }
        tracing(socket, destination, numOfHops, traceID, result, ev, epollFD);
    } catch (std::exception& e) {
        close(epollFD); // in case an exception occurs we can close what we need
        throw;
    }
    close(epollFD);

    return result;
};

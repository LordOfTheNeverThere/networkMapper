
#ifndef NETWORKMAPPER_MAPPER_H
#define NETWORKMAPPER_MAPPER_H
#include "../../cmake-build-debug/_deps/googletest-src/googletest/include/gtest/gtest_prod.h"
#include "socks/LocalHost.h"
#include "socks/ExternalInterface.h"
#include "socks/RawSocket.h"
#include <unordered_map>
#include <deque>
#include <netinet/ip_icmp.h>

class Mapper {
    friend class Tracer;
    FRIEND_TEST(MethodChecking, simpleThreadChecking);
    sa_family_t m_ipVersion {};
    std::vector<ExternalInterface> m_neighbours{};

    template <size_t N> //Variable packet size
    static void receiving(const RawSocket& socket, std::deque<std::array<uint8_t, N>>& bufferVector,
        bool& finished, std::mutex& exclusioner, std::condition_variable& conditionVar,  const bool isARP,
        const Int icmpType = ICMP_ECHOREPLY, const uint64_t processID = 0) {
        Int epollFD = epoll_create1(0);
        if (epollFD == -1) {
            throw EpollCreationException();
        }
        try {
            epoll_event ev {};
            ev.events = EPOLLIN;
            ev.data.fd = socket.m_socket;
            Int result = epoll_ctl(epollFD, EPOLL_CTL_ADD, socket.m_socket, &ev);
            if (result == -1) {
                throw EpollControllerException(EPOLL_CTL_ADD);
            }
            while (true) {
                result = epoll_wait(epollFD, &ev, 1,10000);
                if (result == -1) {
                    throw EpollWaitException();
                } else if (result == 0) {
                    //timeout
                    close(epollFD);
                    finished = true;
                    conditionVar.notify_one();
                    return;
                }  if (ev.data.fd == socket.m_socket && ev.events & EPOLLIN){
                    std::array<uint8_t, N> recvBuffer {};
                    int64_t numBytesRecv {};
                    try{
                        if (isARP) {
                            numBytesRecv = socket.receiveArpEchoReply(recvBuffer.data());
                        } else {
                            numBytesRecv = socket.receivePing(
                                recvBuffer.data(), "", 0, processID, icmpType);
                        }
                    } catch (std::exception& e) {

                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            continue; //EWOULDBLOCK or EAGAIN, kernel dropped the packet for some reason, such as CRC corruption
                        } else {
                            std::cerr << e.what() << '\n';
                        }
                    }
                    if (numBytesRecv == 0) {
                        continue; //Rare: Client Closed connection or empty packet
                    }

                    std::unique_lock<std::mutex> lock(exclusioner);
                    bufferVector.push_back(recvBuffer);
                    lock.unlock();
                    conditionVar.notify_one();
                    //std::cout << "Received a packet, loading into the buffer" << '\n';
                }
            }
        } catch (std::runtime_error& e) {
            close(epollFD); // in case an exception occurs we can close what we need
            throw;
        }
    };


public:

    static std::vector<ExternalInterface> mapLocalNetwork(const std::unordered_map<std::string, std::vector<uint32_t>>& localIPsToMap,
        const LocalHost& machine = LocalHost(true));

    static std::vector<ExternalInterface> mapNonLocalNetwork(const std::vector<uint32_t>& nonLocalIPsToMap);

    std::vector<ExternalInterface> mapNetwork(const std::vector<uint32_t>& nonLocalIPsToMap,
        const std::unordered_map<std::string, std::vector<uint32_t>>& localIPsToMap,
        LocalHost myMachine = LocalHost(true));

    Mapper(const sa_family_t ipVersion);

#define MAX_RETRIES 5
#define PING_SEND_BATCH_SIZE 64
#define PING_SEND_BATCH_SLEEP_MS 1
#define ARP_SEND_BATCH_SIZE 64
#define ARP_SEND_BATCH_SLEEP_MS 1
#define RCV_BUFFER_SIZE 4*1024*1024
#define SND_BUFFER_SIZE 4*1024*1024
};
#endif //NETWORKMAPPER_MAPPER_H
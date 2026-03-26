#include <condition_variable>
#include <deque>
#include <string>
#include <unordered_map>
#include <vector>
#include <thread>
#include <sys/epoll.h>

#include "socks/LocalHost.h"
#include "socks/RawSocket.h"
#include "socks/types.h"
#include "networkMapper/Mapper.h"

#include <random>
Mapper::Mapper(const sa_family_t ipVersion)
: m_ipVersion{ipVersion} {}




static void arpMappingSending(const RawSocket& socket, const std::vector<uint32_t>& ips, const InternalInterface& interface) {

    uint32_t srcIPAddressNum {};
    Int conversionResult = inet_pton(AF_INET, interface.getIPAddress().c_str(), &srcIPAddressNum);
    if (conversionResult != 1) {
        throw ConversionToIPBinaryException(interface.getIPAddress());
    }
    std::string interfaceName {interface.getInterfaceName()};
    std::string interfaceMAC {interface.getMacAddress()};

    Int retries {MAX_RETRIES};
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<std::mt19937::result_type> uni1To10(1,10);

    for (Int i = 0; i < ips.size(); ++i) {
        try {
            //std::cout << "Sending ARP Echo Request on Interface " << interfaceName << '\n';
            socket.sendArpEchoRequest(ips[i], interfaceMAC, srcIPAddressNum, interfaceName);
            retries = 5;
        } catch (SendingException& se) {
            if (retries == 0) {
                char ipString[INET_ADDRSTRLEN] {};
                inet_ntop(AF_INET, &ips[i], ipString, INET_ADDRSTRLEN);
                std::cerr << "Sending the ARP Echo request to " << ipString << " failed after " << std::to_string(MAX_RETRIES) << " times" << '\n'
                << "Reason: " << std::system_category().message(errno) << '\n';
                continue;
            }
            retries--;
            i--;
            std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Add more waiting time in case the socket is presently occupied
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(uni1To10(rng)));
    }
}
template <size_t N> //Variable packet size
static void bufferHandler(std::deque<std::array<uint8_t, N>>& bufferVector,std::vector<ExternalInterface>& neighbours,
    const bool& finished, std::mutex& exclusioner, std::condition_variable& conditionVar, const bool isARP) {


    while (true) {
        std::unique_lock<std::mutex> lock(exclusioner);
        conditionVar.wait(lock, [&] {
            return finished || !bufferVector.empty(); // Protection against spurious wake-ups
        });
        if (bufferVector.empty()) {
            // Receiver has ceased receiving and no more data is waiting for handling
            //std::cout << "Receiver has ceased receiving and no more data is waiting for handling" << '\n';
            break;
        } else {
            std::deque<std::array<uint8_t, N>> localBuffer {std::move(bufferVector)};
            bufferVector.clear();
            lock.unlock(); //Allow receiver to continue receiving
            //std::cout << "Will add " << std::to_string(localBuffer.size()) << " new neighbours" << '\n';
            while (!localBuffer.empty()) {

                std::array<uint8_t, N> packet {std::move(localBuffer.front())};// transfer ownership of the data
                localBuffer.pop_front();// delete empty entry
                ExternalInterface neighbour {};
                if (isARP) {
                    neighbour.populateFromARPEchoReply(packet.data());
                } else {
                    neighbour.populateFromICMPEchoReply(packet.data());
                }
                neighbours.push_back(neighbour);
            }
        }
    }
}
template <size_t N> //Variable packet size
static void receiving(const RawSocket& socket, std::deque<std::array<uint8_t, N>>& bufferVector,
    bool& finished, std::mutex& exclusioner, std::condition_variable& conditionVar,  const bool isARP) {
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
                uint64_t numBytesRecv {};
                try{
                    if (isARP) {
                        numBytesRecv = socket.receiveArpEchoReply(recvBuffer.data());
                    } else {
                        numBytesRecv = socket.receivePing(recvBuffer.data());
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
}

std::vector<ExternalInterface> Mapper::mapLocalNetwork(LocalHost& machine, const std::unordered_map<std::string, std::vector<uint32_t>>& localIPsToMap, RawSocket& socket) {

    socket.setSocketAsNonBlock();
    socket.setSocketReceiveBuffer(1*1024*1024);
    std::vector<std::thread> senders{};
    senders.reserve(localIPsToMap.size());
    std::vector<ExternalInterface> neighbours{};
    std::deque<std::array<uint8_t, ETH_FRAME_LEN>> bufferVector {};

    std::mutex exclusioner {};
    std::condition_variable conditionVar {};
    bool finished = false;


    for (auto& mapEle : localIPsToMap) {


        std::thread newThread(
            arpMappingSending,
            std::cref(socket),
            std::cref(mapEle.second),
            machine.getInterfaceByName(mapEle.first));
        senders.push_back(std::move(newThread));
    }

    std::thread receiver(receiving<ETH_FRAME_LEN>, std::cref(socket), std::ref(bufferVector),
        std::ref(finished), std::ref(exclusioner), std::ref(conditionVar), true);
    std::thread handler(bufferHandler<ETH_FRAME_LEN>, std::ref(bufferVector), std::ref(neighbours),
        std::cref(finished), std::ref(exclusioner), std::ref(conditionVar), true);

    for (std::thread& sender: senders) {
        sender.join();
    }
    receiver.join();
    handler.join();

    return neighbours;
}



static void pingMappingSending(RawSocket& socket, const std::vector<uint32_t>& ips) {


    Int retries {MAX_RETRIES};
    for (Int i = 0; i < ips.size(); ++i) {
        try {
             // char ipString[INET_ADDRSTRLEN] {};
             // inet_ntop(AF_INET, &ips[i], ipString, INET_ADDRSTRLEN);
             // std::cout << "Sending ICMP Echo Request to " << ipString << '\n';
            socket.sendPingIPv4Only(ips[i]);
            retries = 5;
        } catch (SendingException& se) {
            if (retries == 0) {
                char ipString[INET_ADDRSTRLEN] {};
                inet_ntop(AF_INET, &ips[i], ipString, INET_ADDRSTRLEN);
                std::cerr << "Sending the ICMP Echo request to " << ipString << " failed after " << std::to_string(MAX_RETRIES) << " times" << '\n'
                << "Reason: " << std::system_category().message(errno) << '\n';
                continue;
            }
            retries--;
            i--;
            std::this_thread::sleep_for(std::chrono::milliseconds(PING_SEND_BATCH_SLEEP_MS)); // Add more waiting time in case the socket is presently occupied
        }
        if (i % PING_SEND_BATCH_SIZE == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(PING_SEND_BATCH_SLEEP_MS));
        }
    }
}

std::vector<ExternalInterface> Mapper::mapNonLocalNetwork(const std::vector<uint32_t>& nonLocalIPsToMap, RawSocket& socket) {

    socket.setSocketAsNonBlock();
    socket.setSocketReceiveBuffer(RCV_BUFFER_SIZE);
    uint64_t buff {socket.getSocketRcvBuffer()};
    std::vector<ExternalInterface> neighbours{};
    bool finished = false;
    std::deque<std::array<uint8_t, IP_MAXPACKET>> bufferVector {};
    std::mutex exclusioner {};
    std::condition_variable conditionVar {};

    std::thread sender(
           pingMappingSending,
           std::ref(socket),
           std::cref(nonLocalIPsToMap));

    std::thread receiver(receiving<IP_MAXPACKET>, std::cref(socket), std::ref(bufferVector),
        std::ref(finished), std::ref(exclusioner), std::ref(conditionVar), false);
    std::thread handler(bufferHandler<IP_MAXPACKET>, std::ref(bufferVector), std::ref(neighbours),
        std::cref(finished), std::ref(exclusioner), std::ref(conditionVar), false);

    sender.join();
    receiver.join();
    handler.join();

    return neighbours;
}

void Mapper::getTraceRoute(const std::string& destIPAddr, const Int& hops) {

}


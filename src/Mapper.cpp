#include <condition_variable>
#include <string>
#include <unordered_map>
#include <vector>
#include <thread>
#include <sys/epoll.h>

#include "socks/LocalHost.h"
#include "socks/types.h"
#include "networkMapper/Mapper.h"
#include <future>

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
            std::this_thread::sleep_for(std::chrono::milliseconds(ARP_SEND_BATCH_SLEEP_MS));
        }
        if (i % ARP_SEND_BATCH_SIZE == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(ARP_SEND_BATCH_SLEEP_MS));
        }
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

std::vector<ExternalInterface> Mapper::mapLocalNetwork(
    const std::unordered_map<std::string, std::vector<uint32_t>>& localIPsToMap, const LocalHost& machine) {

    if (localIPsToMap.empty()) {
        return std::vector<ExternalInterface>{};
    }

    RawSocket socket = RawSocket(AF_PACKET, htons(ETH_P_ARP));
    socket.setSocketAsNonBlock();
    socket.setSocketReceiveBuffer(RCV_BUFFER_SIZE);
    socket.setSocketSendBuffer(SND_BUFFER_SIZE);
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
        std::ref(finished), std::ref(exclusioner), std::ref(conditionVar), true, 0, 0);
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

std::vector<ExternalInterface> Mapper::mapNonLocalNetwork(const std::vector<uint32_t>& nonLocalIPsToMap) {
    if (nonLocalIPsToMap.empty()) {
        return std::vector<ExternalInterface>{};
    }

    RawSocket socket {AF_INET, IPPROTO_ICMP};
    socket.setSocketAsNonBlock();
    socket.setSocketReceiveBuffer(RCV_BUFFER_SIZE);
    socket.setSocketSendBuffer(SND_BUFFER_SIZE);
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
        std::ref(finished), std::ref(exclusioner), std::ref(conditionVar), false, 0, 0);
    std::thread handler(bufferHandler<IP_MAXPACKET>, std::ref(bufferVector), std::ref(neighbours),
        std::cref(finished), std::ref(exclusioner), std::ref(conditionVar), false);

    sender.join();
    receiver.join();
    handler.join();
    return neighbours;
}

std::vector<ExternalInterface> Mapper::mapNetwork(
    const std::vector<uint32_t>& nonLocalIPsToMap,
    const std::unordered_map<std::string, std::vector<uint32_t>>& localIPsToMap,
    LocalHost myMachine
    ) {

    std::future<std::vector<ExternalInterface>> nonLocalMapper = std::async(mapNonLocalNetwork, std::cref(nonLocalIPsToMap));
    std::future<std::vector<ExternalInterface>> localMapper = std::async(mapLocalNetwork, std::cref(localIPsToMap), std::ref(myMachine));

    m_neighbours = std::move(localMapper.get());
    std::vector<ExternalInterface> nonLocalNeighbours {nonLocalMapper.get()};
    m_neighbours.insert(
    m_neighbours.end(),
    std::make_move_iterator(nonLocalNeighbours.begin()),
    std::make_move_iterator(nonLocalNeighbours.end()));

    return m_neighbours;
}


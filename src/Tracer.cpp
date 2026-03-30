#include "networkMapper/Tracer.h"

#include <deque>


#include "networkMapper/Mapper.h"
#include "networkMapper/TraceRouteResult.h"
#include "socks/RawSocket.h"



static void bufferHandler(std::deque<std::array<uint8_t, IP_MAXPACKET>>& bufferVector,std::vector<ExternalInterface>& neighbours,
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
            std::deque<std::array<uint8_t, IP_MAXPACKET>> localBuffer {std::move(bufferVector)};
            bufferVector.clear();
            lock.unlock(); //Allow receiver to continue receiving
            //std::cout << "Will add " << std::to_string(localBuffer.size()) << " new neighbours" << '\n';
            while (!localBuffer.empty()) {

                std::array<uint8_t, IP_MAXPACKET> packet {std::move(localBuffer.front())};// transfer ownership of the data
                localBuffer.pop_front();// delete empty entry
                //TraceRouteResult neighbour {};// TODO: Find way to validate and construct the Result from the buffer. Validade Seq Number

            }
        }
    }
}



void sendTraceroutePing(RawSocket& socket, uint16_t numOfHops, const std::string& origin, const std::string& destination) {
    uint8_t ipHeaderSendBuffer[sizeof(ip)] {};
    IPv4Header ipHeader{ipHeaderSendBuffer, origin.c_str(), destination.c_str(),0, 0};

    for (uint16_t ttl = 1; ttl < numOfHops; ttl++) {
        ipHeader.setTTL(ttl);
        for (uint16_t retryNum = 0; retryNum < NUM_OF_PINGS; retryNum++) {
            try {
                socket.sendPing(destination, Tracer::getCustomSeqNum(ttl, retryNum), 0, ipHeader);
            } catch (SystemCallException& e) {
                std::cerr << e.what() << '\n';
            }
        }
    }
}



std::vector<TraceRouteResult> Tracer::trace(std::string& destination, uint16_t numOfHops) {
    std::vector<TraceRouteResult> result {};
    if (destination.empty()) {
        return result;
    }
    if (numOfHops > MAX_NUM_OF_HOPS) {
        throw InvalidTTLException(numOfHops);
    }

    RawSocket socket {AF_INET, IPPROTO_ICMP};
    socket.setSocketAsNonBlock();
    socket.setSocketReceiveBuffer(RCV_BUFFER_SIZE_TR);
    socket.setSocketSendBuffer(SND_BUFFER_SIZE_TR);
    socket.setSocketIPHeaderManually(1);

    bool finished = false;
    std::deque<std::array<uint8_t, IP_MAXPACKET>> bufferVector {};
    std::mutex exclusioner {};
    std::condition_variable conditionVar {};

    std::thread sender(
        sendTraceroutePing,
        std::ref(socket),
        numOfHops,
        Tools::getDefaultGateway(),
        destination);


    std::thread receiver(Mapper::receiving<IP_MAXPACKET>, std::cref(socket), std::ref(bufferVector),
        std::ref(finished), std::ref(exclusioner), std::ref(conditionVar), false);
    // std::thread handler(bufferHandler<IP_MAXPACKET>, std::ref(bufferVector), std::ref(neighbours),
    //     std::cref(finished), std::ref(exclusioner), std::ref(conditionVar), false);
    //
    sender.join();
    receiver.join();
    // handler.join();
    return result;
};

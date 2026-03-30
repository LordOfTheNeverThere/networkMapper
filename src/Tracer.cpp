
#include <deque>
#include <random>
#include "networkMapper/Mapper.h"
#include "networkMapper/Tracer.h"
#include "networkMapper/TraceRouteResult.h"
#include "socks/RawSocket.h"



static void bufferHandler(std::deque<std::array<uint8_t, IP_MAXPACKET>>& bufferVector, TraceRouteResult& result,
    const bool& finished, std::mutex& exclusioner, std::condition_variable& conditionVar, const uint16_t numOfHops) {

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
                result.addEntryFromEchoReply(packet.data(), numOfHops);
            }
        }
    }
}



void sendTraceroutePing(RawSocket& socket, uint16_t numOfHops, const std::string& origin,
    const std::string& destination, const uint64_t processID) {
    uint8_t ipHeaderSendBuffer[sizeof(ip)] {};
    IPv4Header ipHeader{ipHeaderSendBuffer, origin.c_str(), destination.c_str(),0, 0};

    for (uint16_t ttl = 1; ttl < numOfHops; ttl++) {
        ipHeader.setTTL(ttl);
        for (uint16_t retryNum = 0; retryNum < NUM_OF_PINGS; retryNum++) {
            try {
                socket.sendPing(destination, Tracer::getCustomSeqNum(ttl, retryNum), processID, ipHeader);
            } catch (SystemCallException& e) {
                std::cerr << e.what() << '\n';
            }
        }
    }
}



TraceRouteResult Tracer::trace(std::string& destination, uint16_t numOfHops) {
    TraceRouteResult result {};

    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<std::mt19937::result_type> uni70To7070(70,7070);
    uint64_t processID {uni70To7070(rng)};

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
        destination,
        processID);


    std::thread receiver(Mapper::receiving<IP_MAXPACKET>, std::cref(socket), std::ref(bufferVector),
        std::ref(finished), std::ref(exclusioner), std::ref(conditionVar), false, ICMP_TIME_EXCEEDED, processID);
    std::thread handler(bufferHandler, std::ref(bufferVector), std::ref(result),
        std::cref(finished), std::ref(exclusioner), std::ref(conditionVar), numOfHops);

    sender.join();
    receiver.join();
    handler.join();

    return result;
};

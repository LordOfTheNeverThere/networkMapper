#include "gtest/gtest.h"
#include "networkMapper/IPv4Range.h"
#include "socks/LocalHost.h"
#include "networkMapper/Mapper.h"

#include <thread>

TEST(MethodChecking, simpleMapLocalNetworkCheck) {

    LocalHost myMachine {LocalHost(true)};
    std::string defaultGateway {Tools::getDefaultGateway()};
    InternalInterface interfaceWithGateway = myMachine.getInterfaceFromSubnet(defaultGateway, AF_INET);
    IPv4Range range {IPv4Range(interfaceWithGateway.getIPAddress(), interfaceWithGateway.getNetworkMask(), myMachine)};
    RawSocket socket {AF_PACKET, htons(ETH_P_ARP)};

    Mapper mapper {AF_INET};
    auto result = Mapper::mapLocalNetwork(myMachine,range.getIPsLocal(), socket);
    EXPECT_TRUE(result.size() >= 1);
}


TEST(MethodChecking, dualSenderMapLocalNetworkCheck) {

    LocalHost myMachine {LocalHost(true)};
    std::string defaultGateway {Tools::getDefaultGateway()};
    InternalInterface interfaceWithGateway = myMachine.getInterfaceFromSubnet(defaultGateway, AF_INET);
    IPv4Range range {IPv4Range(interfaceWithGateway.getIPAddress(), interfaceWithGateway.getNetworkMask(), myMachine)};
    Int numOfPacketsToSend {50};
    range.m_ipsRangeLocal["lo"].insert(range.m_ipsRangeLocal["lo"].begin() , range.m_ipsRangeLocal[interfaceWithGateway.getInterfaceName()].begin(), std::next(range.m_ipsRangeLocal[interfaceWithGateway.getInterfaceName()].begin(), numOfPacketsToSend));

    RawSocket socket {AF_PACKET, htons(ETH_P_ARP)};
    std::string cmdOutput {};
    bool run {true};
    std::string cmd = "tcpdump -i lo -n ether host ff:ff:ff:ff:ff:ff -c" + std::to_string(numOfPacketsToSend);
    std::thread worker(Tools::getOutputFromCommand, std::ref(cmdOutput), std::cref(cmd), std::ref(run));
    std::this_thread::sleep_for(std::chrono::seconds(5)); // so that worker has time to initiate the packet tracer
    auto result = Mapper::mapLocalNetwork(myMachine,range.getIPsLocal(), socket);
    run = false;
    worker.join();

    std::string::difference_type numOfLines = std::count(cmdOutput.begin(), cmdOutput.end(), '\n');
    EXPECT_EQ(numOfLines, numOfPacketsToSend);
}
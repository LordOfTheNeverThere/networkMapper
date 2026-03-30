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

    Mapper mapper {AF_INET};
    auto result = Mapper::mapLocalNetwork(range.getIPsLocal());
    EXPECT_TRUE(result.size() >= 1);
}



TEST(MethodChecking, dualSenderMapLocalNetworkCheck) {

    LocalHost myMachine {LocalHost(true)};
    std::string networkIP {"127.0.0.1"};
    std::string networkMask {"255.255.255.192"};
    InternalInterface loInterface {networkIP, networkMask, "000000000000", AF_INET, "lo"};
    myMachine.pushDataToInterfaces(loInterface); // mock loopback interface
    IPv4Range range {networkIP, networkMask, myMachine};
    uint32_t networkMaskInBytes {};
    inet_pton(AF_INET, networkMask.c_str(), &networkMaskInBytes);
    uint32_t numOfPacketsToSend {Tools::getNumberOfIPsInMaskIPv4(networkMaskInBytes)};

    std::string cmdOutput {};
    bool run {true};
    std::string cmd = "tcpdump -i lo -n ether host ff:ff:ff:ff:ff:ff -c" + std::to_string(numOfPacketsToSend);
    std::thread worker(Tools::getOutputFromCommand, std::ref(cmdOutput), std::cref(cmd), std::ref(run));
    std::this_thread::sleep_for(std::chrono::seconds(5)); // so that worker has time to initiate the packet tracer
    auto result = Mapper::mapLocalNetwork(range.getIPsLocal(),myMachine);
    run = false;
    worker.join();

    std::string::difference_type numOfLines = std::count(cmdOutput.begin(), cmdOutput.end(), '\n');
    EXPECT_EQ(numOfLines, numOfPacketsToSend);
}

TEST(MethodChecking, checkIfAlgoHandlesAllReplies) {
    LocalHost myMachine {LocalHost(true)};
    IPv4Range range {IPv4Range("127.0.0.0", "255.255.0.0", myMachine)};
    Mapper mapper {AF_INET};
    auto result = Mapper::mapNonLocalNetwork(range.getIPsNonLocal());
    EXPECT_EQ(result.size(), range.getIPsNonLocal().size());
}

TEST(MethodChecking, mapNonLocalInternet) {
    LocalHost myMachine {LocalHost(true)};
    IPv4Range rangeNonLocal {"8.8.8.8", "255.255.0.0", myMachine};
    Mapper mapper {AF_INET};
    auto result = Mapper::mapNonLocalNetwork(rangeNonLocal.getIPsNonLocal());
    EXPECT_TRUE(true); //TODO: CHECK if MS sleep needs to be adjusted for Pings (compare with nmap)
}

TEST(MethodChecking, mapNetwork2Local1NonLocal) {
    LocalHost myMachine {LocalHost(true)};
    std::string networkIP {"127.0.0.1"};
    std::string networkMask {"255.255.0.0"};
    InternalInterface loInterface {networkIP, networkMask, "000000000000", AF_INET, "lo"};
    myMachine.pushDataToInterfaces(loInterface); // mock loopback interface
    IPv4Range rangeLocal1 {networkIP, networkMask, myMachine}; //Local Range 1

    std::string defaultGateway {Tools::getDefaultGateway()};
    InternalInterface interfaceWithGateway = myMachine.getInterfaceFromSubnet(defaultGateway, AF_INET);
    IPv4Range rangeLocal2 {IPv4Range(interfaceWithGateway.getIPAddress(), interfaceWithGateway.getNetworkMask(), myMachine)}; //Local Range 2
    IPv4Range rangeLocal {rangeLocal1 + rangeLocal2};
    IPv4Range rangeNonLocal {"8.8.8.8", "255.255.0.0", myMachine};// Non-Local Range

    Mapper mapper {AF_INET};
    auto result = mapper.mapNetwork(rangeNonLocal.getIPsNonLocal(), rangeLocal.getIPsLocal(), myMachine);
    EXPECT_GT(result.size(), 0 + 1 + 1); // Loopback addresses do not respond to ARP + at least the gateway + at least the google DNS server
    // 980
}
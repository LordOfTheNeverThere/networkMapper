
#include "networkMapper/IPv4Range.h"

#include "gtest/gtest.h"
#include "socks/LocalHost.h"



TEST(MethodChecking, OnlyLocal) {

    LocalHost myMachine {LocalHost(true)};
    InternalInterface localInterface {};
    for (auto interface: myMachine.getInterfaces()) {
        if (interface.getIPVersion() == AF_INET && !interface.getNetworkMask().empty() && !interface.getIPAddress().empty()) {
            localInterface = interface;
            break;
        }
    }
    IPv4Range range {IPv4Range(localInterface.getIPAddress(), localInterface.getNetworkMask(), myMachine)};

    EXPECT_EQ(0, range.getIPsNonLocal().size());

    sockaddr_in maskSockAddr {};
    Int result = inet_pton(AF_INET, localInterface.getNetworkMask().c_str(), &maskSockAddr.sin_addr);
    if (result != 1) {
        throw ConversionToIPBinaryException(localInterface.getNetworkMask());
    }
    EXPECT_EQ(Tools::getNumberOfIPsInMaskIPv4(maskSockAddr.sin_addr.s_addr), range.getIPsLocal().begin()->second.size());
}


TEST(MethodChecking, OnlyNonLocal) {

    LocalHost myMachine {LocalHost(true)};

    std::string networkIP {"8.8.0.0"};
    std::string networkMaskIP {"255.255.0.0"};

    IPv4Range range {IPv4Range(networkIP, networkMaskIP, myMachine)};

    EXPECT_EQ(0, range.getIPsLocal().size());

    sockaddr_in maskSockAddr {};
    Int result = inet_pton(AF_INET, networkMaskIP.c_str(), &maskSockAddr.sin_addr);
    if (result != 1) {
        throw ConversionToIPBinaryException(networkMaskIP);
    }

    EXPECT_EQ(Tools::getNumberOfIPsInMaskIPv4(maskSockAddr.sin_addr.s_addr), range.getIPsNonLocal().size());
}


TEST(MethodChecking, NonLocalAndLocal) {

    LocalHost myMachine {LocalHost(true)};
    InternalInterface interfaceWithGateway = myMachine.getInterfaceFromSubnet(Tools::getDefaultGateway(), AF_INET);
    std::string networkMask16 {"255.255.0.0"};
    sockaddr_in maskSockAddr16 {};
    Int result = inet_pton(AF_INET, networkMask16.c_str(), &maskSockAddr16.sin_addr);
    if (result != 1) {
        throw ConversionToIPBinaryException(networkMask16);
    }
    IPv4Range range {IPv4Range(interfaceWithGateway.getIPAddress(), networkMask16, myMachine)};
    uint32_t numberOfIPs {Tools::getNumberOfIPsInMaskIPv4(maskSockAddr16.sin_addr.s_addr)};

    for (const auto& ips: range.getIPsLocal()) {
        numberOfIPs = numberOfIPs - ips.second.size();
    }
    EXPECT_NE(0, numberOfIPs);
    numberOfIPs = numberOfIPs - range.getIPsNonLocal().size();
    EXPECT_EQ(0, numberOfIPs);
}

TEST(ErrorChecking, ExcessiveNetworkMask) {

    LocalHost myMachine {LocalHost(true)};
    InternalInterface interfaceWithGateway = myMachine.getInterfaceFromSubnet(Tools::getDefaultGateway(), AF_INET);
    std::string networkMask16 {"255.254.0.0"};

    EXPECT_THROW({
        try {
            IPv4Range range {IPv4Range(interfaceWithGateway.getIPAddress(), networkMask16, myMachine)};
        } catch( const IPv4OutOfRangeException& e ) {
            EXPECT_STREQ("The smallest network mask possible is 255.255.0.0 aka /16"
                , e.what());
            throw; // Re-throw so EXPECT_THROW sees it
        }
    }, IPv4OutOfRangeException);
}


TEST(MethodChecking, plusOperatorOverloading) {

    LocalHost myMachine {LocalHost(true)};
    std::string networkIP {"127.0.0.1"};
    std::string networkMask {"255.255.255.192"};
    InternalInterface loInterface {networkIP, networkMask, "000000000000", AF_INET, "lo"};
    myMachine.pushDataToInterfaces(loInterface); // mock loopback interface
    IPv4Range rangeLocal1 {networkIP, networkMask, myMachine}; //Local Range 1

    std::string defaultGateway {Tools::getDefaultGateway()};
    InternalInterface interfaceWithGateway = myMachine.getInterfaceFromSubnet(defaultGateway, AF_INET);
    IPv4Range rangeLocal2 {IPv4Range(interfaceWithGateway.getIPAddress(), interfaceWithGateway.getNetworkMask(), myMachine)}; //Local Range 2

    IPv4Range rangeNonLocal {"8.8.8.8", "255.255.255.0", myMachine};// Non Local Range

    IPv4Range whollyRange {rangeLocal1 + rangeLocal2 + rangeNonLocal};
    EXPECT_EQ(whollyRange.getIPsLocal().size(), 2);
    EXPECT_EQ(whollyRange.getIPsNonLocal().size(), 256);
    EXPECT_EQ(whollyRange.getIPsLocal().begin()->second.size(), 64);
    uint32_t interfaceWithGatewayMask {};
    inet_pton(AF_INET, interfaceWithGateway.getNetworkMask().c_str(), &interfaceWithGatewayMask);
    EXPECT_EQ(std::next(whollyRange.getIPsLocal().begin(),1)->second.size(), Tools::getNumberOfIPsInMaskIPv4(interfaceWithGatewayMask));
}
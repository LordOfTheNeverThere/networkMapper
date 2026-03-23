
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
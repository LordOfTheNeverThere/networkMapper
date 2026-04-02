
#include <gtest/gtest.h>
#include "networkMapper/MainLogic.h"

namespace MainLogicTest {
    std::string progName {"test_app"};
    std::string helpString {"Usage: " + progName + " [OPTION] [ARGUMENTS...]\n\n"
              + "Options:\n"
              + "  -m, --map <IP> <NET>    Map an IPv4 address to a network address.\n"
              + "  -t, --trace <IPs...>    Perform a traceroute on a list of IP addresses.\n"
              + "  -h, --help              Display this help and exit.\n"};
}

// Helper to convert a vector of strings to the char** format main expects
int callRunProgram(const std::vector<std::string>& args, bool testing) {
    std::vector<char*> argv;

    for (const auto& arg : args) {
        argv.push_back(const_cast<char*>(arg.c_str()));
    }
    argv.push_back(nullptr); // Null terminator per C standard
    std::string progName {argv[0]};
    EXPECT_EQ(MainLogicTest::progName, progName);

    return mainLogic(static_cast<int>(argv.size() - 1), argv.data(), testing);
}

TEST(CmdLineTest, UnknownFlagFails) {
    testing::internal::CaptureStderr();
    testing::internal::CaptureStdout();

    EXPECT_EQ(callRunProgram({MainLogicTest::progName,"-z"}, true), 1);

    EXPECT_EQ(callRunProgram({MainLogicTest::progName, "random_string"}, true), 1);
    std::string err = testing::internal::GetCapturedStderr();
    std::string out = testing::internal::GetCapturedStdout();
    std::string expectedError {"Unknown option: -z\nUnknown option: random_string\n"};
    EXPECT_EQ(err, expectedError);
    EXPECT_EQ(out, MainLogicTest::helpString + MainLogicTest::helpString);
}

TEST(CmdLineTest, HandlesHelpFlag) {
    testing::internal::CaptureStdout();
    EXPECT_EQ(callRunProgram({MainLogicTest::progName,"-h"}, true), 0);
    EXPECT_EQ(callRunProgram({MainLogicTest::progName, "--help"}, true), 0);
    std::string out = testing::internal::GetCapturedStdout();
    EXPECT_EQ(out, MainLogicTest::helpString + MainLogicTest::helpString);
}

TEST(CmdLineTest, Mapping) {
    testing::internal::CaptureStderr();
    testing::internal::CaptureStdout();
    // // Valid case
    EXPECT_EQ(callRunProgram({MainLogicTest::progName,"-m", "192.168.1.1", "192.168.1.0"}, 1), 0);

    // // Missing one arg
    EXPECT_EQ(callRunProgram({MainLogicTest::progName,"-m", "192.168.1.1"}, 1), 1);

    // Missing both args
    EXPECT_EQ(callRunProgram({MainLogicTest::progName,"-m"}, true), 1);
    std::string err = testing::internal::GetCapturedStderr();
    std::string out = testing::internal::GetCapturedStdout();
    EXPECT_EQ("Error: --map requires an IP and a Network address.\nError: --map requires an IP and a Network address.\n", err);
    EXPECT_EQ("Mapping IP: 192.168.1.1 to Network: 192.168.1.0\n", out);
}

TEST(CmdLinetest, Tracing) {
    testing::internal::CaptureStderr();
    testing::internal::CaptureStdout();
    // Empty trace (Error)
    EXPECT_EQ(callRunProgram({MainLogicTest::progName,"-t"}, true), 1);
    // Single IP
    EXPECT_EQ(callRunProgram({MainLogicTest::progName,"-t", "8.8.8.8"}, true), 0);
    // Multiple IPs
    EXPECT_EQ(callRunProgram({MainLogicTest::progName,"-t", "8.8.8.8", "1.1.1.1", "127.0.0.1"}, true), 0);
    std::string err = testing::internal::GetCapturedStderr();
    std::string out = testing::internal::GetCapturedStdout();
    EXPECT_EQ("Error: --trace requires at least one IP address.\n", err);
    EXPECT_EQ("Tracing 1 target(s)...\nTracing 3 target(s)...\n", out);
}

TEST(MainTest, Mapping) {
    testing::internal::CaptureStdout();
    EXPECT_EQ(callRunProgram({MainLogicTest::progName,"-m", "127.0.0.1", "255.255.255.0"}, false), 0);
    std::string out = testing::internal::GetCapturedStdout();
    std::string::difference_type numIPsMapped = std::count(out.begin(), out.end(), '\n')/2;
    EXPECT_EQ(256, numIPsMapped);
}

TEST(MainTest, Tracing) {
    
    EXPECT_EQ(callRunProgram({MainLogicTest::progName,"-t", "8.8.8.8"}, true), 0);
    EXPECT_EQ(callRunProgram({MainLogicTest::progName,"-t", "8.8.8.8", "1.1.1.1", "127.0.0.1"}, false), 0);
}
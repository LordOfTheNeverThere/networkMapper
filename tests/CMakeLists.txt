add_executable(networkMapperTests tests.cpp
        IPv4Range.cpp
)

target_link_libraries(networkMapperTests
        PRIVATE
        networkMapperLib
        socks
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(networkMapperTests)
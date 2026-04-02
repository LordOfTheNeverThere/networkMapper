#ifndef NETWORKMAPPER_MAINLOGIC_H
#define NETWORKMAPPER_MAINLOGIC_H
#include <iostream>
#include <vector>
#include <string>

void printHelp(const std::string& progName) {
    std::cout << "Usage: " << progName << " [OPTION] [ARGUMENTS...]\n\n"
              << "Options:\n"
              << "  -m, --map <IP> <NET>    Map an IPv4 address to a network address.\n"
              << "  -t, --trace <IPs...>    Perform a traceroute on a list of IP addresses.\n"
              << "  -h, --help              Display this help and exit.\n";
}

int mainLogic(int argc, char *argv[], bool testing = false) {
    const std::string progName {argv[0]};

    if (argc == 1) {
        // We need more than the program's name to work with
        printHelp(progName);
        return 1;
    }
    const std::string flag {argv[1]};
    if (flag == "-h" || flag == "--help") {
        printHelp(progName);
        return 0;
    } else if (flag == "-m" || flag == "--map") {
        if (argc != 4 ) {
            std::cerr << "Error: --map requires an IP and a Network address.\n";
            return 1;
        } else {
            std::string ip {argv[2]};
            std::string mask {argv[3]};
            if (!testing) {
                //TODO: Do Mapping
            } else {
                std::cout << "Mapping IP: " << ip << " to Network: " << mask << "\n";
            }
        }
    } else if (flag == "-t" || flag == "--trace") {
        if (argc < 3) {
            std::cerr << "Error: --trace requires at least one IP address.\n";
            return 1;
        } else {
            std::vector<std::string> ips {};
            ips.reserve(argc - 2);
            for (int argIndex = 2; argIndex < argc; ++argIndex) {
                ips.emplace_back(argv[argIndex]);
            }
            if (!testing) {
                //TODO: Do Tracing
            } else {
                std::cout << "Tracing " << ips.size() << " target(s)...\n";
            }
        }
    } else {
        std::cerr << "Unknown option: " << flag << "\n";
        printHelp(progName);
        return 1;
    }
    return 0;
}
#endif //NETWORKMAPPER_MAINLOGIC_H
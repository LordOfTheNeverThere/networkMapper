#ifndef NETWORKMAPPER_MAINLOGIC_H
#define NETWORKMAPPER_MAINLOGIC_H
#include <iostream>
#include <vector>
#include <string>

#include "IPv4Range.h"
#include "Mapper.h"
#include "Tracer.h"
#include "socks/Exceptions.h"
void printHelp(const std::string& progName) {
    std::cout << "Usage: " << progName << " [OPTION] [ARGUMENTS...]\n\n"
              << "Options:\n"
              << "  -m, --map <IP> <NET>    Map an IPv4 address to a network address.\n"
              << "  -t, --trace <IPs...>    Perform a traceroute on a list of IP addresses.\n"
              << "  -h, --help              Display this help and exit.\n";
}

void mappingLogic(const std::string& ip, const std::string& mask) {
    Mapper mapper {AF_INET};
    LocalHost myMachine {LocalHost(true)};
    IPv4Range range {ip, mask, myMachine};
    auto results = mapper.mapNetwork(range.getIPsNonLocal(), range.getIPsLocal(), myMachine);
    for (auto result: results) {
        std::cout << result;
    }
}

void tracingLogic(const std::vector<std::string>& ips, Int numOfHops = 64) {
    Tracer::multipleTraces(ips, numOfHops);
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
                try {
                    mappingLogic(ip, mask);
                } catch (IPv4OnlyException& ipv4err) {
                    std::cerr << "Only IPv4 is allowed\n";
                    return 1;
                } catch (std::runtime_error& runerr) {
                    std::cerr << runerr.what() << '\n';
                    return 1;
                }
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
                try {
                    tracingLogic(ips);
                } catch (std::runtime_error& runerr) {
                    std::cerr << runerr.what() << '\n';
                    return 1;
                }
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
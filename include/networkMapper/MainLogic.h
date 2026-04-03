#ifndef NETWORKMAPPER_MAINLOGIC_H
#define NETWORKMAPPER_MAINLOGIC_H
#include <iostream>
#include <vector>
#include <string>

#include "IPv4Range.h"
#include "Mapper.h"
#include "Tracer.h"
#include "socks/Exceptions.h"

class WrongMapperFormatException :public ConfigException {
public:
    WrongMapperFormatException()
    : ConfigException("Error: --map requires an IP and a Network address."){}
};

class WrongTracerFormatException : public ConfigException {
public:
    WrongTracerFormatException()
: ConfigException("Error: --trace requires at least one IP address."){}
};

class WrongTracerCountFormatException : public ConfigException {
public:
    WrongTracerCountFormatException()
: ConfigException("Error: After the --count/-c flag please specify an integer for the max hop count."){}
};


void printHelp(const std::string& progName) {
    std::cout << "Usage: " << progName << " { -m | -t | -h } [ARGUMENTS...]\n\n"
              << "Options:\n"
              << "  -m, --map <IP> <NET>              Map an IPv4 address to a network address.\n"
              << "  -t, --trace [-c, --count COUNT] <IP>... \n"
              << "                                    Perform a traceroute. COUNT sets max hops.\n"
              << "  -h, --help                        Display this help and exit.\n";
}

void mappingLogic(int argc, char *argv[], bool testing = false) {

    if (argc != 4 ) {
        throw WrongMapperFormatException();
    } else {
        std::string ip {argv[2]};
        std::string mask {argv[3]};
        if (!Tools::isValidIPv4(ip)) {
            throw WrongIPv4Format(ip);
        }
        if (!Tools::isValidIPv4(mask)) {
            throw WrongIPv4Format(mask);
        }

        if (!testing) {
            Mapper mapper {AF_INET};
            LocalHost myMachine {LocalHost(true)};
            IPv4Range range {ip, mask, myMachine};
            auto results = mapper.mapNetwork(range.getIPsNonLocal(), range.getIPsLocal(), myMachine);
            for (auto result: results) {
                std::cout << result;
            }
        } else {
            std::cout << "Mapping IP: " << ip << " to Network: " << mask << "\n";
        }
    }
}



void tracingLogic(int argc, char *argv[], bool testing = false) {
    if (argc < 3) {
        throw WrongTracerFormatException();
    }
    uint8_t maxHops {DEFAULT_MAX_HOP};
    Int ipsIndex {2};
    std::string afterFirstFlag {argv[2]};
    if(afterFirstFlag == "-c" || afterFirstFlag == "--count") {
        if (argc < 4 ) {
            throw WrongTracerCountFormatException();
        } else {
            try {
                std::string ttl {argv[3]};
                if (ttl.find('.') != ttl.npos || ttl.find(',') != ttl.npos) {
                    throw InvalidTTLException(ttl);
                }
                Int intermediary = std::stoi(ttl);
                // Bound Checking
                if (intermediary < std::numeric_limits<uint8_t>::min() || intermediary > std::numeric_limits<uint8_t>::max()) {
                    throw InvalidTTLException();
                } else {
                    maxHops = intermediary;
                }
            } catch (std::invalid_argument& inArgErr) {
                std::string ttl {argv[3]};
                throw InvalidTTLException(ttl);
            }
        }
        ipsIndex = ipsIndex + 2; // we increment the index where IPs start id we have the count flag and number
        if (argc < 5) {
            throw WrongTracerFormatException();
        }
    }
    std::vector<std::string> ips {};
    ips.reserve(argc - ipsIndex);
    for (; ipsIndex < argc; ++ipsIndex) {
        std::string ip {argv[ipsIndex]};
        if (!Tools::isValidIPv4(ip)) {
            throw WrongIPv4Format(ip);
        }
        ips.emplace_back(ip);
    }
    if (!testing) {
        Tracer::multipleTraces(ips, maxHops);
    } else {
        std::cout << "Tracing " << ips.size() << " target(s)...\n";
    }
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
        try {
            mappingLogic(argc, argv, testing);
        } catch (std::runtime_error& runerr) {
            std::cerr << runerr.what() << '\n';
            return 1;
        }

    } else if (flag == "-t" || flag == "--trace") {
        try {
            tracingLogic(argc, argv, testing);
        } catch (std::runtime_error& runerr) {
            std::cerr << runerr.what() << '\n';
            return 1;
        }
        return 0;
    } else {
        std::cerr << "Unknown option: " << flag << "\n";
        printHelp(progName);
        return 1;
    }
    return 0;
}
#endif //NETWORKMAPPER_MAINLOGIC_H
#ifndef NETWORKMAPPER_TRACEROUTEHOP_H
#define NETWORKMAPPER_TRACEROUTEHOP_H
#include <cstdint>
#include <iomanip>
#include <string>


class TraceRouteHop {

private:
    std::string m_ipAddress {"*"};
    std::uint64_t m_timelapse {0};

public:
    [[nodiscard]] std::string getIPAddress() const {
        return m_ipAddress;
    }

    void setIPAddress(const std::string &ip) {
        m_ipAddress = ip;
    }

    [[nodiscard]] uint64_t getTimelapse() const {
        return m_timelapse;
    }

    [[nodiscard]] double getTimelapseInMillis() const {
        return static_cast<double>(m_timelapse)/1000000;
    }

    void setTimelapse(const uint64_t timelapse) {
        m_timelapse = timelapse;
    }

};

inline std::ostream& operator<<(std::ostream& os, const TraceRouteHop& hop) {
    os << std::setw(15) << hop.getIPAddress();

    if (hop.getTimelapse() == 0) {
        os << " Unknown timelapse!";
    } else {
        os << " (" << hop.getTimelapseInMillis() << " ms)";
    }

    return os;
}

#endif //NETWORKMAPPER_TRACEROUTEHOP_H
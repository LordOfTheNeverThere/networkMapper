//
// Created by miguel on 2/28/26.
//

#ifndef NETWORKMAPPER_HOST_H
#define NETWORKMAPPER_HOST_H
#include <memory>
#include <string>
#include <vector>


class Host {

private:
    std::string m_name {};
    std::string m_ipAddress {};
    std::string m_networkMask {};
    std::string m_defaultGateway {};
    std::vector<std::weak_ptr<Host>> m_connections {};
public:
    Host(const std::string& name, const std::string& ipAddress, const std::string& networkMask, const std::string& defaultGateway)
    :
    m_name {name}, m_ipAddress {ipAddress},
    m_networkMask {networkMask}, m_defaultGateway {defaultGateway}
    {}

};


#endif //NETWORKMAPPER_HOST_H
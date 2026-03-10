//
// Created by miguel on 3/2/26.
//

#ifndef NETWORKMAPPER_CENTRALHOST_H
#define NETWORKMAPPER_CENTRALHOST_H
#include "networkMapper/Host.h"


class CentralHost : public Host{



    public:
    void createNetworkMap();
    void getHostInformation();
};


#endif //NETWORKMAPPER_CENTRALHOST_H
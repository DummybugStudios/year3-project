//
// Created by sid on 23/04/2021.
//

#ifndef RSU_APPLICATION_HELPER_H
#define RSU_APPLICATION_HELPER_H

#include "ns3/application-container.h"
#include "ns3/ptr.h"
#include "ns3/node-container.h"
#include "ns3/object-factory.h"

using namespace ns3;
class RSUApplicationHelper {
public:
    RSUApplicationHelper();
    ApplicationContainer Install(Ptr<Node> node) const;
    ApplicationContainer Install(NodeContainer c) const;
private:
    Ptr<Application> InstallPriv(Ptr<Node> node) const;
    ObjectFactory m_factory;
};


#endif //

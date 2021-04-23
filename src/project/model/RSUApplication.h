//
// Created by sid on 23/04/2021.
//

#ifndef RSU_APPLICATION_H
#define RSU_APPLICATION_H


#include "ns3/application.h"
#include "ns3/socket.h"
#include "ns3/ipv4-address.h"
#include <map>

using namespace ns3;
class RSUApplication : public Application {
public:
    static TypeId GetTypeId();
    RSUApplication() = default;
private:
    void StartApplication(void) override;
    void StopApplication(void) override;
    void ReceiveReputationPacket(Ptr<Socket> socket);
    static std::map<Ipv4Address, double> m_reputations;
    Ptr<Socket> m_reputationSocket;
    constexpr static int m_reputationPort = 1081;
};


#endif //

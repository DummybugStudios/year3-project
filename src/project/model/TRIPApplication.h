//
// Created by sid on 17/04/2021.
//

#ifndef NS_3_32_TRIPAPPLICATION_H
#define NS_3_32_TRIPAPPLICATION_H
#include "ns3/application.h"
#include "ns3/core-module.h"
#include "ns3/socket.h"


using namespace ns3;

class TRIPApplication : public Application{
public:
    static TypeId GetTypeId();
    TRIPApplication();
    ~TRIPApplication();

private:
    virtual void StartApplication();
    virtual void StopApplication();
    void PollForEvents();
    void ReceiveEventPacket(Ptr<Socket> socket);
    bool isEvil;
    int m_eventPort = 1080; ///< Port for communication about events
    Ptr<Socket> m_eventSocket;
    Ptr<UniformRandomVariable> m_unirv;
};


#endif //NS_3_32_TRIPAPPLICATION_H

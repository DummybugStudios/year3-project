/**
 * This class provides the vanets with the roadside events.
 * VANETs in similar positions are given similar numbers to communicate with each other
 * Bad VANETs will be given a wrong a different number to the other VANETs which is big sad
**/
 
#ifndef VANET_APPLICATION_H
#define VANET_APPLICATION_H

#include <iostream>
#include "ns3/application.h"
#include "ns3/header.h"
#include "ns3/socket.h"
#include "RoadEvents.h"


using namespace ns3;

class EventPacketHeader : public Header
{ 
    public:
    EventPacketHeader(){};
    void SetData(uint32_t x, uint32_t y, uint32_t val)
    {
        m_x = x;
        m_y = y;
        m_val = val;
    };

    uint32_t GetX(){return m_x;};
    uint32_t GetY(){return m_y;};
    uint32_t GetVal(){return m_val;};

    static TypeId GetTypeId();
    virtual TypeId GetInstanceTypeId() const{return GetTypeId();}; 
    virtual void Serialize(Buffer::Iterator start) const;
    virtual uint32_t Deserialize(Buffer::Iterator start); 
    virtual uint32_t GetSerializedSize() const;

    virtual void Print(std::ostream &stream) const
    {
        stream << "Header info: (" << m_x << ", "<< m_y << ") : " << m_val << std::endl;
    }

    private:
    uint32_t m_x,m_y,m_val;

};

class VanetApplication : public Application 
{
    public:
    static TypeId GetTypeId();
    VanetApplication();

    private:
    virtual void StartApplication();
    virtual void StopApplication();
    void StartLoop();
    std::vector<RoadEvent *> GetReachableEvents();
    void ReceiveEventPacket(Ptr<Socket> socket);


    bool isEvil;
    int m_port;
    Ptr<Socket> m_socket;
};

#endif
#ifndef AGGREGATE_APPLICATION_H
#define AGGREGATE_APPLICATION_H

#include "ns3/application.h"
#include "ns3/header.h"
#include "ns3/trailer.h"
#include "ns3/random-variable-stream.h"
#include "ns3/socket.h"
#include "project-bsm-application.h"
#include "RoadEvents.h"

using namespace ns3; 

class AggregateEventHeader : public Header
{
public:
    AggregateEventHeader(){};

    void SetData(uint32_t x, uint32_t y, uint32_t val, uint32_t signatureCount)
    {
        m_x = x;
        m_y = y;
        m_val = val;
        m_signatureCount = signatureCount;
    }

    const int GetX() const {
        return m_x;
    }

    const int GetY() const {
        return m_y;
    }

    const int GetVal() const {
        return m_val;
    }

    const int GetSignatureCount() const {
        return m_signatureCount;
    }

    void IncrementSignatureCount()
    {
        m_signatureCount++;
    }

    static TypeId GetTypeId();
    virtual TypeId GetInstanceTypeId() const{return GetTypeId();}
    virtual void Serialize(Buffer::Iterator start) const;
    virtual uint32_t Deserialize(Buffer::Iterator start);
    virtual uint32_t GetSerializedSize() const;
    virtual void Print (std::ostream &os) const
    {
        os << "x: " << m_x << " y: " << m_y << " val: " << m_val << " signatureCount: " << m_signatureCount << std::endl;
    }

private:
    uint32_t m_x, m_y, m_val, m_signatureCount;

};

class AggregateSignatureTrailer : public Trailer
{
public:
    static TypeId GetTypeId();
    virtual void Serialize(Buffer::Iterator start) const;
    virtual uint32_t Deserialize(Buffer::Iterator start);

    virtual uint32_t GetSerializedSize() const {return sizeof(uint32_t);}
    virtual TypeId GetInstanceTypeId() const {return GetTypeId();}
    virtual void Print(std::ostream &os) const
    {
        os << "Trailer contains signature from node: " << m_nodeId << std::endl;
    }

    void SetSignature(uint32_t nodeId)
    {
        m_nodeId = nodeId;
    }

    uint32_t GetSignature()
    {
        return m_nodeId;
    }
    // Cryptography is not implemented therefore appending nodeId is the form of signature
private:
    uint32_t m_nodeId;
};

class AggregateApplication : public Application
{
    public:
    static TypeId GetTypeId();
    AggregateApplication();


    private:
    virtual void StartApplication();
    virtual void StopApplication();
    void PollForEvents();
    void ReceiveEventPacket(Ptr<Socket> socket);
    bool SendToNearbyNodes(Ptr<Packet> p);
    bool SendToOtherGroups(Ptr<Packet> p);
    bool isEvil;
    Ptr<UniformRandomVariable> m_unirv;
    int m_eventPort = 1080;    ///< Port for event communication
    Ptr<Socket> m_eventSocket;
    Ptr<ProjectBsmApplication> m_bsmApplication;
    std::vector<RoadEvent *> m_recentEvents;
};

#endif
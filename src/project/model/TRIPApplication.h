//
// Created by sid on 17/04/2021.
//

#ifndef NS_3_32_TRIPAPPLICATION_H
#define NS_3_32_TRIPAPPLICATION_H
#include "ns3/application.h"
#include "ns3/core-module.h"
#include "ns3/socket.h"
#include "ns3/header.h"
#include "ns3/ipv4-address.h"


using namespace ns3;

class ReputationHeader: public Header
{
public:
    static TypeId GetTypeId();
    virtual TypeId GetInstanceTypeId() const {return GetTypeId();}
    ReputationHeader(bool isRequest = true, bool isRSU = false) :
    m_reputationValue(0),
    m_address(0),
    m_isRequest(isRequest),
    m_isRSU(isRSU)
    {
    }
    void SetData(uint32_t address, double reputationValue)
    {
        m_address = address;
        m_reputationValue = reputationValue;
    }
    bool IsRequest() {return m_isRequest;}
    bool IsRSU() {return m_isRSU;}

    virtual void Print(std::ostream &os) const
    {
        os << (m_isRequest ? "REQUEST: " : "RESPONSE: ") <<
        Ipv4Address(m_address) << " " <<
        m_reputationValue << std::endl;
    }

    double GetReputationValue(){return m_reputationValue;}
    uint32_t GetAddress(){return m_address;}


    virtual uint32_t GetSerializedSize() const;
    virtual uint32_t Deserialize(Buffer::Iterator start);
    virtual void Serialize(Buffer::Iterator start) const;

private:
    double m_reputationValue;
    uint32_t m_address;
    // TODO: since they're just bools you can merge them into one flag
    bool m_isRequest;
    bool m_isRSU;
};

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
    void ReceiveReputationPacket(Ptr<Socket> socket);
    bool isEvil;
    int m_eventPort = 1080; ///< Port for communication about events
    int m_reputationPort = 1081; ///< Port for communication about reputation
    Ptr<Socket> m_eventSocket;
    Ptr<Socket> m_reputationSocket;
    Ptr<UniformRandomVariable> m_unirv;
    // These three must add up to 1
    constexpr static double m_directTrustWeight = 0.3; ///< Weighting of a vehicle's own experience
    constexpr static double m_recTrustWeight = 0.4; ///< Weighting of other vehicle's recommendations
    constexpr static double m_rsuWeight = 0.3;     ///< Weighting of other vehicle's recommendations
};


#endif //NS_3_32_TRIPAPPLICATION_H

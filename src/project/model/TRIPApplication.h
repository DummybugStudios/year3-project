//
// Created by sid on 17/04/2021.
//

#ifndef NS_3_32_TRIPAPPLICATION_H
#define NS_3_32_TRIPAPPLICATION_H
#include "RoadEvents.h"
#include "ns3/application.h"
#include "ns3/core-module.h"
#include "ns3/socket.h"
#include "ns3/header.h"
#include "ns3/ipv4-address.h"
#include "ns3/simulator.h"
#include <vector>
#include <map>
#include <memory>


using namespace ns3;

class ReputationCountHeader : public Header {
public:
    static TypeId GetTypeId();

    ReputationCountHeader():m_count(0){};

    void SetData(uint32_t count){
        m_count = count;
    };

    uint32_t GetCount() {return m_count;}
    uint32_t GetSerializedSize(void) const override;

    void Serialize(Buffer::Iterator start) const override;

    uint32_t Deserialize(Buffer::Iterator start) override;

    void Print(ostream &os) const override;

    TypeId GetInstanceTypeId(void) const override;

private:
    uint32_t m_count;

};

class ReputationHeader: public Header
{
public:
    static TypeId GetTypeId();
    virtual TypeId GetInstanceTypeId() const {return GetTypeId();}
    ReputationHeader() :
    m_reputationValue(0),
    m_address(0),
    m_isRequest(false),
    m_isRSU(false)
    {}

    void SetData(uint32_t address, bool isRequest, double reputationValue=0.0f, bool isRSU = false)
    {
        m_address = address;
        m_reputationValue = reputationValue;
        m_isRequest = isRequest;
        m_isRSU = isRSU;
    }
    bool IsRequest() {return m_isRequest;}
    bool IsRSU() {return m_isRSU;}

    virtual void Print(std::ostream &os) const
    {
        os << (m_isRequest ? "REQUEST: " : "RESPONSE: ") <<
        Ipv4Address(m_address) << "\t" <<
        m_reputationValue;
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

enum TrustLevel
{
    NO_TRUST,
    SOME_TRUST,
    FULL_TRUST,
};

struct PeerScores
{
    Ipv4Address referrerAddress;
    double reputation;
};

//TODO: think of a more descriptive name?
struct Scores
{
    std::shared_ptr<std::vector<PeerScores>> peerScores;
    double rsuScore;
    bool didRsuReply;
};

struct EventNotification
{
    RoadEvent event;
    Ipv4Address referrer;
};

struct UnverifiedEventEntry
{
    EventNotification notification;
    std::shared_ptr<std::vector<PeerScores>> peerScores;
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
    void HandleEventVerification(const UnverifiedEventEntry &event, bool isTrue);
    TrustLevel DetermineTrustLevel(const Ipv4Address &address);
    void SendReputationsToRSU();
    void HandleRSUConfirmations(Ptr<Socket> p);
    static double GetNormalDistribution (double x, double mean, double sd);
    bool isEvil;
    int m_eventPort = 1080; ///< Port for communication about events
    int m_reputationPort = 1081; ///< Port for communication about reputation
    Ptr<Socket> m_eventSocket;
    Ptr<Socket> m_reputationSocket;
    Ptr<Socket> m_rsuNotifySocket;
    Ptr<UniformRandomVariable> m_unirv;
    // These three must add up to 1
    constexpr static double m_directTrustWeight = 0.3; ///< Weighting of a vehicle's own experience
    constexpr static double m_recTrustWeight = 0.4; ///< Weighting of other vehicle's recommendations
    constexpr static double m_rsuWeight = 0.3;     ///< Weighting of other vehicle's recommendations

    // All the means for the three fuzzy sets
    constexpr static double m_notTrustMean  = 0;
    constexpr static double m_someTrustMean = 0.5;
    constexpr static double m_fullTrustMean = 1.0f;

    // Standard Deviation for the three fuzzy sets
    constexpr static double m_notTrustSd  = 0.2;
    constexpr static double m_someTrustSd = 0.2;
    constexpr static double m_fullTrustSd = 0.2;

    std::map<Ipv4Address, Scores> m_carsBeingEvaluated; ///<Cars which are being evaluated
    std::map<Ipv4Address, std::vector<Ptr<Packet>>> m_packets;
    std::map<Ipv4Address, double> m_reputations;
    std::map<Ipv4Address, double> m_weights;
    std::vector<EventNotification> m_alreadySeenEvents;
    std::vector<UnverifiedEventEntry> m_unverifiedEvents;
    std::map<Ipv4Address, double> m_reputationsNotNotified;
};


#endif //NS_3_32_TRIPAPPLICATION_H

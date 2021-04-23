//
// Created by sid on 17/04/2021.
//

#include "TRIPApplication.h"
#include "RoadEvents.h"
#include "VanetApplication.h"
#include "ns3/mobility-model.h"
#include "ns3/global-value.h"
#include <iostream>
#include <cmath>

using namespace ns3;
TypeId ReputationHeader::GetTypeId()
{
    static TypeId tid = TypeId("ReputationHeader")
    .SetParent<Header>()
    .AddConstructor<ReputationHeader>();

    return tid;
}

uint32_t ReputationHeader::GetSerializedSize() const {
    return sizeof(m_address)+sizeof(m_reputationValue)+sizeof(m_isRequest)+sizeof(m_isRSU);
}

uint32_t ReputationHeader::Deserialize(Buffer::Iterator start) {
    // need to reinterpret uint64_t as double before it can be used
    uint64_t x = start.ReadNtohU64();
    double *y = (double *)(&x);
    m_reputationValue = *y;

    m_address = start.ReadNtohU32();
    m_isRequest = start.ReadU8();
    m_isRSU = start.ReadU8();
    return GetSerializedSize();
}

void ReputationHeader::Serialize(Buffer::Iterator start) const {
    // Cannot directly write a double so have to reinterpret it as to uint64_t
    const uint64_t *x  = (uint64_t *)&m_reputationValue;
    start.WriteHtonU64(*x);
    start.WriteHtonU32(m_address);

    start.WriteU8(m_isRequest);
    start.WriteU8(m_isRSU);
}

TypeId TRIPApplication::GetTypeId() {
    static TypeId tid = TypeId("TRIPApplication")
    .SetParent<Application>()
    .AddAttribute(
            "Evil",
            "Are nodes evil?",
            BooleanValue(false),
            MakeBooleanAccessor(&TRIPApplication::isEvil),
            MakeBooleanChecker()
            )
    .AddConstructor<TRIPApplication>();

    return tid;
}

TRIPApplication::TRIPApplication() : isEvil(false){
    m_unirv = CreateObject<UniformRandomVariable>();
}

TRIPApplication::~TRIPApplication() {

}

void TRIPApplication::StartApplication() {
    // Bind the port used for event communication
    // Can this be done in the constructor?
    TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
    m_eventSocket = Socket::CreateSocket(GetNode(), tid);
    m_eventSocket->SetRecvCallback(MakeCallback(&TRIPApplication::ReceiveEventPacket, this));
    InetSocketAddress local = InetSocketAddress(Ipv4Address::GetAny(), m_eventPort);
    m_eventSocket->Bind(local);

    // Bind the reputation socket to 0.0.0.0:m_reputationPort
    m_reputationSocket = Socket::CreateSocket(GetNode(), tid);
    m_reputationSocket->SetRecvCallback(MakeCallback(&TRIPApplication::ReceiveReputationPacket, this));
    local = InetSocketAddress(Ipv4Address::GetAny(), m_reputationPort);
    m_reputationSocket->Bind(local);

    // Schedule with some random delay to avoid collisions
    Simulator::Schedule(
            MilliSeconds(1000 + m_unirv->GetInteger(0,10.0f)),
            &TRIPApplication::PollForEvents,
            this);
}

void TRIPApplication::StopApplication() {

}

void TRIPApplication::PollForEvents() {

    auto position = GetNode()->GetObject<MobilityModel>()->GetPosition();
    IntegerValue threshold;
    GlobalValue::GetValueByName("VRCthreshold", threshold);

    std::vector<RoadEvent *> events = RoadEventManger::getReachableEvents(
            (int)position.x,(int) position.y, threshold.Get());

    for (const auto &event : events)
    {
        // TODO: change the val in the header if the car is evil
        EventPacketHeader header;
        header.SetData(event->x, event->y, event->val);
        Ptr<Packet> p = Create<Packet>();
        p->AddHeader(header);

        // Create socket to send it
        //FIXME: use the socket that already exists
        TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
        Ptr<Socket> socket = Socket::CreateSocket(GetNode(), tid);
        socket->SetAllowBroadcast(true);

        InetSocketAddress remote = InetSocketAddress(Ipv4Address("255.255.255.255"), m_eventPort);
        socket->Connect(remote);
        socket->Send(p);
    }

    Simulator::Schedule(Seconds(1), &TRIPApplication::PollForEvents, this);
}

void TRIPApplication::ReceiveEventPacket(Ptr <Socket> socket){

    Address address;
    Ptr<Packet> recvPacket = socket->RecvFrom(address);
    Ipv4Address peerAddress = InetSocketAddress::ConvertFrom(address).GetIpv4();
    auto search = m_carsBeingEvaluated.find(peerAddress);

    if (search == m_carsBeingEvaluated.end())
    {
        m_carsBeingEvaluated[peerAddress] = Scores{.didRsuReply=false};
    }
    m_packets[peerAddress].push_back(recvPacket);

    // Request reputation from other vehicles
    ReputationHeader header;
    header.SetData(peerAddress.Get(), true);
    Ptr<Packet> p = Create<Packet>();
    p->AddHeader(header);

    InetSocketAddress remote = InetSocketAddress("255.255.255.255",m_reputationPort);
    m_reputationSocket->SetAllowBroadcast(true);
    m_reputationSocket->Connect(remote);
    m_reputationSocket->Send(p);
}

void TRIPApplication::ReceiveReputationPacket(Ptr <Socket> socket) {
    ReputationHeader header;
    Address address;
    Ptr<Packet> p  = socket->RecvFrom(address);
    InetSocketAddress remote = InetSocketAddress::ConvertFrom(address);
    Ipv4Address peerAddress =  remote.GetIpv4();

    p->RemoveHeader(header);
    std::cout << GetNode()->GetId()+1 << ": " << header << "\tFROM " << peerAddress << std::endl;

    if (header.IsRequest())
    {
        // repurpose the header into a response header and send it back
        header.SetData(header.GetAddress(), false, 0.5f);
        p->AddHeader(header);
        socket->SendTo(p,0,remote);
        return;
    }

    auto evalCar = m_carsBeingEvaluated.find(Ipv4Address(header.GetAddress()));
    if (evalCar == m_carsBeingEvaluated.end())
        return;

    Scores &scores = evalCar->second;

    if (header.IsRSU() && !scores.didRsuReply)
    {
        scores.rsuScore = header.GetReputationValue();
        scores.didRsuReply = true;
        return;
    }

    // Don't let a vehicle give its own reputation
    if (peerAddress.Get() == evalCar->first.Get())
        return;

    // ensure that the same vehicle is not giving a reputation again
    // which can happen when the same node sends multiple road events notification
    // and the reputation request gets sent out multiple times
    for (PeerScores const &x : scores.peerScores)
    {
        if (peerAddress == x.referrerAddress)
            return;
    }

    std::cout << GetNode()->GetId()+1 << ": storing reputation score\n";
    PeerScores score{.address=evalCar->first,
                          .referrerAddress = peerAddress,
                          .reputation = header.GetReputationValue()};

    scores.peerScores.push_back(score);

    for (PeerScores const &x : scores.peerScores)
    {
        std::cout << Simulator::Now().GetMilliSeconds() << " " << x.address << " " << x.reputation <<
                  " (" << scores.peerScores.size() << ")"
                  << std::endl;
    }
    std::cout << std::endl;

    if (scores.peerScores.size() >= 3)
    {
        DetermineTrustLevel(evalCar->first);
    }
}

/**
 * Update the reputation of a vehicle based on the scores given by the peers and the RSUs
 */
TrustLevel TRIPApplication::DetermineTrustLevel(const Ipv4Address &address) {
    Scores scores = m_carsBeingEvaluated[address];

    // Sum the peer scores where each have the same weight:
    // the sum of the weights has to add up to 1
    double weight = 1.0f/scores.peerScores.size();

    double peerScore = 0.0f;
    for (PeerScores const &x : scores.peerScores)
    {
        peerScore += weight * x.reputation;
    }

    double rsuScore = scores.didRsuReply ? scores.rsuScore :0.5;

    double reputationScore = m_directTrustWeight * 0.5 +
            m_recTrustWeight * peerScore +
            m_rsuWeight * rsuScore;

    std::cout << "Final reputation Score: " << reputationScore << std::endl;

    // Figure out the reputation score's membership in each set
    double memberNotTrust = GetNormalDistribution(reputationScore, m_notTrustMean, m_notTrustSd);
    double memberSomeTrust = GetNormalDistribution(reputationScore, m_someTrustMean, m_someTrustSd);
    double memberTrust = GetNormalDistribution(reputationScore, m_fullTrustMean, m_fullTrustSd);

    double total = memberNotTrust + memberSomeTrust + memberTrust;

    // figure out the probabilities of it being each set
    // p(SomeTrust) not needed since probabilities add up to 1
    double pNotTrust = memberNotTrust/ total;
    double pTrust = memberTrust/total;

    // collapse the probabilities to figure out which set it belongs to:
    double finalProbability = m_unirv->GetValue(0,1);
    std::cout << "final prob: " << finalProbability << std::endl;

    if (finalProbability <= pNotTrust)
    {
        // Reject event
        std::cout << "NOT TRUSTING\n";
        return NO_TRUST;
    }
    if (finalProbability >= (1 - pTrust))
    {
        // Accept Event and broadcast it further
        std::cout << "FULL TRUST\n";
        return FULL_TRUST;
    }
    std::cout << "SOME TRUST\n";
    return SOME_TRUST;
}

double TRIPApplication::GetNormalDistribution(double x, double mean, double sd) {
    return 1 / (sd * sqrt(2 * M_PI)) * pow(M_E, -0.5 * pow ((x - mean)/sd,2));
}

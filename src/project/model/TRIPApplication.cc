//
// Created by sid on 17/04/2021.
//

#include "TRIPApplication.h"
#include "RoadEvents.h"
#include "VanetApplication.h"
#include "EventLogger.h"
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
    m_eventSocket->SetAllowBroadcast(true);
    InetSocketAddress local = InetSocketAddress(Ipv4Address::GetAny(), m_eventPort);
    m_eventSocket->Bind(local);

    // Bind the reputation socket to 0.0.0.0:m_reputationPort
    m_reputationSocket = Socket::CreateSocket(GetNode(), tid);
    m_reputationSocket->SetRecvCallback(MakeCallback(&TRIPApplication::ReceiveReputationPacket, this));
    m_reputationSocket->SetAllowBroadcast(true);
    local = InetSocketAddress(Ipv4Address::GetAny(), m_reputationPort);
    m_reputationSocket->Bind(local);

    // Initialise the m_rsuNotifySocket
    m_rsuNotifySocket = Socket::CreateSocket(GetNode(), tid);
    m_rsuNotifySocket->SetRecvCallback(MakeCallback(&TRIPApplication::HandleRSUConfirmations, this));
    m_rsuNotifySocket->SetAllowBroadcast(true);

    // Schedule with some random delay to avoid collisions
    Simulator::Schedule(
            MilliSeconds(1000 + m_unirv->GetInteger(0,10.0f)),
            &TRIPApplication::PollForEvents,
            this);

    // Schedule sending to an RSU
    Simulator::Schedule(
            MilliSeconds((3000 + m_unirv->GetInteger(0, 100))),
            &TRIPApplication::SendReputationsToRSU,
            this);
}

void TRIPApplication::StopApplication() {

}

void TRIPApplication::PollForEvents() {

    std::vector<RoadEvent *> events = GetReachableEvents();

    // For each event verify them and/or
    for (const auto &event : events)
    {
        // Check to see if any events can be verified
        for (auto iter = m_unverifiedEvents.begin(); iter != m_unverifiedEvents.end();)
        {
            const auto &notificationEvent = iter->notification.event;
            if (notificationEvent.x == event->x && notificationEvent.y == event->y)
            {
                if (notificationEvent.val == event->val)
                    HandleEventVerification(*iter, true);
                else
                    HandleEventVerification(*iter, false);
                iter = m_unverifiedEvents.erase(iter);
                // Continue ensure the ++iterator is not called
                continue;
            }
            iter++;
        }
        // TODO: change the val in the header if the car is evil
        EventPacketHeader header;
        int evilValue = event->val;
        if (isEvil)
            evilValue = 909090;
        header.SetData(event->x, event->y, evilValue);
        Ptr<Packet> p = Create<Packet>();
        p->AddHeader(header);

        InetSocketAddress remote = InetSocketAddress(Ipv4Address("255.255.255.255"), m_eventPort);
        m_eventSocket->Connect(remote);
        m_eventSocket->Send(p);
    }

    Simulator::Schedule(Seconds(1), &TRIPApplication::PollForEvents, this);
}

void TRIPApplication::ReceiveEventPacket(Ptr <Socket> socket){

    Address address;
    Ptr<Packet> recvPacket = socket->RecvFrom(address);
    Ipv4Address peerAddress = InetSocketAddress::ConvertFrom(address).GetIpv4();

    EventPacketHeader eventHeader;
    recvPacket->PeekHeader(eventHeader);

    //Return if the same event sent by the same user has already been seen
    // to avoid loops
    //TODO: remove the casting and actually define it properly
    for (auto const &x : m_alreadySeenEvents)
    {
        if (x.referrer == peerAddress &&
        x.event.val == (int) eventHeader.GetVal() &&
        x.event.x == (int) eventHeader.GetX() &&
        x.event.y == (int) eventHeader.GetY())
        {
            return;
        }
    }

    // Notify event logger that you have recieved an event
    EventLogger::guess(GetNode()->GetId(), eventHeader.GetX(), eventHeader.GetY(), eventHeader.GetVal(), ARRIVED);
    // Add event and referrer to already seen events
    RoadEvent event(
        eventHeader.GetX(),
        eventHeader.GetY(),
        eventHeader.GetVal()
    );

    EventNotification eventNotification {
        .event = event,
        .referrer = peerAddress
    };

    m_alreadySeenEvents.push_back(eventNotification);

    // Make the event unverified
    UnverifiedEventEntry unverifiedEventEntry{
        .notification = eventNotification,
        .peerScores = nullptr
    };

    // Decide if a new peerscores list needs to be created or if the old one works
    auto search = m_carsBeingEvaluated.find(peerAddress);
    if (search == m_carsBeingEvaluated.end())
    {
        auto peerScores = std::make_shared<std::vector<PeerScores>>();
        m_carsBeingEvaluated[peerAddress] = Scores{
                .peerScores = peerScores,
                .didRsuReply=false};
        unverifiedEventEntry.peerScores = peerScores;
    }
    else
    {
        unverifiedEventEntry.peerScores = search->second.peerScores;
    }

    // Check if you can trivially accept or reject the event
    // TODO: this code is repeated and can be made a function
    std::vector<RoadEvent *> events = GetReachableEvents();
    for (const auto &x : events)
    {
        if (x->x == (int) eventHeader.GetX() && x->y == (int)eventHeader.GetY())
        {
            if (x->val == (int)eventHeader.GetVal())
            {
                EventLogger::guess(GetNode()->GetId(), x->x , x->y, x->val, ACCEPTED);
                HandleEventVerification(unverifiedEventEntry, true);
            }
            else
            {
                EventLogger::guess(GetNode()->GetId(), x->x , x->y, eventHeader.GetVal(), REJECTED);
                HandleEventVerification(unverifiedEventEntry, false);
            }
            return;
        }
    }

    // Did not push back earlier because otherwise we would have had to then immediately delete it
    // if it was trivially verified
    m_unverifiedEvents.push_back(unverifiedEventEntry);
    m_packets[peerAddress].push_back(recvPacket);

    // Request reputation from other vehicles
    ReputationHeader header;
    header.SetData(peerAddress.Get(), true);
    Ptr<Packet> p = Create<Packet>();
    p->AddHeader(header);

    InetSocketAddress remote = InetSocketAddress("255.255.255.255",m_reputationPort);
    m_reputationSocket->Connect(remote);
    m_reputationSocket->Send(p);
}

void TRIPApplication::ReceiveReputationPacket(Ptr <Socket> socket) {
    Address baseAddress;
    Ptr<Packet> p  = socket->RecvFrom(baseAddress);
    InetSocketAddress remote = InetSocketAddress::ConvertFrom(baseAddress);
    Ipv4Address peerAddress =  remote.GetIpv4();

    ReputationHeader header;
    p->RemoveHeader(header);
    std::cout << GetNode()->GetId()+1 << ": " << header << "\tFROM " << peerAddress << std::endl;

    if (header.IsRequest())
    {
        // repurpose the header into a response header and send it back
        //TODO: default reputation value
        double reputation = 0.5;
        auto search = m_reputations.find(Ipv4Address(header.GetAddress()));
        if (search != m_reputations.end())
            reputation = search->second;
        header.SetData(header.GetAddress(), false, reputation);
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
    for (PeerScores const &x : *scores.peerScores)
    {
        if (peerAddress == x.referrerAddress)
            return;
    }

    std::cout << GetNode()->GetId()+1 << ": storing reputation score\n";
    PeerScores score{.referrerAddress = peerAddress,
                     .reputation = header.GetReputationValue()};

    scores.peerScores->push_back(score);

    for (PeerScores const &x : *scores.peerScores)
    {
        std::cout << Simulator::Now().GetMilliSeconds() << " " << evalCar->first << " " << x.reputation <<
                  " (" << scores.peerScores->size() << ")"
                  << std::endl;
    }
    std::cout << std::endl;

    if (scores.peerScores->size() < 3)
        return;

    TrustLevel trustLevel = DetermineTrustLevel(evalCar->first);
    //TODO: clear the peerscores and store this whole thing in
    m_carsBeingEvaluated.erase(peerAddress);

    bool acceptEvent = true;
    Ipv4Address headerAddress = Ipv4Address(header.GetAddress());
    if (trustLevel == NO_TRUST)
    {
        acceptEvent = false;
    }
    else if (trustLevel == SOME_TRUST)
    {
        // Figure out if the event should be accepted or not
        double pForwardEvent = m_someTrustMean - m_notTrustMean - m_notTrustSd;
        double decider = m_unirv->GetValue(0, 1);
        // Accept the event
        if (pForwardEvent < decider)
            acceptEvent = false;
    }
    else
    {
        // Forward the packets when trust level is FULL_TRUST
        InetSocketAddress broadcast = InetSocketAddress("255.255.255.255", m_eventPort);
        // Loop over all the waiting packets and send them
        for (auto const &x : m_packets[headerAddress])
        {
            m_eventSocket->SendTo(x, 0, broadcast);
        }
    }
    // Go through all the packets belonging to that address and either accept or reject them
    for (const auto &x : m_packets[headerAddress])
    {
        EventPacketHeader eventPacketHeader;
        x->PeekHeader(eventPacketHeader);
        EventLogger::guess(GetNode()->GetId(), eventPacketHeader.GetX(), eventPacketHeader.GetY(), eventPacketHeader.GetVal(),
                           acceptEvent ? ACCEPTED : REJECTED);
    }
    m_packets[headerAddress].clear();
}

/**
 * Update the reputation of a vehicle based on the scores given by the peers and the RSUs
 */
TrustLevel TRIPApplication::DetermineTrustLevel(const Ipv4Address &address) {

    // TODO: default reputation value
    double selfReputation = 0.5;
    auto reputationSearch = m_reputations.find(address);
    if (reputationSearch != m_reputations.end() )
        selfReputation = reputationSearch->second;

    Scores scores = m_carsBeingEvaluated[address];
    // collect all the weights so you can use as ratios
    std::vector<double> weights;
    double totalWeights = 0.0;
    for (auto const &x : *scores.peerScores)
    {
        double weight = 0.5;
        auto search = m_weights.find(x.referrerAddress);
        if (search != m_weights.end())
            weight = search->second;
        weights.push_back(weight);
        totalWeights += weight;
    }

    double peerScore = 0.0f;
    for (long unsigned int i = 0; i < weights.size(); i++)
    {
        peerScore += (weights[i] / totalWeights) * scores.peerScores->at(i).reputation;
    }

    double rsuScore = scores.didRsuReply ? scores.rsuScore :0.5;

    double reputationScore = m_directTrustWeight * selfReputation +
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

void TRIPApplication::HandleEventVerification(const UnverifiedEventEntry &event, bool isTrue) {

    // Adjust reputation of vehicle
    // TODO: default reputation value
    auto thing = m_reputations.find(event.notification.referrer);
    double reputation = 0.5f;
    if (thing != m_reputations.end())
        reputation = thing->second;

    if (isTrue)
    {
        reputation += 0.1;
        reputation = reputation > 1.0 ? 1.0 : reputation;
    }
    else
    {
        reputation -= 0.1;
        reputation = reputation < 0.0 ? 0.0 : reputation;
    }

    m_reputations[event.notification.referrer] = reputation;
    m_reputationsNotNotified[event.notification.referrer] = reputation;

    // punish or reward weights of peers
    for (auto const &x : *event.peerScores)
    {
        // TODO: have a default reputation value instead of using 0.5 everywhere
        if (x.reputation == 0.5)
            continue;

        double weight = 0.5;
        auto search = m_weights.find(event.notification.referrer);
        if (search != m_weights.end())
            weight = search->second;

        if (isTrue)
        {
            if (x.reputation > 0.5)
            {
                weight += 0.1;
                weight = weight > 1.0 ? 1.0 : weight;
            }
            else
            {
                weight -= 0.1;
                weight = weight < 0.0 ? 0.0 : weight;
            }
        }
        else
        {
            if (x.reputation > 0.5)
            {
                weight -= 0.1;
                weight = weight < 0.0 ? 0.0 : weight;
            }
            else
            {
                weight += 0.1;
                weight = weight > 1.0 ? 1.0 : weight;
            }
        }

        m_weights[x.referrerAddress] = weight;
    }
}

/**
 * Every so often notify an RSU about your current set of reputations
 */
void TRIPApplication::SendReputationsToRSU() {
    Simulator::Schedule(MilliSeconds(3000 + m_unirv->GetInteger(0,100)), &TRIPApplication::SendReputationsToRSU, this);
    if (m_reputationsNotNotified.empty())
        return;

    Ptr<Packet> p = Create<Packet>();
    for (auto const &x : m_reputationsNotNotified)
    {
        ReputationHeader header;
        header.SetData(x.first.Get(), false, x.second);
        p->AddHeader(header);
    }
    ReputationCountHeader header;
    header.SetData(m_reputationsNotNotified.size());
    p->AddHeader(header);

    InetSocketAddress remote = InetSocketAddress("255.255.255.255", 1082);
    m_rsuNotifySocket->Connect(remote);
    m_rsuNotifySocket->Send(p);
}


void TRIPApplication::HandleRSUConfirmations(Ptr<Socket> p) {
    m_reputationsNotNotified.clear();
}

std::vector<RoadEvent *> TRIPApplication::GetReachableEvents()
{
    auto position = GetNode()->GetObject<MobilityModel>()->GetPosition();
    IntegerValue threshold;
    GlobalValue::GetValueByName("VRCthreshold", threshold);

    return RoadEventManger::getReachableEvents(
            (int)position.x,(int) position.y, threshold.Get());
}


/*
 * Implementation For ReputationCountHeader starts here
 */
TypeId ReputationCountHeader::GetTypeId() {
    static TypeId tid = TypeId("ReputationCountHeader")
            .SetParent<Header>()
            .AddConstructor<ReputationCountHeader>();
    return tid;
}

uint32_t ReputationCountHeader::GetSerializedSize(void) const {
    return sizeof(m_count);
}

void ReputationCountHeader::Serialize(Buffer::Iterator start) const {
    start.WriteU32(m_count);
}

uint32_t ReputationCountHeader::Deserialize(Buffer::Iterator start) {
    m_count = start.ReadU32();
    return GetSerializedSize();
}

void ReputationCountHeader::Print(ostream &os) const {
    os << "count = " << m_count;
}

TypeId ReputationCountHeader::GetInstanceTypeId(void) const {
    return GetTypeId();
}

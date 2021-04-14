#include <iostream>
#include <vector>
#include <string>  // DEBUG
#include "AggregateApplication.h"

#include "ns3/core-module.h"
#include "ns3/type-id.h"
#include "ns3/mobility-model.h"
#include "project-group.h"

using namespace ns3;

/*
 * Implmentation of packet Header
 */
TypeId AggregateEventHeader::GetTypeId() {
    static TypeId tid = TypeId("AggregateEventHeader")
    .SetParent<Header>()
    .AddConstructor<AggregateEventHeader>();

    return tid;
}

void AggregateEventHeader::Serialize(Buffer::Iterator start) const {
    start.WriteHtonU32(m_x);
    start.WriteHtonU32(m_y);
    start.WriteHtonU32(m_val);
    start.WriteHtonU32(m_signatureCount);
}

uint32_t AggregateEventHeader::Deserialize(Buffer::Iterator start) {
    m_x = start.ReadNtohU32();
    m_y = start.ReadNtohU32();
    m_val = start.ReadNtohU32();
    m_signatureCount = start.ReadNtohU32();

    return sizeof(uint32_t) * 4;
}

uint32_t AggregateEventHeader::GetSerializedSize() const {
    return sizeof(uint32_t) * 4;
}

/*
 * Implementation of Aggregate Trailer
 */

TypeId AggregateSignatureTrailer::GetTypeId() {
    static TypeId tid = TypeId("AggregateSignatureTrailer")
    .SetParent<Trailer>()
    .AddConstructor<AggregateSignatureTrailer>();

    return tid;
}

void AggregateSignatureTrailer::Serialize(Buffer::Iterator start) const {
    start.Prev(GetSerializedSize());
    start.WriteHtonU32(m_nodeId);
}

uint32_t AggregateSignatureTrailer::Deserialize(Buffer::Iterator start) {
    m_nodeId = start.ReadNtohU32();
    return sizeof(uint32_t);
}

/*
 * Implementation of Aggregate Application
 */

// Small helper function to an ipv4 address from a node
Ipv4Address getNodeAddress(Ptr<Node> node)
{
    Ptr<Ipv4> ipv4 = node->GetObject<Ipv4>();
    // TODO: interface will not always be 1
    Ipv4InterfaceAddress iaddr = ipv4->GetAddress(1,0);
    return iaddr.GetLocal();

}

TypeId AggregateApplication::GetTypeId()
{ 
    static TypeId tid = TypeId("AggregateApplication")
    .SetParent<Application>()
    .AddAttribute(
        "Evil",
        "Are nodes evil?",
        BooleanValue(false),
        MakeBooleanAccessor(&AggregateApplication::isEvil),
        MakeBooleanChecker()
    )
    .AddConstructor<AggregateApplication>();

    return tid; 
}

AggregateApplication::AggregateApplication():
isEvil(false)
{
    // Set up the uniform random variable
    m_unirv = CreateObject<UniformRandomVariable>();
}

void AggregateApplication::StartApplication()
{
    // Bind event socket to  0.0.0.0:m_eventPort for incoming event messages
    TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
    m_eventSocket = Socket::CreateSocket(GetNode(), tid);
    m_eventSocket->SetRecvCallback(MakeCallback(&AggregateApplication::ReceiveEventPacket, this));
    InetSocketAddress local = InetSocketAddress(Ipv4Address::GetAny(), m_eventPort);
    m_eventSocket->Bind(local);

    // Set up the reference to the BSM application
    //TODO: Use TypeId instead of using index 0 to future proof this code
    m_bsmApplication = DynamicCast<ProjectBsmApplication>(GetNode()->GetApplication(0));

    // Start polling with a bit of delay since if a group starts off near an event
    // Only one car detects the message so there's only one car releasing the message
    // Otherwise there will be conflict in the group about which car's message gets signed
    Simulator::Schedule(MilliSeconds(1000 + m_unirv->GetInteger(0,10.0f)), &AggregateApplication::PollForEvents, this);
}

void AggregateApplication::PollForEvents()
{

    auto position = GetNode()->GetObject<MobilityModel>()->GetPosition();
    std::vector<RoadEvent *> events =  RoadEventManger::getReachableEvents((int)position.x, (int)position.y, 100);

    if (!events.empty())
    {
        // Create a packet
        for (RoadEvent * const &event : events)
        {
            Ptr<Packet> p = Create<Packet>();
            AggregateEventHeader header;
            header.SetData(event->x, event->y, event->val, 1);
            p->AddHeader(header);

            // TODO: find out if this creates some sort of memory leak
            AggregateSignatureTrailer trailer;
            trailer.SetSignature(GetNode()->GetId());
            p->AddTrailer(trailer);

            std::cout << Simulator::Now().GetMilliSeconds()/1000.0f<<"s "<<GetNode()->GetId() <<": (" << event->x << ", " << event->y<< "):" << event->val << std::endl;

            bool sent = SendToNearbyNodes(p);
            if (sent)
                m_recentEvents.push_back(event);
        }
    }
    Simulator::Schedule(MilliSeconds(1000 + m_unirv->GetInteger(0,10.0f)), &AggregateApplication::PollForEvents, this);
}

void AggregateApplication::ReceiveEventPacket(Ptr<Socket> socket)
{
    auto position = GetNode()->GetObject<MobilityModel>()->GetPosition();
    Address address;
    Ptr<Packet> p = socket->RecvFrom(address);
    Ptr<Packet> copy = p->Copy();

    AggregateEventHeader header;
    p->RemoveHeader(header);

    // Check if data information is already in your memory:
    std::cout << Simulator::Now().GetMilliSeconds()/1000.0f<<"s "<<GetNode()->GetId() << ": ";
    std::cout << "(" << header.GetX() <<", " << header.GetY() <<") : "<<header.GetVal() << "(" << header.GetSignatureCount() << ") ";
              bool found = false;
    for (auto const &x : m_recentEvents)
    {
        if (x->x == header.GetX() && x->y == header.GetY())
        {
            found = true;
            break;
        }
    }

    if (found) {
        std::cout << "ALREADY STORED\n" ;
        return;
    }

    // Just return if event cannot be validated
    RoadEvent *event = nullptr;
    if (header.GetSignatureCount() < 4)
    {
        //TODO: turn the threshold into a configurable number
        vector<RoadEvent *> events = RoadEventManger::getReachableEvents((int)position.x,(int) position.y, 100);
        for (RoadEvent * const &x : events)
        {
            if (x->x == header.GetX() && x->y == header.GetY() && x->val == header.GetVal())
            {
                event = x;
                break;
            }
        }

        if (!event)
        {
            std::cout << "NOT VALIDATED\n";
            return;
        }
    }

    // If the event can be validated then
    // verify the signatures cryptographically but this has not been implemented
    // therefore we are just making sure we aren't signing a packet twice
    bool valid = true;
    AggregateSignatureTrailer trailer;
    for (int i =0; i < header.GetSignatureCount(); i++)
    {
        p->RemoveTrailer(trailer);
        // obviously fake validation
        if (trailer.GetSignature() == GetNode()->GetId())
        {
            valid = false;
            break;
        }

    }
    if (!valid)
    {
        std::cout << "ALREADY SIGNED \n";
        return;
    }

    // If the packet needs more validations then
    // Append your own signature and redistribute the packet
    std::cout << "ACCEPTED: ";
    if (header.GetSignatureCount() < 3)
    {
        AggregateEventHeader header;
        copy->RemoveHeader(header);
        header.IncrementSignatureCount();
        copy->AddHeader(header);

        AggregateSignatureTrailer trailer;
        trailer.SetSignature(GetNode()->GetId());
        copy->AddTrailer(trailer);
        std::cout << "ADDED OWN TRAILER ";
    }
    bool sent = SendToNearbyNodes(copy);
    if (sent)
    {
        std::cout << "SENT PACKET\n";
        m_recentEvents.push_back(event);
    }
    else
    {
        std::cout << "BUT NO CAR IN SAME GROUP FOUND\n";
    }
}

/**
 * Function to send a packet to nodes in the same group
 * @param p Packet to send
 * @return whether anything was sent or not
 */
bool AggregateApplication::SendToNearbyNodes(Ptr<Packet> p)
{
    // Create a socket that will send the packet
    bool sent = false;
    auto position = GetNode()->GetObject<MobilityModel>()->GetPosition();

    TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
    Ptr<Socket> socket = Socket::CreateSocket(GetNode(), tid);

    // Get a list of cars in the group from the BSM application
    int groupId = Group::GetGroup(position.x, position.y);

    // TODO: stop using the direct access to m_reachableNodes and find other methods
    std::string debugMessage;
    for (auto const &x : m_bsmApplication->m_reachableNodes)
    {
        debugMessage += std::to_string(GetNode()->GetId());
        debugMessage += std::string(": sending it to: ");
        if (x.second->groupId == groupId)
        {
            InetSocketAddress remote = InetSocketAddress(getNodeAddress(x.first), m_eventPort);
            socket->Connect(remote);
            socket->Send(p);
            if (!sent)
                sent = true;
            debugMessage += std::to_string(x.first->GetId());
            debugMessage += std::string(" ");
        }
    }
    if (sent)
    {
        std::cout << debugMessage << std::endl;
    }
    return sent;
}

bool AggregateApplication::SendToOtherGroups(Ptr <Packet> p) {
    bool sent = false;
    auto position = GetNode()->GetObject<MobilityModel>()->GetPosition();

    TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
    Ptr<Socket> socket = Socket::CreateSocket(GetNode(), tid);

    // Get a list of cars in the group from the BSM application
    int groupId = Group::GetGroup(position.x, position.y);

    // TODO: stop using the direct access to m_reachableNodes and find other methods
    for (auto const &x : m_bsmApplication->m_reachableNodes)
    {
        if (x.second->groupId != groupId && x.second->groupId != -1)
        {
            InetSocketAddress remote = InetSocketAddress(getNodeAddress(x.first), m_eventPort);
            socket->Connect(remote);
            socket->Send(p);
            if (!sent)
                sent = true;
        }
    }
    return sent;
}

void AggregateApplication::StopApplication()
{
}


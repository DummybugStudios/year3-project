#include <iostream>
#include "AggregateApplication.h"

#include "ns3/core-module.h"
#include "ns3/type-id.h"
#include "RoadEvents.h"
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
    RoadEvent *event = RoadEventManger::getNearestEvent((int)position.x, (int)position.y, 100);

    if (event != nullptr)
    {
        // Create a packet
        Ptr<Packet> p = Create<Packet>();
        AggregateEventHeader header;
        header.SetData(event->x, event->y, event->val, 1);
        p->AddHeader(header);

        AggregateSignatureTrailer trailer;
        trailer.SetSignature(GetNode()->GetId());
        p->AddTrailer(trailer);

        // Create a socket that will send the packet
        TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
        Ptr<Socket> socket = Socket::CreateSocket(GetNode(), tid);

        // Get a list of cars in the group from the BSM application
        int groupId = Group::GetGroup(position.x, position.y);

        // TODO: stop using the direct access to m_reachableNodes and find other methods
        for (auto const &x : m_bsmApplication->m_reachableNodes)
        {
            if (x.second->groupId == groupId)
            {
                InetSocketAddress remote = InetSocketAddress(getNodeAddress(x.first), m_eventPort);
                socket->Connect(remote);
                socket->Send(p);
            }
        }

    }
    Simulator::Schedule(MilliSeconds(1000 + m_unirv->GetInteger(0,10.0f)), &AggregateApplication::PollForEvents, this);
}

void AggregateApplication::ReceiveEventPacket(Ptr<Socket> socket)
{
    int self = GetNode()->GetId();
    int other = socket->GetNode()->GetId();
    std::cout<< self << " Received message from " << other << std::endl;
}

void AggregateApplication::StopApplication()
{
}
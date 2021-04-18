#include <iostream>
#include <string>

#include "ns3/core-module.h"
#include "ns3/type-id.h"
#include "ns3/mobility-model.h"
#include "ns3/global-value.h"

#include "ns3/simulator.h"

#include "VanetApplication.h"

#include "RoadEvents.h"
#include "EventLogger.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("VanetApplication");


/**
 * 
 *  Event Packet Header implementation starts here
 * 
*/

TypeId EventPacketHeader::GetTypeId()
{
    static TypeId tid = TypeId("EventPacketHeader")
    .SetParent<Header>()
    .AddConstructor<EventPacketHeader>();

    return tid; 
}


uint32_t EventPacketHeader::GetSerializedSize() const
{
    return sizeof(uint32_t)*3 ;
}


void EventPacketHeader::Serialize(Buffer::Iterator start) const
{
    start.WriteHtonU32(m_x);
    start.WriteHtonU32(m_y);
    start.WriteHtonU32(m_val);
}


uint32_t EventPacketHeader::Deserialize(Buffer::Iterator start)
{
    m_x = start.ReadNtohU32();
    m_y = start.ReadNtohU32();
    m_val = start.ReadNtohU32();

    return sizeof(uint32_t) * 3;
}


/**
 * 
 *  Vanet Application implementation starts here
 * 
*/


TypeId VanetApplication::GetTypeId()
{
    static TypeId tid = TypeId("VanetApplication")
    .SetParent<Application>()
    .AddAttribute(
        "Evil",
        "Are nodes evil?",
        BooleanValue(false),
        MakeBooleanAccessor(&VanetApplication::isEvil),
        MakeBooleanChecker()
    )
    .AddConstructor<VanetApplication>();

    return tid;
}

VanetApplication::VanetApplication():
isEvil(false),
// Random port for event communication
m_port(1080),
m_socket(nullptr)
{
}


void VanetApplication::StartApplication()
{
    StartLoop();
}


void VanetApplication::ReceiveEventPacket(Ptr<Socket> socket)
{
    Ptr<Packet> p = socket->Recv();
    EventPacketHeader header;
    p->RemoveHeader(header);

    std::cout << "(" << header.GetX() <<", " << header.GetY() <<") : "<<header.GetVal()<<std::endl;

    EventLogger::guess(header.GetX(), header.GetY(), header.GetVal(), true);
}


void VanetApplication::StartLoop()
{
    Simulator::Schedule (Seconds(1),&VanetApplication::StartLoop, this);

    auto position  = GetNode()->GetObject<MobilityModel>()->GetPosition();

    IntegerValue threshold;
    GlobalValue::GetValueByName("VRCthreshold", threshold);
    RoadEvent *event = RoadEventManger::getNearestEvent((int)position.x, (int)position.y, threshold.Get());
    if (event != nullptr)
    {
        if (!m_socket)
        {
            TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
            m_socket = Socket::CreateSocket(GetNode(), tid);
            m_socket->SetRecvCallback(MakeCallback(&VanetApplication::ReceiveEventPacket, this));

            InetSocketAddress local = InetSocketAddress(Ipv4Address::GetAny(), m_port);


            Ptr<NetDevice> device = GetNode()->GetDevice(0);
            m_socket->BindToNetDevice(device);
            m_socket->Bind(local);
            m_socket->SetAllowBroadcast(true);

            InetSocketAddress remote = InetSocketAddress(Ipv4Address("255.255.255.255"), m_port);
            m_socket->Connect(remote);
        }

        Ptr<Packet> p = Create<Packet>();

        // Evil packets change the values
        int val = isEvil ? 909090: event->val;

        EventPacketHeader header;
        header.SetData(event->x, event->y, val);
        p->AddHeader(header); 
        m_socket->Send(p);
    }
}



void VanetApplication::StopApplication()
{
}
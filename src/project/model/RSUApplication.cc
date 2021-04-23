//
// Created by sid on 23/04/2021.
//

#include "RSUApplication.h"

#include "TRIPApplication.h"

using namespace ns3;

std::map<Ipv4Address, double> RSUApplication::m_reputations{};

TypeId RSUApplication::GetTypeId() {
    static TypeId tid = TypeId("RSUApplication")
            .SetParent<Application>()
            .AddConstructor<RSUApplication>();

    return tid;
}

void RSUApplication::StartApplication(void) {
    TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
    m_reputationSocket = Socket::CreateSocket(GetNode(), tid);
    InetSocketAddress local = InetSocketAddress(Ipv4Address::GetAny(), m_reputationPort);
    m_reputationSocket->Bind(local);
    m_reputationSocket->SetRecvCallback(MakeCallback(&RSUApplication::ReceiveReputationPacket, this));
}

void RSUApplication::StopApplication(void) {
}

void RSUApplication::ReceiveReputationPacket(Ptr<Socket> socket) {
    ReputationHeader header;
    Address address;
    Ptr<Packet> p = socket->RecvFrom(address);
    InetSocketAddress remote = InetSocketAddress::ConvertFrom(address);
    Ipv4Address ipv4Address = remote.GetIpv4();
    p->RemoveHeader(header);

    if (!header.IsRequest())
        return;

    auto search = m_reputations.find(ipv4Address);
    double reputation = 0.0f;
    if (search != m_reputations.end())
        reputation = search->second;

    header.SetData(header.GetAddress(), false, reputation, true);
    p->AddHeader(header);

    socket->Connect(remote);
    socket->Send(p);
}


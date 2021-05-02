//
// Created by sid on 23/04/2021.
//

#include "RSUApplication.h"

#include "TRIPApplication.h"

using namespace ns3;

std::map<Ipv4Address, std::map<Ipv4Address, double>> RSUApplication::m_reputations{};

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

    m_notificationSocket = Socket::CreateSocket(GetNode(), tid);
    local = InetSocketAddress(Ipv4Address::GetAny(), 1082);
    m_notificationSocket->Bind(local);
    m_notificationSocket->SetRecvCallback(MakeCallback(&RSUApplication::ReceiveNotifyPacket, this));
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
    {
        // Calculate an average of the scores
        auto& mapOfReputations = search->second;
        for (auto const &x : mapOfReputations)
        {
            reputation += x.second;
        }
        reputation /= mapOfReputations.size();
    }
    else {
         reputation = 0.5f;
    }

    header.SetData(header.GetAddress(), false, reputation, true);
    p->AddHeader(header);

    socket->Connect(remote);
    socket->Send(p);
}

void RSUApplication::ReceiveNotifyPacket(Ptr<Socket> socket) {
    Address address;
    Ptr<Packet> recv = socket->RecvFrom(address);
    // Empty packet to act as acknowledgement
    Ptr<Packet> p = Create<Packet>();

    InetSocketAddress remote = InetSocketAddress::ConvertFrom(address);
    Ipv4Address ipv4 = remote.GetIpv4();

    ReputationCountHeader countHeader;
    recv->RemoveHeader(countHeader);

    for (unsigned int i = 0; i < countHeader.GetCount(); i++)
    {
        ReputationHeader header;
        recv->RemoveHeader(header);
        auto targetAddr = Ipv4Address(header.GetAddress());
        m_reputations[targetAddr][ipv4] = header.GetReputationValue();
    }

    m_notificationSocket->SendTo(p,0,remote);
}


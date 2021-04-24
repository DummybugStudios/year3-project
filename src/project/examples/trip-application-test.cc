//
// An example to test the basic functionality of the TRIP application
//


#include "ns3/VanetApplication.h"

#include "ns3/core-module.h"

#include <ns3/node-container.h>
#include <ns3/net-device-container.h>
#include <ns3/csma-helper.h>
#include <ns3/internet-stack-helper.h>
#include <ns3/ipv4-address-helper.h>

#include <ns3/VanetApplicationHeper.h>
#include <ns3/simulator.h>
#include <ns3/mobility-helper.h>
#include <ns3/global-value.h>

#include <ns3/TRIPApplication.h>
#include <iostream>

using namespace ns3;

void receiveReputationPacket(Ptr<Socket>);

void bindSocketToReceiveReputation(Ptr<Socket> &socket)
{
    InetSocketAddress local = InetSocketAddress(Ipv4Address::GetAny(), 1081);
    socket->SetRecvCallback(MakeCallback(&receiveReputationPacket));
    socket->Bind(local);
}

void sendEventPacket(Ptr<Socket> &socket, Ipv4InterfaceContainer &interfaces)
{
    // Send two event headers
    EventPacketHeader header;
    header.SetData(10,20,9090);
    Ptr<Packet> p = Create<Packet>();
    p->AddHeader(header);
    InetSocketAddress remote = InetSocketAddress(interfaces.GetAddress(0), 1080);
    socket->Connect(remote);
    socket->Send(p);
}

void receiveReputationPacket(Ptr<Socket> socket)
{
    std::cout << "Reputation Packet received\n";
}

void sendReputationFor(Ipv4Address address, Ipv4InterfaceContainer &interfaces, Ptr<Socket> &socket)
{
    ReputationHeader header;
    header.SetData(address.Get(), false, 0.3f);

    Ptr<Packet> p = Create<Packet>();
    p->AddHeader(header);

    InetSocketAddress remote = InetSocketAddress(interfaces.GetAddress(0), 1081);
    socket->Connect(remote);
    socket->Send(p);
}

int main( int argc, char **argv)
{
    static GlobalValue g_threshold("VRCthreshold",
                                   "bing bong",
                                   ns3::IntegerValue(50),
                                   ns3::MakeIntegerChecker<int>());
    NodeContainer nodes;
    nodes.Create(3);

    CsmaHelper csmaHelper;
    NetDeviceContainer devices;
    devices = csmaHelper.Install(nodes);

    InternetStackHelper stack;
    stack.Install(nodes);

    Ipv4AddressHelper address;
    address.SetBase("10.1.0.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces;
    interfaces = address.Assign(devices);

    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                   "MinX", DoubleValue (0.0),
                                   "MinY", DoubleValue (0.0),
                                   "DeltaX", DoubleValue (5.0),
                                   "DeltaY", DoubleValue (10.0),
                                   "GridWidth", UintegerValue (3),
                                   "LayoutType", StringValue ("RowFirst"));

    mobility.Install(nodes);
    // Use the TRIP application algo
    VanetApplicationHelper appHelper(false, 2);
    ApplicationContainer apps = appHelper.Install(nodes.Get(0));
    apps.Start(Seconds(0));
    apps.Stop(Seconds(30));

    // Socket  1
    TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
    Ptr<Socket> socket1 = Socket::CreateSocket(nodes.Get(1), tid);
    Ptr<Socket> socket2 = Socket::CreateSocket(nodes.Get(2), tid);
    bindSocketToReceiveReputation(socket1);
    bindSocketToReceiveReputation(socket2);

    // Make two nodes each send an event
    // nodes[1] and then nodes[2]
    Simulator::Schedule(Seconds(2), &sendEventPacket,socket1, interfaces);
    Simulator::Schedule(Seconds(3), &sendEventPacket,socket2, interfaces);

    // Make socket 1 send the same event again
    // The thingy should not accept it
    Simulator::Schedule(Seconds(3.1), &sendEventPacket,socket1, interfaces);

    // Send responses in a different order
    // nodes[2] and then nodes[1]
    // It should print them in reverese order too
    Simulator::Schedule(Seconds(4), &sendReputationFor, interfaces.GetAddress(1), interfaces, socket2);
    Simulator::Schedule(Seconds(5), &sendReputationFor, interfaces.GetAddress(2), interfaces, socket1);

    Simulator::Run();
    Simulator::Destroy();
    return 0;

}

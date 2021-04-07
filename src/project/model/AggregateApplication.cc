#include <iostream>
#include "AggregateApplication.h"

#include "ns3/core-module.h"
#include "ns3/type-id.h"

using namespace ns3;

/*
 * Implmentation of packet Header
 */
TypeId AggregateEventPacketHeader::GetTypeId() {
    static TypeId tid = TypeId("AggregateEventPacketHeader")
    .SetParent<Header>()
    .AddConstructor<AggregateEventPacketHeader>();

    return tid;
}

void AggregateEventPacketHeader::Serialize(Buffer::Iterator start) const {
    start.WriteHtonU32(m_x);
    start.WriteHtonU32(m_y);
    start.WriteHtonU32(m_val);
    start.WriteHtonU32(m_signatureCount);
}

uint32_t AggregateEventPacketHeader::Deserialize(Buffer::Iterator start) {
    m_x = start.ReadNtohU32();
    m_y = start.ReadNtohU32();
    m_val = start.ReadNtohU32();
    m_signatureCount = start.ReadNtohU32();

    return sizeof(uint32_t) * 4;
}

uint32_t AggregateEventPacketHeader::GetSerializedSize() const {
    return sizeof(uint32_t) * 4;
}


/*
 * Implementation of Aggregate Application
 */
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
}

void AggregateApplication::StartApplication()
{
    std::cout << "Hello world thingy is starting!!\n"; 
}

void AggregateApplication::StopApplication()
{
}
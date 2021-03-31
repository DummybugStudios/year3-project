#include <iostream>
#include "AggregateApplication.h"

#include "ns3/core-module.h"
#include "ns3/type-id.h"

using namespace ns3; 

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
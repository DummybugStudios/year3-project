//
// Created by sid on 17/04/2021.
//

#include "TRIPApplication.h"
#include <iostream>

using namespace ns3;
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
}

TRIPApplication::~TRIPApplication() {

}

void TRIPApplication::StartApplication() {
    std::cout << "Starting other application\n";
}

void TRIPApplication::StopApplication() {

}

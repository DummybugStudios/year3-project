//
// Created by sid on 17/04/2021.
//

#ifndef NS_3_32_TRIPAPPLICATION_H
#define NS_3_32_TRIPAPPLICATION_H
#include "ns3/application.h"
#include "ns3/core-module.h"


using namespace ns3;

class TRIPApplication : public Application{
public:
    static TypeId GetTypeId();
    TRIPApplication();
    ~TRIPApplication();

private:
    virtual void StartApplication();
    virtual void StopApplication();
    bool isEvil;
};


#endif //NS_3_32_TRIPAPPLICATION_H

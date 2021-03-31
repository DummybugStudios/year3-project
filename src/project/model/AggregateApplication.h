#ifndef AGGREGATE_APPLICATION_H
#define AGGREGATE_APPLICATION_H

#include "ns3/application.h"

using namespace ns3; 

class AggregateApplication : public Application
{
    public:
    static TypeId GetTypeId();
    AggregateApplication();


    private:
    virtual void StartApplication();
    virtual void StopApplication();
    bool isEvil;
};

#endif
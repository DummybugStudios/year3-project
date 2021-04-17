#include "ns3/core-module.h"

#include "ns3/VanetApplication.h"
#include "ns3/AggregateApplication.h"
#include "ns3/TRIPApplication.h"
#include "VanetApplicationHeper.h"
#include <iostream>

using namespace ns3;

VanetApplicationHelper::VanetApplicationHelper(bool isEvil, int algo)
:m_algo(algo)
{
    switch (algo)
    {
        case 0:
            m_factory.SetTypeId(VanetApplication::GetTypeId());
            break;
        case 1:
            m_factory.SetTypeId(AggregateApplication::GetTypeId());
            break;
        case 2:
            m_factory.SetTypeId(TRIPApplication::GetTypeId());
            break;
    }
    m_factory.Set("Evil", BooleanValue(isEvil));
}

ApplicationContainer VanetApplicationHelper::Install(Ptr<Node> node) const
{
    return ApplicationContainer(InstallPriv(node));
}

ApplicationContainer VanetApplicationHelper::Install(NodeContainer c) const
{
    Ptr<Application> app = m_factory.Create<VanetApplication>();
    ApplicationContainer apps;
    for (NodeContainer::Iterator it = c.Begin(); it != c.End(); ++it)
    {
        apps.Add(InstallPriv(*it));
    }

    return apps;
}


Ptr<Application> VanetApplicationHelper::InstallPriv(Ptr<Node> node) const
{
    Ptr<Application> app;
    switch(m_algo)
    {
        case 0:
            app = m_factory.Create<VanetApplication>();
            break;
        case 1:
            app = m_factory.Create<AggregateApplication>();
            break;
        case 2:
            app = m_factory.Create<TRIPApplication>();
            break;
    }
    node->AddApplication(app) ; 
    return app; 
}
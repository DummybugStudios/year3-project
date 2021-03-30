#include "ns3/core-module.h"

#include "ns3/VanetApplication.h"
#include "VanetApplicationHeper.h"

using namespace ns3;

VanetApplicationHelper::VanetApplicationHelper(bool isEvil)
{
    m_factory.SetTypeId(VanetApplication::GetTypeId());
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
    Ptr<Application> app = m_factory.Create<VanetApplication>();
    node->AddApplication(app) ; 
    return app; 
}
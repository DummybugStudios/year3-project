//
// Created by sid on 23/04/2021.
//

#include "RSUApplicationHelper.h"
#include "ns3/RSUApplication.h"

RSUApplicationHelper::RSUApplicationHelper() {
    m_factory.SetTypeId(RSUApplication::GetTypeId());
}

ApplicationContainer RSUApplicationHelper::Install(Ptr <Node> node) const {
    return ApplicationContainer(InstallPriv(node));
}

ApplicationContainer RSUApplicationHelper::Install(NodeContainer c) const {
    Ptr<Application> app = m_factory.Create<RSUApplication>();
    ApplicationContainer apps;
    for (auto it = c.Begin(); it != c.End(); ++it)
    {
        apps.Add(InstallPriv(*it));
    }
    return apps;
}

Ptr <Application> RSUApplicationHelper::InstallPriv(Ptr <Node> node) const {
    Ptr<Application> app;
    app = m_factory.Create<RSUApplication>();
    node->AddApplication(app);
    return app;
}


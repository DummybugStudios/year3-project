#include "project-bsm-application.h"
#include <iostream>

using namespace ns3;

NS_OBJECT_ENSURE_REGISTERED(ProjectBsmApplication);

TypeId ProjectBsmApplication::GetTypeId() {
  static TypeId tid = TypeId ("ProjectBsmApplication")
    .SetParent<BsmApplication> ()
    .AddConstructor<ProjectBsmApplication> ();
  return tid;
}
 
ProjectBsmApplication::ProjectBsmApplication() : BsmApplication()
{
}

void ProjectBsmApplication::StartApplication()
{
}
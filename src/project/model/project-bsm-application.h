#ifndef PROJECT_WAVE_BSM
#define PROJECT_WAVE_BSM

#include "ns3/bsm-application.h"

using namespace ns3; 

class ProjectBsmApplication : public BsmApplication
{
    public:
    static TypeId GetTypeId();
    ProjectBsmApplication();
    private:

    virtual void StartApplication();

};

#endif

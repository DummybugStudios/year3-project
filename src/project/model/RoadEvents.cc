#include <iostream> 
#include "RoadEvents.h"
#include <ns3/random-variable-stream.h>

using namespace ns3;

bool RoadEventManger::initialized(false);

vector<RoadEvent> RoadEventManger::events;

void RoadEventManger::setupEvents(int n, int xmax, int ymax)
{
    // only initialze once 
    if (initialized) return; 


    // TODO: let's do this somewehre else in the future yeah? 

    Ptr<UniformRandomVariable> m_univr = CreateObject<UniformRandomVariable>();

    for (int i =0; i < n; i++)
    { 

        int x = m_univr->GetInteger(0, xmax);
        int y = m_univr->GetInteger(0, ymax);
        int val = m_univr->GetInteger(0,n*2);

        events.push_back(RoadEvent(x, y, val));
        
    };

    initialized = true;
}


RoadEvent* RoadEventManger::getNearestEvent(int x, int y , int threshold)
{
    for (RoadEvent& event : events)
    {
        // strange way to check if number lies in the bounds of two other numbers
        if ((unsigned int) (x - event.x )< (unsigned int)threshold && (unsigned int)(y - event.y)< (unsigned int)threshold)
        {
            return &event; 
        }
    }
    return nullptr;
}




void RoadEventManger::debugPrintEvents()
{
    if (!initialized)
    {
        std::cout << "Events not initalized yet" << endl;
        return; 
    }

    std::cout << "Events initialized" << endl;

    for (auto const &it : events)
    {
        std::cout << it << endl;
    }

}

vector<RoadEvent *> RoadEventManger::getReachableEvents(int x, int y, int threshold) {

    vector<RoadEvent *> retval;
    for (RoadEvent& event : events)
    {
        // strange way to check if number lies in the bounds of two other numbers
        if ((unsigned int) (x - event.x )< (unsigned int)threshold && (unsigned int)(y - event.y)< (unsigned int)threshold)
        {
            retval.push_back(&event);
        }
    }
    return retval;
}

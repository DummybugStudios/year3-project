#include <iostream> 
#include <cstdlib>
#include <ctime> 
#include "RoadEvents.h"

using namespace std; 

bool RoadEventManger::initialized(false);

vector<RoadEvent> RoadEventManger::events;

void RoadEventManger::setupEvents(int n, int xmax, int ymax)
{
    // only initialze once 
    if (initialized) return; 

    // TODO: let's do this somewehre else in the future yeah? 
    srand(time(0));

    for (int i =0; i < n; i++)
    { 
        int x = rand() % xmax;
        int y = rand() % ymax;
        int val = rand() % (n*2); 

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
        cout << "Events not initalized yet" << endl;
        return; 
    }

    cout << "Events initialized" << endl;

    for (auto const &it : events)
    {
        cout << it << endl;
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

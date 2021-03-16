#ifndef ROAD_EVENTS_H
#define ROAD_EVENTS_H

#include <iostream>
#include <vector> 
#include <string>


using namespace std; 

class RoadEvent
{ 
    public:

    RoadEvent(int x, int y, int val) : x(x), y(y), val(val){};
    int x, y, val;

    friend ostream& operator<< (ostream &out, const RoadEvent &coords)
    {
        out << "(" << coords.x << ", " << coords.y << ")" << ": " <<coords.val;
        return out;
    };
};


class RoadEventManger
{
    public:
    static void setupEvents(int n, int x, int y);
    static RoadEvent * getNearestEvent(int x, int y, int threshold);
    static void debugPrintEvents();
    static vector<RoadEvent> & getEvents(){return events;};

    private: 
    // maybe it shouldn't be a vector but we will see
    static vector<RoadEvent> events;
    static bool initialized;
};


#endif
/**
 * This class keeps track of the right and wrong guesses to allow you to collect data about the 
 * efficacy of the protocol being used to judge the cars
 **/
#include <iostream>
#include <vector> 

#include "EventLogger.h"

int EventLogger::m_right(0);
int EventLogger::m_wrong(0);

void EventLogger::guess(int x, int y, int val, bool accepted)
{
    vector<RoadEvent> events = RoadEventManger::getEvents();


    for (auto const &event : events)
    {
        if (event.x == x && event.y == y && event.val == val) 
        {
            if (accepted)
                m_right++;
            else
                m_wrong++;

            return;
        }
    }

    if (accepted)
        m_wrong++;
    else
        m_right++; 

}

void EventLogger::printStats()
{
    std::cout << m_right << " " << m_wrong << std::endl;
}
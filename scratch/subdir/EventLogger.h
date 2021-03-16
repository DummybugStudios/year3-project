#ifndef EVENT_LOGGER_H
#define EVENT_LOGGER_H

#include "RoadEvents.h"


class EventLogger
{
    public:
    static void guess(int x ,int y, int val, bool accepted);
    static void printStats();
    static int m_wrong, m_right;
};

#endif
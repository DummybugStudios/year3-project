#ifndef EVENT_LOGGER_H
#define EVENT_LOGGER_H

#include "RoadEvents.h"

enum EventType
{
    ARRIVED,
    ACCEPTED,
    REJECTED
};

class EventLogger
{
    public:
    static void guess(uint32_t nodeid, int x, int y, int val, EventType type);
private:
    static bool firstCall;
};

#endif
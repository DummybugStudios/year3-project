/**
 * This class keeps track of the right and wrong guesses to allow you to collect data about the 
 * efficacy of the protocol being used to judge the cars
 **/
#include <iostream>
#include <vector> 

#include "EventLogger.h"
#include "ns3/simulator.h"
#include "ns3/global-value.h"
#include "ns3/integer.h"
#include <fstream>

bool EventLogger::firstCall(true);

void EventLogger::guess(uint32_t nodeid, int x, int y, int val, EventType type)
{
    vector<RoadEvent> events = RoadEventManger::getEvents();
    std::ofstream csvFile;

    ns3::IntegerValue runNumber;
    ns3::GlobalValue::GetValueByName("VRCrunNumber", runNumber);
    std::string name = "results/results" + std::to_string(runNumber.Get()) +".csv";
    // Open File in append mode
    if (firstCall)
    {
        // Write csv heading the first time it is called
        // time is the simulator time
        // node id is the id of the node calling this event
        // x, y, and val are information about the event
        // type is either arrived, rejected, or accepted (0,1,or 2 respectively)
        csvFile.open(name, ios::trunc);
        csvFile  << "time, id, x, y, val, type"<< std::endl;
        csvFile.close();
        firstCall = false;
    }
    csvFile.open(name, ios::app);
    csvFile << ns3::Simulator::Now().GetMilliSeconds() <<"," <<
    nodeid<< ","<< x << "," << y << "," << val <<","<< type << std::endl;
}
//
// Created by sid on 02/04/2021.
//

#include "project-group.h"
#include "ns3/global-value.h"
#include "ns3/integer.h"
#include <iostream>

using namespace ns3;
int Group::GetGroup(int x, int y) {
    IntegerValue worldWidth;
    IntegerValue cellSize;
    GlobalValue::GetValueByName("VRCworldWidth", worldWidth);
    GlobalValue::GetValueByName("VRCcellSize", cellSize);

    NS_ASSERT_MSG((worldWidth.Get() % cellSize.Get())== 0, "cellsize is not a multiple of world size");

    // Honestly I'm too tired to figure out if there's a quicker way to do this
    int groupsPerRow = worldWidth.Get() / cellSize.Get();
    int rowindex = x / cellSize.Get();
    int colindex = y / cellSize.Get();

    int rowid = rowindex + colindex*groupsPerRow;

    return rowid;
}


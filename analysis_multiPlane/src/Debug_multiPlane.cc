#include "RootIO.hh"
#include "DigitalUtils.hh"
#include "Utils.hh"
#include "CalorimetryUtils.hh"
#include "Debug_multiPlane.hh"
#include <iostream>
#include <random>

void Debug_multiPlane()
{
    ThresholdMap thresholdMap{};

    for(int i=0; i<511; i++)
    {
        for(int j=0; j<511; j++)
        {
            thresholdMap[{i,j}] = 0;
        }
    }

    double inputThreshold = 0;
    int runNumber = 227;
    std::string saveName = "Test";

    AnaFlags cfg{};
    std::string configPath = "configs/analysis_flags_MP_Vlad.cfg";
    LoadAnalysisFlagsFromFile(configPath, cfg);

    auto multiPlanes = CaloPreProcessing(inputThreshold, runNumber, saveName, cfg);
    auto rawHits = GetRawHits(multiPlanes[0]);
    auto vertex  = GetVertex(cfg, runNumber);
    /*
    for (int i=0;i<rawHits.size();i++)
    {
        std::cout << "EventID: " << rawHits[i].key.event 
                  << " ;Plane: "   << rawHits[i].key.plane
                  << " ;X: "       << rawHits[i].key.x
                  << " ;Y: "       << rawHits[i].key.y
                  << " ;Energy: "  << rawHits[i].energy
                  << " ;Time: "    << rawHits[i].time << std::endl;
    }

    for (int i=0;i<vertex.size();i++)
    {
        std::cout << "VX: "   <<  vertex[i].x
                  << " ;VY: "   <<  vertex[i].y
                  << " ;Time: " <<  vertex[i].t << std::endl;
    }
    */

    auto [enMap,timeMap]  = BuildEnergyTimeMap(rawHits);
    auto sortedTimings  = CorrectAndSortTimeMap(enMap, timeMap, thresholdMap, cfg, std::random_device{}());
    auto digitizedWords = AssignMALTA2WordBuckets(enMap, sortedTimings, thresholdMap, cfg);

    int i = 0;
    for (const auto& el: digitizedWords)
    {
        i++;
    }
    std::cout << i << std::endl;
}
#include "PRIOFIFOHWCProcessing_multiPlane.hh"
#include "ConfigAnalysis.hh"
#include "CalorimetryUtils.hh"
#include "DigitalProcessing_multiPlane.hh"
#include "DigitalUtils.hh"
#include "RootIO.hh"
#include "Tracking_multiPlane.hh"
#include "Utils.hh"
#include <chrono>
#include <iostream>
#include <random>
#include <vector>

void PRIOFIFOHWCProcessing_multiPlane(double inputThreshold, int runNumber, std::string saveName)
{

    auto start = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> elapsed{};
    std::chrono::time_point<std::chrono::high_resolution_clock> end;

    std::cout << "############################# FIFO Processing started!" << std::endl;

    auto config = GetDigitalConfig();
    std::cout << "GetDigitalConfig done!" << std::endl;
    ThresholdMap thresholdMap = generateThrMap(inputThreshold, runNumber, saveName, config, std::random_device{}());
    std::cout << "generateThrMap done!" << std::endl;
    auto multiPlanes = CaloPreProcessing(inputThreshold, runNumber, saveName);
    std::cout << "CaloPreProcessing done!" << std::endl;
    std::string inputPath = config.inputPath + Form("_%04d/", runNumber);
    DetectorConfig detConfig = LoadConfig(inputPath + "flags.cfg");
    std::cout << "LoadConfig done!" << std::endl;
    auto geoMaps = LoadGeometry(config.geoFile, detConfig);
    std::cout << "LoadGeometry done!" << std::endl;

    std::vector<int> planes;
    int nPlanes_100 = config.nPlanes_100;
    int nPlanes_10 = config.nPlanes_10;
    int nPlanes_1 = config.nPlanes_1;
    int nPlanes = nPlanes_100*nPlanes_10*nPlanes_1;
    for (int iz = 0; iz < nPlanes_100; ++iz) 
    {
        for (int iy = 0; iy < nPlanes_10; ++iy) 
        {
            for (int ix = 0; ix < nPlanes_1; ++ix) 
            {
                planes.push_back(iz*10000 + iy*100 + ix);
            }
        }
    }
    
    for (int i = 0; i< nPlanes; i++)
    {
        auto rawHits = GetRawHits(multiPlanes[i]);
        std::cout << "Plane " << i << " GetRawHits done!" << std::endl;      
        auto [enMap,timeMap]  = BuildEnergyTimeMap(rawHits);
        std::cout << "Plane " << i << " BuildEnergyTimeMap done!" << std::endl; 
        auto sortedTimings  = CorrectAndSortTimeMap(enMap, timeMap, thresholdMap, config, std::random_device{}());
        std::cout << "Plane " << i << " CorrectAndSortTimeMap done!" << std::endl; 
        auto encodedWords =  EncodeMALTA3ReducedWord(enMap, sortedTimings, thresholdMap, config);
        std::cout << "Plane " << i << " EncodeMALTA3ReducedWord done!" << std::endl; 
        auto busMap = SortWordsByBus(encodedWords);
        std::cout << "Plane " << i << " SortWordsByBus done!" << std::endl; 
        auto groupMap = SortWordsByGroup(busMap);
        std::cout << "Plane " << i << " SortWordsByGroup done!" << std::endl; 
        auto groupMergedWords = GroupMerge(groupMap, config);
        std::cout << "Plane " << i << " GroupMerge done!" << std::endl; 
        auto groupMergedBusMap =  SortWordsByBus(groupMergedWords);
        std::cout << "Plane " << i << " SortWordsByBus done!" << std::endl; 
        auto busMergedWords = BusMerge(groupMergedBusMap, config);
        std::cout << "Plane " << i << " BusMerge done!" << std::endl; 
        auto memoryPassedWords = MemorySynchronize(busMergedWords, config);
        std::cout << "Plane " << i << " MemorySynchronize done!" << std::endl; 
        auto fifoPassedWords = FIFOPass(memoryPassedWords, config);
        std::cout << "Plane " << i << " FIFOPass done!" << std::endl; 
        auto malta3Hits =  DecodeMALTA3HWCHits(fifoPassedWords, planes[i], geoMaps[planes[i]], config);
        std::cout << "Plane " << i << " DecodeMALTA3HWCHits done!" << std::endl; 
        FillMALTA3HWCTree(malta3Hits, config, inputThreshold, runNumber, saveName);
        std::cout << "Plane " << i << " FillMALTA3HWCTree done!" << std::endl; 
    }

    end = std::chrono::high_resolution_clock::now();
    elapsed = end - start;
    std::cout << "############################# FIFO Processing stopped after " << elapsed.count() << "ms" << std::endl;
}

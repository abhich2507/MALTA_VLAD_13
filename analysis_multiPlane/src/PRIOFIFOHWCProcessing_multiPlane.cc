#include "PRIOFIFOHWCProcessing_multiPlane.hh"
#include "DigitalProcessing_multiPlane.hh"
#include "Tracking_multiPlane.hh"
#include "CaloPreProcessing.hh"
#include "ConfigAnalysis.hh"
#include "Utils.hh"
#include <bit>
#include <queue>
#include "TRandom3.h"
#include "Utils.hh"
#include <TTree.h>
#include "Utils.hh"
#include "TStyle.h"
#include "TROOT.h"
#include <TROOT.h>
#include <TStyle.h>
#include <TH1.h>
#include <TH2.h>
#include <TCanvas.h>
#include <vector>
#include <utility>
#include <cstdint>
#include <iostream>
#include <ROOT/RNTuple.hxx>
#include <TH3D.h>
#include <TFile.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <chrono>
#include <random>

void PRIOFIFOHWCProcessing_multiPlane(double inputThreshold, int runNumber, std::string saveName)
{

    auto start = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> elapsed{};
    std::chrono::time_point<std::chrono::high_resolution_clock> end;

    std::cout << "############################# FIFO Processing started!" << std::endl;


    auto config = GetDigitalConfig();
    ThresholdMap thresholdMap = generateThrMap(inputThreshold, runNumber, saveName, config, std::random_device{}());
    auto multiPlanes = CaloPreProcessing(inputThreshold, runNumber, saveName);
    std::string inputPath = config.inputPath + Form("_%04d/", runNumber);
    DetectorConfig detConfig = LoadConfig(inputPath + "flags.cfg");
    auto geoMaps = LoadGeometry(config.geoFile, detConfig);

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
        auto [enMap,timeMap]  = BuildEnergyTimeMap(rawHits);
        auto sortedTimings  = CorrectAndSortTimeMap(enMap, timeMap, thresholdMap, config, std::random_device{}());
        auto encodedWords =  EncodeMALTA3ReducedWord(enMap, sortedTimings, thresholdMap, config);
        

        auto busMap = SortWordsByBus(encodedWords);

        auto groupMap = SortWordsByGroup(busMap);

        auto groupMergedWords = GroupMerge(groupMap, config);

        auto groupMergedBusMap =  SortWordsByBus(groupMergedWords);

        auto busMergedWords = BusMerge(groupMergedBusMap, config);

        auto memoryPassedWords = MemorySynchronize(busMergedWords, config);

        auto fifoPassedWords = FIFOPass(memoryPassedWords, config);

        auto malta3Hits =  DecodeMALTA3HWCHits(fifoPassedWords, planes[i], geoMaps[planes[i]], config);

        FillMALTA3HWCTree(malta3Hits, config, inputThreshold, runNumber, saveName);
    }


    end = std::chrono::high_resolution_clock::now();
    elapsed = end - start;
    std::cout << "############################# FIFO Processing stopped after " << elapsed.count() << "ms" << std::endl;
}

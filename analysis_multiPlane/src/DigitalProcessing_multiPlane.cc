#include "DigitalProcessing_multiPlane.hh"
#include "CalorimetryUtils.hh"
#include "DigitalUtils.hh"
#include "RootIO.hh"
#include "Utils.hh"
#include <chrono>
#include <iostream>
#include <random>
#include <vector>

void DigitalProcessing_multiPlane(double inputThreshold, int runNumber, std::string saveName, bool proteusFlag)
{
    std::cout << "############################# Digital Processing MultiPlane started!" << std::endl;
    auto start = std::chrono::high_resolution_clock::now();
    auto config = GetDigitalConfig();
    std::cout << "GetDigitalConfig done!" << std::endl;
    ThresholdMap thresholdMap = generateThrMap(inputThreshold, runNumber, saveName, config, std::random_device{}());
    std::cout << "generateThrMap done!" << std::endl;
    auto multiPlanes = CaloPreProcessing(inputThreshold, runNumber, saveName); 
    std::cout << "CaloPreProcessing done!" << std::endl;

    std::vector<int> modules{};
    for (int i = 0; i< config.modules; i++)
    {
        modules.push_back(i);
    }

    std::vector<ProcessedHit> allProcessHits{};

    for (size_t m = 0; m < modules.size(); m++)
    {
        auto rawHits = GetRawHits(multiPlanes[m]);
        std::cout << "Module " << m << " GetRawHits done!" << std::endl;   
        auto [enMap,timeMap]  = BuildEnergyTimeMap(rawHits);
        std::cout << "Module " << m << " BuildEnergyTimeMap done!" << std::endl;   
        auto sortedTimings  = CorrectAndSortTimeMap(enMap, timeMap, thresholdMap, config, std::random_device{}());
        std::cout << "Module " << m << " CorrectAndSortTimeMap done!" << std::endl;  
        auto digitizedWords = AssignMALTA2WordBuckets(enMap, sortedTimings, thresholdMap, config);
        std::cout << "Module " << m << " AssignMALTA2WordBuckets done!" << std::endl;  
        auto mergedWords = MergeMALTA2Words(digitizedWords);
        std::cout << "Module " << m << " MergeMALTA2Words done!" << std::endl;  
        auto processedHits = ProcessDecodedHits(mergedWords, config, std::random_device{}());
        std::cout << "Module " << m << " ProcessDecodedHits done!" << std::endl;  
        for (const auto& el: processedHits) allProcessHits.push_back(el);
    }
    FillReconstructedTree(allProcessHits, inputThreshold, runNumber, saveName, config);
    std::cout << "FillReconstructedTree done!" << std::endl;

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> elapsed = end - start;
    std::cout << "############################# Digital Processing MultiPlane stopped after " << elapsed.count() << "ms" << std::endl;
}


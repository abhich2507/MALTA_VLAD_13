#include "DigitalProcessing_multiPlane.hh"
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
#include "ConfigAnalysis.hh"
#include "CaloPreProcessing.hh"
#include <TROOT.h>
#include <TStyle.h>
#include <TH1.h>
#include <TH2.h>
#include <TCanvas.h>
#include <Utils.hh>

void DigitalProcessing_multiPlane(double inputThreshold, int runNumber, std::string saveName, bool proteusFlag)
{
    std::cout << "############################# Digital Processing MultiPlane starter " << std::endl;
    auto start = std::chrono::high_resolution_clock::now();
    auto config = GetDigitalConfig();
    ThresholdMap thresholdMap = generateThrMap(inputThreshold, runNumber, saveName, config, std::random_device{}());
    auto multiPlanes = CaloPreProcessing(inputThreshold, runNumber, saveName); 

    std::vector<int> modules{};
    for (int i = 0; i< config.modules; i++)
    {
        modules.push_back(i);
    }

    std::vector<ProcessedHit> allProcessHits{};

    for (size_t m = 0; m < modules.size(); m++)
    {
        auto rawHits = GetRawHits(multiPlanes[m]);        
        auto [enMap,timeMap]  = BuildEnergyTimeMap(rawHits);
        auto sortedTimings  = CorrectAndSortTimeMap(enMap, timeMap, thresholdMap, config, std::random_device{}());
        auto digitizedWords = AssignMALTA2WordBuckets(enMap, sortedTimings, thresholdMap, config);
        auto mergedWords = MergeMALTA2Words(digitizedWords);
        auto processedHits = ProcessDecodedHits(mergedWords, config, std::random_device{}());
        for (const auto& el: processedHits) allProcessHits.push_back(el);
    }
    FillReconstructedTree(allProcessHits, inputThreshold, runNumber, saveName, config);

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> elapsed = end - start;
    std::cout << "############################# Digital Processing MultiPlane stopped after " << elapsed.count() << "ms" << std::endl;
}


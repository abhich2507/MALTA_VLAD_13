#include "DigitalProcessing_multiPlane.hh"
#include "Tracking_multiPlane.hh"
#include "Calorimetry_multiPlane.hh"
#include <Utils.hh>
#include<stack>
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


void Calorimetry_multiPlane(float threshold, int runNumber, std::string saveName)
{
    auto start = std::chrono::high_resolution_clock::now();
    std::cout << "############################# Calorimetry MultiPlane started!" << std::endl;

    auto config = GetDigitalConfig(); 
    std::cout << "GetDigitalConfig done!" << std::endl;
    std::string inputPath = config.inputPath+Form("_%04d/", runNumber);

    auto calorimetryHits = GetCalorimetryHits(config, runNumber, saveName, threshold);
    std::cout << "GetCalorimetryHits done!" << std::endl;
    auto positionHits = GetPositionHits(config, runNumber, saveName, threshold);
    std::cout << "GetPositionHits done!" << std::endl;
    auto processedHits = ProcessCalorimetry(calorimetryHits, positionHits, config);
    std::cout << "ProcessCalorimetry done!" << std::endl;
    FillClusterTree(config, runNumber, saveName, threshold, processedHits);
    std::cout << "FillClusterTree done!" << std::endl;
    auto rawCaloPerMap = GetCalorimetryAnalyzedHits(runNumber, saveName, threshold);
    std::cout << "GetCalorimetryAnalyzedHits done!" << std::endl;
    auto fitCaloInfo = GetCalorimetryMultiLayerFitInformation(rawCaloPerMap);
    std::cout << "GetCalorimetryMultiLayerFitInformation done!" << std::endl;
    SaveCalorimetryHistograms(rawCaloPerMap, fitCaloInfo, runNumber, saveName, threshold);
    std::cout << "SaveCalorimetryHistograms done!" << std::endl;
    
    auto end = std::chrono::high_resolution_clock::now(); 
    std::chrono::duration<float, std::milli> elapsed = end - start;     
    std::cout << "############################# Calorimetry stopped after " << elapsed.count() << " ms" << std::endl;
}

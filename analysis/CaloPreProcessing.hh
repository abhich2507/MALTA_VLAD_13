

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
#include "Tracking.hh"

std::vector<TTree*> CaloPreProcessing(double inputThreshold, int runNumber, std::string saveName)
{
    auto start = std::chrono::high_resolution_clock::now();
    // Set all the analysis flags for the digital processing
    auto analysisFlags = new SimFlags{};
    const char* configPath = std::getenv("ANALYSIS_CONFIG");
    LoadAnalysisFlagsFromFile(configPath, *analysisFlags);
    std::string localPath = "./";
    std::string inputPath = localPath +  Form("Results/local_%04d/", runNumber);
    std::string directoryPath = localPath + "Plots/";
    std::string runPath = Form("local_%04d/", runNumber);

    std::cout << "############################# Calo PreProcessing started for:" << std::endl;
    std::cout << inputPath << std::endl;
    int numThreads = analysisFlags->numThreads; 
    int nPlanes = analysisFlags->nPlanes;
    // Extract raw data
    TChain *chainPixel = new TChain("RawPixelHits");
    for (int t = 0; t <= numThreads - 1; ++t) 
    {
        chainPixel->Add(Form("%soutput0_t%d.root", inputPath.c_str() , t));
    }
    std::vector<TTree*> forest{};
    forest.resize(nPlanes);
    for (int p=0; p<nPlanes; p++)
    {
        TTree* treeSplit = chainPixel->CopyTree(Form("iPlane==%d", p));
        treeSplit->SetName(Form("Plane%dHits", p));
        forest[p] = treeSplit;
    }
    return forest;

}
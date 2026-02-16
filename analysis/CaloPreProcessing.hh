

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
    std::string localPath = analysisFlags->localPath;
    std::string inputPath = analysisFlags->inputPath+Form("_%04d/", runNumber);
    std::string directoryPath = localPath + "Plots/";
    //std::string runPath = Form("local_%04d/", runNumber);

    std::cout << "############################# Calo PreProcessing started for:" << std::endl;
    std::cout << inputPath << std::endl;
    int numThreads = analysisFlags->numThreads; 

    int nPlanes_100 = analysisFlags->nPlanes_100;
    int nPlanes_10 = analysisFlags->nPlanes_10;
    int nPlanes_1 = analysisFlags->nPlanes_1;
    int nPlanes = nPlanes_100*nPlanes_10*nPlanes_1; // total number of planes

    // Extract raw data
    TChain *chainPixel = new TChain("RawPixelHits");
    std::string fileName = analysisFlags->fileName;
    for (int t = 0; t <= numThreads - 1; ++t) 
    {
        chainPixel->Add(Form("%s%s_t%d.root", inputPath.c_str(), fileName.c_str(), t));
    }
    std::vector<TTree*> forest{};
    forest.resize(nPlanes);

    std::vector<int> planes;
    for (int iz = 0; iz < nPlanes_100; ++iz) {
        for (int iy = 0; iy < nPlanes_10; ++iy) {
            for (int ix = 0; ix < nPlanes_1; ++ix) {
                planes.push_back(iz*100 + iy*10 + ix); // decoded position (works for up to 10 planes in each dimension)
            }
        }
    }

    for (int p : planes)
    {
        TTree* treeSplit = chainPixel->CopyTree(Form("iPlane==%d", p));
        treeSplit->SetName(Form("Plane%dHits", p));
        forest[p] = treeSplit;
        treeSplit->SetDirectory(nullptr);
    }
    return forest;

}
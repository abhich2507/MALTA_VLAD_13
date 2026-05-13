#include "Analysis_multiPlane.hh"
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



void Analysis_multiPlane(double threshold, int runNumber, std::string saveName)
{
    auto start = std::chrono::high_resolution_clock::now();
    std::cout << "############################# Analysis started!" << std::endl;
    auto config = GetDigitalConfig(); 
    std::string inputPath = config.inputPath+Form("_%04d/", runNumber);
    DetectorConfig detConfig = LoadConfig(inputPath + "flags.cfg");

    // Get analysistree for each PlaneZ
    int nPlanes_100 = config.nPlanes_100;
    for (int planeZ = 0; planeZ<nPlanes_100; planeZ++)
    {        
        auto histograms2D = Create2DHistograms(config, planeZ);
        auto histograms1D = Create1DHistograms(planeZ);
        auto analysisHits = GetAnalysisHits(config, threshold, runNumber, saveName, planeZ);
        auto h2PASSInPixelAux = FillHistograms(analysisHits, histograms2D, histograms1D, config, detConfig, planeZ);
        auto statistics = GetStatistics(histograms2D, h2PASSInPixelAux);
        ScaleHistograms(histograms2D, histograms1D, h2PASSInPixelAux);
        SetHistogramStyle(histograms2D, histograms1D, statistics);
        SaveHistograms(histograms2D, histograms1D, h2PASSInPixelAux, config, threshold, runNumber, saveName);
        SaveSummaryRoot(config, runNumber, saveName, threshold, statistics);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> elapsed = end - start;
    std::cout << "############################# Analysis stopped after " << elapsed.count() << "ms" << std::endl;
}
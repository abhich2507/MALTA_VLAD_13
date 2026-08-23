#include "Analysis_multiPlane.hh"
#include "AnalysisUtils.hh"
#include "RootIO.hh"
#include "Utils.hh"
#include <chrono>
#include <iostream>

void Analysis_multiPlane(double threshold, int runNumber, std::string saveName)
{
    auto start = std::chrono::high_resolution_clock::now();
    std::cout << "############################# Analysis started!" << std::endl;
    auto config = GetDigitalConfig(); 
    std::cout << "GetDigitalConfig done!" << std::endl;
    std::string inputPath = config.inputPath+Form("_%04d/", runNumber);
    DetectorConfig detConfig = LoadConfig(inputPath + "flags.cfg");
    std::cout << "LoadConfig done!" << std::endl;

    int nPlanes_100 = config.nPlanes_100;
    for (int planeZ = 0; planeZ<nPlanes_100; planeZ++)
    {        
        auto histograms2D = Create2DHistograms(config, planeZ);
        std::cout << "PlaneZ " << planeZ <<" Create2DHistograms done!" << std::endl;
        auto histograms1D = Create1DHistograms(planeZ);
        std::cout << "PlaneZ " << planeZ <<" Create1DHistograms done!" << std::endl;
        auto analysisHits = GetAnalysisHits(config, threshold, runNumber, saveName, planeZ);
        std::cout << "PlaneZ " << planeZ <<" GetAnalysisHits done!" << std::endl;
        auto h2PASSInPixelAux = FillHistograms(analysisHits, histograms2D, histograms1D, config, detConfig, planeZ);
        std::cout << "PlaneZ " << planeZ <<" FillHistograms done!" << std::endl;
        auto statistics = GetStatistics(histograms2D, h2PASSInPixelAux);
        std::cout << "PlaneZ " << planeZ <<" GetStatistics done!" << std::endl;
        ScaleHistograms(histograms2D, histograms1D, h2PASSInPixelAux);
        std::cout << "PlaneZ " << planeZ <<" ScaleHistograms done!" << std::endl;
        SetHistogramStyle(histograms2D, histograms1D, statistics);
        std::cout << "PlaneZ " << planeZ <<" SetHistogramStyle done!" << std::endl;
        SaveHistograms(histograms2D, histograms1D, h2PASSInPixelAux, config, threshold, runNumber, saveName);
        std::cout << "PlaneZ " << planeZ <<" SaveHistograms done!" << std::endl;
        SaveSummaryRoot(config, runNumber, saveName, threshold, planeZ, statistics);
        std::cout << "PlaneZ " << planeZ <<" SaveSummaryRoot done!" << std::endl;
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> elapsed = end - start;
    std::cout << "############################# Analysis stopped after " << elapsed.count() << "ms" << std::endl;
}
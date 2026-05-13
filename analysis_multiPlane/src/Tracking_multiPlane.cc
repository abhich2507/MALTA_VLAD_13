#include "Tracking_multiPlane.hh"
#include <TChain.h>
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


void Tracking_multiPlane(double threshold, int runNumber, std::string saveName)
{
    auto start = std::chrono::high_resolution_clock::now();
    std::cout << "############################# Tracking started for:" << std::endl;

    auto config = GetDigitalConfig();    
    auto tracks = GetVertex(config, runNumber);
    auto outfile = CreateTrackedTree(threshold, runNumber, saveName, config);

    for (int planeZ = 0; planeZ<config.nPlanes_100; planeZ++)
    {
        auto hits = GetTrackHits(config, threshold, runNumber, saveName, planeZ);
        auto [fullTrackInfo, residualInfo ] = MatchHits(tracks, hits, config, runNumber);
        FillTrackedTree (fullTrackInfo, outfile, planeZ);
        CloseFile(outfile);
        SaveResidualHisto(residualInfo, planeZ, threshold, runNumber, saveName, config);
    }

    auto end = std::chrono::high_resolution_clock::now(); 
    std::chrono::duration<double, std::milli> elapsed = end - start;   
    std::cout << "############################# Tracking stopped after " << elapsed.count() << " ms" << std::endl;
}

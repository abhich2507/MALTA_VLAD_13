#include "Tracking_multiPlane.hh"
#include "RootIO.hh"
#include "TrackingUtils.hh"
#include "Utils.hh"
#include <chrono>
#include <iostream>
#include <string>

void Tracking_multiPlane(double threshold, int runNumber, std::string saveName)
{
    auto start = std::chrono::high_resolution_clock::now();
    std::cout << "############################# Tracking started!" << std::endl;

    auto config = GetDigitalConfig();
    std::cout << "GetDigitalConfig done!" << std::endl;
    auto tracks = GetVertex(config, runNumber);
    std::cout << "GetVertex done!" << std::endl;
    auto outfile = CreateTrackedTree(threshold, runNumber, saveName, config);
    std::cout << "CreateTrackedTree done!" << std::endl;

    for (int planeZ = 0; planeZ<config.nPlanes_100; planeZ++)
    {
        auto hits = GetTrackHits(config, threshold, runNumber, saveName, planeZ);
        std::cout << "PlaneZ " << planeZ <<" GetTrackHits done!" << std::endl;
        auto [fullTrackInfo, residualInfo] = MatchHits(tracks, hits, config, runNumber);
        std::cout << "PlaneZ " << planeZ <<" MatchHits done!" << std::endl;
        FillTrackedTree(fullTrackInfo, outfile, planeZ);
        std::cout << "PlaneZ " << planeZ <<" FillTrackedTree done!" << std::endl;
        //Snip- CloseFile(outfile);
        std::cout << "PlaneZ " << planeZ <<" CloseFile done!" << std::endl;
        SaveResidualHisto(residualInfo, planeZ, threshold, runNumber, saveName, config);
        std::cout << "PlaneZ " << planeZ <<" SaveResidualHisto done!" << std::endl;
    }
    CloseFile(outfile);
    auto end = std::chrono::high_resolution_clock::now(); 
    std::chrono::duration<double, std::milli> elapsed = end - start;   
    std::cout << "############################# Tracking stopped after " << elapsed.count() << " ms" << std::endl;
}


#include "Clustering_multiPlane.hh"
#include "Utils.hh"
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

void Clustering_multiPlane(double threshold, int runNumber, std::string saveName)
{
    auto start = std::chrono::high_resolution_clock::now();
    auto config = GetDigitalConfig(); 
    auto outfile = CreateClusteredTree(config, threshold, runNumber, saveName);
    std::string cfgPath = config.inputPath+Form("_%04d/", runNumber);
    DetectorConfig cfg = LoadConfig(cfgPath + "flags.cfg");

    for (int planeZ = 0; planeZ < config.nPlanes_100; planeZ++)
    {
        auto matchedHits = GetMatchedHits(config, threshold, runNumber, saveName, planeZ);
        auto tracks      = GroupHitsByTrack(matchedHits); // extract to a function

        std::vector<ClusteredHit> allClusters;
        for (const auto& track : tracks)
        {
            auto clusterState    = BuildClusterState(track);
            auto validCluster    = ValidateCluster(clusterState.cluster, cfg);
            auto vertex          = GetClusterPosition(validCluster, clusterState, config.clPos);
            allClusters.push_back(GetValidCluster(validCluster, clusterState, vertex));
        }
        FillTrackedTree(allClusters, outfile, planeZ);
    }
    CloseFile(outfile);

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> elapsed = end - start;
    std::cout << "############################# Clustering stopped after " << elapsed.count() << "ms" << std::endl;
}

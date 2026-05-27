#include "Clustering_multiPlane.hh"
#include "ClusteringUtils.hh"
#include "RootIO.hh"
#include "Utils.hh"
#include <chrono>
#include <iostream>
#include <string>
#include <vector>

void Clustering_multiPlane(double threshold, int runNumber, std::string saveName)
{
    auto start = std::chrono::high_resolution_clock::now();
    auto config = GetDigitalConfig(); 
    std::cout << "GetDigitalConfig done!" << std::endl;
    auto outfile = CreateClusteredTree(config, threshold, runNumber, saveName);
    std::cout << "CreateClusteredTree done!" << std::endl;
    std::string cfgPath = config.inputPath+Form("_%04d/", runNumber);
    DetectorConfig cfg = LoadConfig(cfgPath + "flags.cfg");
    std::cout << "LoadConfig done!" << std::endl;

    for (int planeZ = 0; planeZ < config.nPlanes_100; planeZ++)
    {
        auto matchedHits = GetMatchedHits(config, threshold, runNumber, saveName, planeZ);
        std::cout << "PlaneZ " << planeZ <<" GetMatchedHits done!" << std::endl;
        auto tracks = GroupHitsByTrack(matchedHits);
        std::cout << "PlaneZ " << planeZ <<" GroupHitsByTrack done!" << std::endl;

        std::vector<ClusteredHit> allClusters;
        for (const auto& track : tracks)
        {
            auto clusterState    = BuildClusterState(track);
            auto validCluster    = ValidateCluster(clusterState.cluster, cfg);
            auto vertex          = GetClusterPosition(validCluster, clusterState, config.clPos);
            allClusters.push_back(GetValidCluster(validCluster, clusterState, vertex));
        }
        std::cout << "PlaneZ " << planeZ <<" GetValidClusters done!" << std::endl;
        FillTrackedTree(allClusters, outfile, planeZ);
        std::cout << "PlaneZ " << planeZ <<" FillTrackedTree done!" << std::endl;
    }
    CloseFile(outfile);
    std::cout << "CloseFile done!" << std::endl;

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> elapsed = end - start;
    std::cout << "############################# Clustering stopped after " << elapsed.count() << "ms" << std::endl;
}

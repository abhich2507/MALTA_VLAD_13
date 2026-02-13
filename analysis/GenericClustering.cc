#include "MALTAClustering.hh"


/*
##############################################################################################################################################

Experimental Clustering in order to mimic exactly the MALTA testbeam analysis

##############################################################################################################################################
*/


bool verbose = false;

void GenericClustering(double threshold = 200, int runNumber = 102, std::string saveName = "Test")
{
    std::string localPath = getVarFromConfig();
    std::string inputPath = localPath +  Form("Results/local_%04d/", runNumber); // still hardcoded because: Is analysis.flags reachable here?
    std::string inputSubPath = localPath +  Form("Results/local_%04d/", runNumber) + saveName + "/";
    std::string directoryPath = localPath + "Plots/";
    std::string runPath = Form("local_%04d/", runNumber);

    double matchWindow = 200; // ns
    const std::vector<std::pair<int,int>> diagonals = { {1, 1}, {1, -1}, {-1, 1}, {-1, -1} };
    const std::vector<std::pair<int, int>> sides    = { {1, 0}, {-1, 0}, {0, 1}, {0, -1} };
    TFile *reconstructedFile = TFile::Open((inputSubPath + "Plane0ReconstructedHitsThr" + std::to_string(int(threshold)) + ".root").c_str(), "READ");
    // Get tree
    TTree *reconstructedTree = (TTree*) reconstructedFile->Get("ReconstructedHits");

    int pixX, pixY;
    double timing;
    reconstructedTree->SetBranchAddress("PixX", &pixX);
    reconstructedTree->SetBranchAddress("PixY", &pixY);
    reconstructedTree->SetBranchAddress("timing", &timing);
    Long64_t nReconstructedEntries = reconstructedTree->GetEntries();

    std::cout << "I have received: " << nReconstructedEntries << " hits" << std::endl;

    std::vector <hitCandidate> allHits;
    std::vector<hitCandidate> vcurrentCluster;
    std::vector<std::vector<hitCandidate>> vallClusters;

    // Use vector addressing instead of many ROOT IO calls
    for (int hit = 0; hit < nReconstructedEntries; hit ++)
    {
        reconstructedTree->GetEntry(hit);
        allHits.push_back({pixX, pixY, timing});
    }
    // Go through all hits and clusterize based only on time cut
    if (allHits.empty()) return;
    vcurrentCluster.push_back(allHits[0]);

    for (size_t i = 1; i < allHits.size(); i++)
    {
        // Time difference between adjacent events
        double dt = allHits[i].timing - allHits[i - 1].timing;

        if (dt <= matchWindow) 
        {
            // within window → same cluster
            vcurrentCluster.push_back(allHits[i]);
        } 
        else 
        {
            // new cluster
            vallClusters.push_back(vcurrentCluster);
            vcurrentCluster.clear();
            vcurrentCluster.push_back(allHits[i]);
        }
    }
    // At this point I have a vector of all cluster candidates. However they are not yet corrected for invalid clusters
    int numClusters{};
    for (auto& clusterCandidate: vallClusters)
    {
        numClusters++;
        // We need to put the hits in the cluster in a set for the next look-up
        std::set<std::pair<int, int>> clusterHits;
        for (const auto& hit: clusterCandidate)
        {
            clusterHits.insert({(int)hit.pixX, (int)hit.pixY});
        }
        std::vector<size_t> removeIndices;

        for (size_t i = 0; i< clusterCandidate.size(); i++)
        {
            const auto& hit = clusterCandidate[i];
            int xPos = (int)hit.pixX;
            int yPos = (int)hit.pixY;

            if(verbose) std::cout << "; pixX: " << hit.pixX << "; pixY: " << hit.pixY << "; timing: " << hit.timing << std::endl;

            for (auto &[dx, dy] : diagonals)
            {
                if (clusterHits.count({xPos + dx, yPos + dy}))
                {
                    // Check if *no* side neighbors exist
                    bool hasSideNeighbor = false;
                    for (auto &[sx, sy] : sides)
                    {
                        if (clusterHits.count({xPos + sx, yPos + sy}))
                        {
                            hasSideNeighbor = true;
                            break;
                        }
                    }

                    // If diagonal-only connection → mark for removal
                    if (!hasSideNeighbor)
                    {
                        if(verbose) std::cout << "#########################################################INVALID CLUSTER REMOVED" << std::endl;
                        
                        removeIndices.push_back(i);
                        break;
                    }
                }
            }
        }
        // Remove invalid hits from the cluster
        // (do it backwards so indices remain valid)
        std::sort(removeIndices.rbegin(), removeIndices.rend());
        for (auto idx : removeIndices)
        {
            clusterCandidate.erase(clusterCandidate.begin() + idx);
        }
        if(verbose)
        {
            std::cout << "After cluster cutting: " << std::endl;
            for (const auto& hit: clusterCandidate)
            {
                std::cout << "; pixX: " << hit.pixX << "; pixY: " << hit.pixY << "; timing: " << hit.timing << std::endl;
            }
            std::cout << "-------------------------------------------------------" << std::endl;
        }

    }
    std::cout<< "I am giving you " << numClusters << " clusters" << std::endl;

    // Save to file
    TFile *outfile = new TFile((inputSubPath + "LocalClusteredHitsThr" + std::to_string(int(threshold)) + ".root").c_str(), "RECREATE");
    // Create a TTree
    TTree *clusteredTree = new TTree("ClusteredHits", "Clustered Hits");

    int clusterID, clPixX, clPixY, clSize;
    double clTiming;

    clusteredTree->Branch("clusterID", &clusterID, "clusterID/I");
    clusteredTree->Branch("pixX", &clPixX, "pixX/I");
    clusteredTree->Branch("pixY", &clPixY, "pixY/I");
    clusteredTree->Branch("clSize", &clSize, "clSize/I");
    clusteredTree->Branch("timing", &clTiming, "timing/D");

    std::cout << "Saving to: " << outfile->GetName() << std::endl;

    int clID = 0;
    for (const auto& clusterCandidate: vallClusters)
    {
        if(verbose) std::cout << "Cluster ID: " << clID;
        for (const auto& hit: clusterCandidate)
        {
            if(verbose) std::cout << "; pixX: " << hit.pixX << "; pixY: " << hit.pixY << "; clSize: " << clSize << "; timing: " << hit.timing << std::endl;

            clusterID = clID;
            clPixX = hit.pixX;
            clPixY = hit.pixY;
            clTiming = hit.timing;
            clSize = clusterCandidate.size();
            clusteredTree->Fill();
        }
        clID++;
    }
    outfile->cd();
    clusteredTree->Write();
    outfile->Close();



}
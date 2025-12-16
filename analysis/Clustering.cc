
#include "Clustering.hh"

struct Hit
{
    int x;
    int y;
    double t;
};

bool hasHitAt(const std::vector<Hit>& cluster, int x, int y)
{
    return std::any_of(cluster.begin(), cluster.end(), [&](const Hit& h)
        {
            return h.x == x && h.y == y;
        });
}



void Clustering(double threshold, int runNumber, std::string saveName = "default")
{
    auto start = std::chrono::high_resolution_clock::now();
    ////////// Function can be used for custom analysis paths
    //std::string localPath = getVarFromConfig();
    //////////////////////////////////////////////////////////
    std::string localPath = "./";
    std::string inputPath = localPath +  Form("Results/local_%04d/", runNumber) + saveName + "/";
    
    std::cout << "############################# Clustering started for:" << std::endl;
    std::cout << inputPath << std::endl;

    TFile *trackedFile = TFile::Open((inputPath + "LocalTrackedHitsThr" + std::to_string(int(threshold)) + ".root").c_str(), "READ");
    
    auto analysisFlags = new SimFlags;
    const char* configPath = std::getenv("ANALYSIS_CONFIG");
    LoadAnalysisFlagsFromFile(configPath, *analysisFlags);
    bool verbose = analysisFlags->verboseClustering;

    // Get tree
    TTree *trackedTree = (TTree*) trackedFile->Get("TrackedHits");

    double vertexX, vertexY, globalTrigger;
    double reconstructedTime;
    int trackID, pixX, pixY, nHits;

    trackedTree->SetBranchAddress("reconstructedVertexX", &vertexX);
    trackedTree->SetBranchAddress("reconstructedVertexY", &vertexY);
    trackedTree->SetBranchAddress("reconstructedGlobalTime", &globalTrigger);
    trackedTree->SetBranchAddress("trackID", &trackID);
    trackedTree->SetBranchAddress("PixX", &pixX);  
    trackedTree->SetBranchAddress("PixY", &pixY);  
    trackedTree->SetBranchAddress("nHits", &nHits);
    trackedTree->SetBranchAddress("reconstructedLocalTime", &reconstructedTime);


    TFile *outfile = new TFile((inputPath + "analysisThr" + std::to_string(int(threshold)) + ".root").c_str(), "RECREATE");

    // Create a TTree
    TTree *analysisTree = new TTree("analyzedHits", "Analyzed Hits");

    // Variables for branches
    double analysisVertexX, analysisVertexY, timing;
    int analysisClSize;

    // Create branches
    analysisTree->Branch("analysisVertexX", &analysisVertexX, "analysisVertexX/D");
    analysisTree->Branch("analysisVertexY", &analysisVertexY, "analysisVertexY/D");
    analysisTree->Branch("clSize", &analysisClSize, "clSize/I");
    analysisTree->Branch("timing", &timing, "timing/D");

    Long64_t nTrackedEntries = trackedTree->GetEntries();

    const std::vector<std::pair<int,int>> diagonals = { {1,1}, {1,-1}, {-1,1}, {-1,-1} };
    int currentTrack = 0;
    int realHits;
    
    
    std::set<std::pair<int,int>> clusterCandidate;
    std::vector<double> clusterTiming;
    std::vector<Hit> cluster;
    double currentX, currentY;
    int entry = 0;
    
    for (int i = 0; i < nTrackedEntries; i++)
    {
        trackedTree->GetEntry(i);
        //trackVertixes.push_back(std::make_pair(vertexX, vertexY));      
        if(trackID == entry) 
        {
            //clusterCandidate.insert(std::make_pair(pixX, pixY));
            //clusterTiming.push_back(reconstructedTime);
            cluster.push_back(Hit{pixX, pixY, reconstructedTime});
            currentX = vertexX;
            currentY = vertexY;
        }
        else if (! cluster.empty())
        {
            entry++;
            int clSize = 0;
            // Now that we have a set of cluster candidates. Check that they actually are valid
            int j = 0;
            for (auto it = cluster.begin(); it != cluster.end(); )
            {
                const int xPos = it->x;
                const int yPos = it->y;
                const double hitTiming = it->t;

                if (verbose)
                {
                    std::cout << "Proposed hit in cluster with:"
                            << " xPos=" << xPos
                            << " yPos=" << yPos
                            << " hitTiming=" << hitTiming
                            << std::endl;
                }

                bool erase = false;

                // invalid pixel
                if (xPos == -1 || yPos == -1)
                {
                    erase = true;
                }

                // diagonal-only adjacency check
                for (const auto& [dx, dy] : diagonals)
                {
                    if (!erase &&
                        hasHitAt(cluster, xPos + dx, yPos + dy) &&
                        !(hasHitAt(cluster, xPos + dx, yPos) ||
                        hasHitAt(cluster, xPos, yPos + dy)))
                    {
                        erase = true;
                    }
                }

                if (erase)
                {
                    it = cluster.erase(it);
                }
                else
                {
                    ++clSize;
                    ++it;
                }
            }
            analysisVertexX = currentX;
            analysisVertexY = currentY;                
            analysisClSize = clSize;
            timing = std::min_element(cluster.begin(), cluster.end(), [](const Hit& a, const Hit& b)
            {
                return a.t < b.t;
            })->t;

            if(verbose)std::cout << "Saving to tree: " << " X = " << analysisVertexX << " ;Y = " << analysisVertexY << " ;clSize = " <<  clSize << " ;timing = " << timing << std::endl;

            analysisTree->Fill();
            // Reset and dont forget this event
            cluster.clear();
            cluster.push_back({pixX, pixY, reconstructedTime});
            currentX = vertexX;
            currentY = vertexY;
        }
        if (verbose && nHits == 1) std::cout << "NEW TRACK: " << " trackID: " << trackID << " ;xPos: " << vertexX << " ;yPos: " << vertexY << " ;LocalTime: " << reconstructedTime << std::endl;
    
    }




    analysisTree->Write();
    outfile->Close();
    auto end = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double, std::milli> elapsed = end - start;

    std::cout << "############################# Clustering stopped after " << elapsed.count() << "ms" << std::endl;
}
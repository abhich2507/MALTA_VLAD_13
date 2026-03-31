
#include "Clustering_multiPlane.hh"

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

void Clustering_multiPlane(double threshold, int runNumber, std::string saveName = "default")
{
    auto start = std::chrono::high_resolution_clock::now();
    // Set all the analysis flags for the digital processing
    auto analysisFlags = new AnaFlags;
    const char* configPath = std::getenv("ANALYSIS_CONFIG");
    LoadAnalysisFlagsFromFile(configPath, *analysisFlags);
    ////////// Function can be used for custom analysis paths
    //std::string localPath = getVarFromConfig();
    //////////////////////////////////////////////////////////
    std::string localPath = analysisFlags->localPath;
    std::string inputPath = analysisFlags->inputPath+Form("_%04d/", runNumber) + saveName + "/";
    
    std::cout << "############################# Clustering started for:" << std::endl;
    std::cout << inputPath << std::endl;

    TFile *trackedFile = TFile::Open((inputPath + "LocalTrackedHitsThr" + std::to_string(int(threshold)) + ".root").c_str(), "READ");
    
    bool verbose = analysisFlags->verboseClustering;

    TFile *outfile = new TFile((inputPath + "analysisThr" + std::to_string(int(threshold)) + ".root").c_str(), "RECREATE");
    std::string cfgPath = analysisFlags->inputPath+Form("_%04d/", runNumber);
    DetectorConfig cfg = LoadConfig(cfgPath + "flags.cfg"); // todo: does this need to be generalized?
    // Get trackedtree for each PlaneZ and store analysisTree for each PlaneZ to outfile
    //int nPlanes_100 = analysisFlags->nPlanes_100;

    int nPlanes_100 = analysisFlags->nPlanes_100;
    int nPlanes_10 = analysisFlags->nPlanes_10;
    int nPlanes_1 = analysisFlags->nPlanes_1;
    int nPlanes = nPlanes_100*nPlanes_10*nPlanes_1;
    std::vector<int> planes;
    cout << "Adding Planes to analysis: ";
    for (int iz = 0; iz < nPlanes_100; ++iz) 
    {
        for (int iy = 0; iy < nPlanes_10; ++iy) 
        {
            for (int ix = 0; ix < nPlanes_1; ++ix) 
            {
                planes.push_back(iz*10000 + iy*100 + ix); // decoded position (works for up to 10 planes in each dimension)
                std::cout << iz*10000 + iy*100 + ix << ", " ;
            }
        }
    }


    for (int planeZ = 0; planeZ<nPlanes_100; planeZ++)
    {
        std::cout << "Clustering PlaneZ: " << planeZ << std::endl;
        
        // Get TTree for each planeZ
        TTree *trackedTree = (TTree*) trackedFile->Get(Form("TrackedHits_planeZ%d",planeZ));

        double vertexX, vertexY, globalTrigger;
        double reconstructedTime;
        int trackID, pixX, pixY, nHits, planeID;
        trackedTree->SetBranchAddress("planeID", &planeID);
        trackedTree->SetBranchAddress("vertexX", &vertexX);
        trackedTree->SetBranchAddress("vertexY", &vertexY);
        trackedTree->SetBranchAddress("vertexTime", &globalTrigger);
        trackedTree->SetBranchAddress("trackID", &trackID);
        trackedTree->SetBranchAddress("DUTPixX", &pixX);  
        trackedTree->SetBranchAddress("DUTPixY", &pixY);  
        trackedTree->SetBranchAddress("DUTnHits", &nHits);
        trackedTree->SetBranchAddress("DUTLocalTime", &reconstructedTime);

        // Create a TTree
        TTree *analysisTree = new TTree(Form("analyzedHits_planeZ%d",planeZ), Form("analyzedHits_planeZ%d",planeZ));

        // Variables for branches
        double analysisVertexX, analysisVertexY, timing;
        int analysisClSize, clPlaneID;

        // Create branches
        analysisTree->Branch("planeID", &clPlaneID, "planeID/I");
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
        
        for (int i = 0; i <= nTrackedEntries; i++)
        {
            if (i<nTrackedEntries) trackedTree->GetEntry(i); // last iteration has no new entry
            double COM_x{};
            double COM_y{};
            std::vector<std::pair<int,int>> validHits{};

            //trackVertixes.push_back(std::make_pair(vertexX, vertexY));      
            if(trackID == entry && i < nTrackedEntries) // this assumes that trackIDs are sorted. (which they should be)
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
                        Vec3 pixelPosition = PixelPositionReconstruction(xPos, yPos, cfg);
                        COM_x += pixelPosition.x;
                        COM_y += pixelPosition.y;
                    }
                }
                // TODO: Center of mass not MONTE CARLO in the future or maybe multiple with flags
                if(analysisFlags->clPos == "MC")
                {
                    analysisVertexX = currentX;
                    analysisVertexY = currentY;       
                }
                else if(analysisFlags->clPos == "COM")
                // Center of Mass
                {
                    if(COM_x > 0 && COM_y > 0)
                    {
                        analysisVertexX = COM_x / clSize;
                        analysisVertexY = COM_y / clSize;
                    }
                    else
                    {
                        analysisVertexX = currentX;
                        analysisVertexY = currentY;    
                    }
                }        
                
                //std::cout << "MC_X: " << currentX << "; MC_Y: " << currentY << "; COM_X: " << analysisVertexX << "; COM_Y: " << analysisVertexY << std::endl; 
                analysisClSize = clSize;
                clPlaneID = planeID;
                timing = std::min_element(cluster.begin(), cluster.end(), [](const Hit& a, const Hit& b)
                {
                    return a.t < b.t;
                })->t;

                if(verbose)std::cout << "Saving to tree: " << " X = " << analysisVertexX << " ;Y = " << analysisVertexY << " ;clSize = " <<  clSize << " ;timing = " << timing << std::endl;

                analysisTree->Fill();
                if (i == nTrackedEntries) break; // no need to store last event again after processing it
                // Reset and dont forget this event
                cluster.clear();
                validHits.clear();
                cluster.push_back({pixX, pixY, reconstructedTime});
                currentX = vertexX;
                currentY = vertexY;
            }
            if (verbose && nHits == 1) std::cout << "NEW TRACK: " << " trackID: " << trackID << " ;xPos: " << vertexX << " ;yPos: " << vertexY << " ;LocalTime: " << reconstructedTime << std::endl;
        
        }
        outfile->cd();
        analysisTree->Write();
    }
    outfile->Close();
    auto end = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double, std::milli> elapsed = end - start;

    std::cout << "############################# Clustering stopped after " << elapsed.count() << "ms" << std::endl;
}
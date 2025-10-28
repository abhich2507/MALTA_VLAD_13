
#include "Clustering.hh"

void Clustering(double threshold, int runNumber, std::string saveName = "default")
{
    auto start = std::chrono::high_resolution_clock::now();

    std::string localPath = getVarFromConfig();
    std::string inputPath = localPath +  Form("Results/local_%04d/", runNumber) + saveName + "/";
    
    std::cout << "############################# Clustering started for:" << std::endl;
    std::cout << inputPath << std::endl;

    TFile *trackedFile = TFile::Open((inputPath + "LocalTrackedHitsThr" + std::to_string(int(threshold)) + ".root").c_str(), "READ");

    bool debug = false;

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
    double currentX, currentY;
    int entry = 0;
    for (int i = 0; i < nTrackedEntries; i++)
    {
        trackedTree->GetEntry(i);
        //trackVertixes.push_back(std::make_pair(vertexX, vertexY));      
        if(trackID == entry) 
        {

            clusterCandidate.insert(std::make_pair(pixX, pixY));
            clusterTiming.push_back(reconstructedTime);
            currentX = vertexX;
            currentY = vertexY;
        }
        else if (! clusterCandidate.empty())
        {
            entry++;
            int clSize = 0;
            // Now that we have a set of cluster candidates. Check that they actually are valid
            int j = 0;
            for (const auto& [xPos, yPos] : clusterCandidate) 
            {
                clSize++;
                double hitTiming = clusterTiming[j];
                if(debug)std::cout << "Proposed hit in cluster with: " << " ;xPos: " << xPos << " ;yPos: " << yPos << " ;hitTiming: " << hitTiming << std::endl;
                
                if (xPos == -1 || yPos == -1)
                {
                    clSize--;
                    clusterTiming.erase(clusterTiming.begin() + j);
                }
                
                for (auto& [dx, dy] : diagonals)
                {
                    // Here we check if there are non-adjacent pixel clusters.
                    if (clSize > 1 && clusterCandidate.count(std::make_pair(xPos + dx, yPos +dy)) &&
                        !(clusterCandidate.count(std::make_pair(xPos +dx, yPos)) || clusterCandidate.count(std::make_pair(xPos, yPos + dy) ) ))
                    {
                        clSize--;
                        clusterTiming.erase(clusterTiming.begin() + j);
                    }
                }
                j++;
            }

            analysisVertexX = currentX;
            analysisVertexY = currentY;                
            analysisClSize = clSize;
            timing = *std::min_element(clusterTiming.begin(), clusterTiming.end());

            if(debug)std::cout << "Saving to tree: " << " X = " << analysisVertexX << " ;Y = " << analysisVertexY << " ;clSize = " <<  clSize << " ;timing = " << timing << std::endl;


            analysisTree->Fill();
            // Reset and dont forget this event
            clusterCandidate.clear();
            clusterTiming.clear();
            clusterCandidate.insert(std::make_pair(pixX, pixY));
            clusterTiming.push_back(reconstructedTime);
            currentX = vertexX;
            currentY = vertexY;
        }
        else
        {
            entry++;
            clusterCandidate.clear();
            clusterTiming.clear();
            clusterCandidate.insert(std::make_pair(pixX, pixY));
            clusterTiming.push_back(reconstructedTime);
            currentX = vertexX;
            currentY = vertexY;
        }
        if (debug && nHits == 1) std::cout << "NEW TRACK: " << " trackID: " << trackID << " ;xPos: " << vertexX << " ;yPos: " << vertexY << " ;LocalTime: " << reconstructedTime << std::endl;
        
    }

    /*
    for (int i =0; i< nTrackedEntries; i++)
    {
        int clSize = 0;
        trackedTree->GetEntry(i);
        //std::cout << "TrackID is: " <<trackID << "xPos: " << pixX << " ;yPos: " << pixY << "; LocalTime: " << reconstructedTime << std::endl;
        //std::cout << "currentTrack is: " << currentTrack << std::endl;

        if (trackID == currentTrack)
        {
            // I am still in the same track. This is where we define the cluster
            //std::cout << "Adding event..." << std::endl;

            if (debug) std::cout << "NEW ENTRY STARTED with :"; 
            if (debug) std::cout << "VertexX: " << vertexX << "; VertexY: " << vertexY << std::endl;
            // DEBUG: BUG FOUND where only the largest timing in cluster was saved.
            // TODO: Some code was written but I have a feeling this whole part is rotten. CHECK in the morning thoroughly.
            clusterCandidate.insert(std::make_pair(pixX, pixY));
            clusterTiming.push_back(reconstructedTime);
            realHits = nHits;
        }
        else
        {
            if (debug) std::cout << "-----------------------------------"<< std::endl;
            // We collected all candidates for this track. Now it is time to validate the hits
            int entry = 0;
            for (auto& [xPos,yPos] : clusterCandidate )
            {
                entry++;
                clSize++;
                if (true) std::cout << "xPos: " << xPos << " ;yPos: " << yPos << " ;clSize: " << clSize << " ;realHits: " << realHits << "; LocalTime: " << clusterTiming[entry] << std::endl;
                if (xPos == -1 || yPos == -1)
                {
                    clSize--;
                    clusterTiming.erase(clusterTiming.begin() + entry);
                }
                for (auto& [dx, dy] : diagonals)
                {
                    // Here we check if there are non-adjacent pixel clusters.
                    if (clSize > 1 && clusterCandidate.count(std::make_pair(xPos + dx, yPos +dy)) &&
                        !(clusterCandidate.count(std::make_pair(xPos +dx, yPos)) || clusterCandidate.count(std::make_pair(xPos, yPos + dy) ) ))
                    {
                        if (debug) std::cout << "################################################################################INVALID CLUSTER> DELETED" << std::endl;
                        clSize--;
                        clusterTiming.erase(clusterTiming.begin() + entry);
                    }
                }
            }
            trackedTree->GetEntry(i-1); 
            if (debug) std::cout << "TrackID: " << trackID << "; VertexX: " << vertexX << "; VertexY: " << vertexY << "; GlobalTime: " << globalTrigger << "; PixX: " << pixX << "; PixY: " << pixY << ";nHits: " << clusterCandidate.size() << "; LocalTime: " << reconstructedTime << std::endl;
            if (debug) std::cout <<"Valid cluster size = " << clSize << std::endl;      
            if (debug) std::cout << "-----------------------------------"<< std::endl;
            analysisVertexX = vertexX;
            analysisVertexY = vertexY;
            analysisClSize = clSize;
            timing = *std::min_element(clusterTiming.begin(), clusterTiming.end());//reconstructedTime;
            analysisTree->Fill();
            std::cout << "AAAAAAAAAAAAAAAAAAAAAAAA:" << timing << std::endl;

            // Prepare for the next cluster
            trackedTree->GetEntry(i); 
            if (debug) std::cout << "NEW ENTRY STARTED with :"; 
            if (debug) std::cout << "VertexX: " << vertexX << "; VertexY: " << vertexY << std::endl;
            clusterCandidate.clear();
            clusterTiming.clear();
            clusterCandidate.insert(std::make_pair(pixX, pixY));
            realHits = 1;
        }
        currentTrack = trackID; 
    }
    */



    analysisTree->Write();
    outfile->Close();
    auto end = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double, std::milli> elapsed = end - start;

    std::cout << "############################# Clustering stopped after " << elapsed.count() << "ms" << std::endl;
}
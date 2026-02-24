#include "MALTATracking.hh"
/*
##############################################################################################################################################

Experimental Tracking in order to mimic exactly the MALTA testbeam analysis. Currently doesnt work.

##############################################################################################################################################
*/
void MALTATracking(double threshold, int runNumber, std::string saveName)
{
    bool verbose  = true;
    bool debug = false;

    std::string localPath = getVarFromConfig();
    std::string inputPath = localPath +  Form("Results/local_%04d/", runNumber);
    std::string inputSubPath = localPath +  Form("Results/local_%04d/", runNumber) + saveName + "/";
    std::string directoryPath = localPath + "Plots/";
    std::string runPath = Form("local_%04d/", runNumber);

    DetectorConfig cfg = LoadConfig(localPath + Form("Results/local_%04d/flags.cfg", runNumber));

    std::cout << "############################# Tracking started for:" << std::endl;
    std::cout << inputPath << std::endl;


    double dCut = 500; // in um    

    TChain *trackChain = new TChain("TruthVertex");

    for (int t = 0; t <= 5; ++t) 
    {
        trackChain->Add(Form("%soutput0_t%d.root", inputPath.c_str() , t));
    }

    float vertexX_f, vertexY_f, vertexZ_f, globalTime_f;

    // Connect branches
    trackChain->SetBranchAddress("trueVertexX", &vertexX_f);
    trackChain->SetBranchAddress("trueVertexY", &vertexY_f);
    trackChain->SetBranchAddress("trueVertexZ", &vertexZ_f);
    trackChain->SetBranchAddress("trueGlobalTime", &globalTime_f);

    Long64_t nTrackEntries = trackChain->GetEntries();
    //std::cout << "Number of track entries: " << nTrackEntries << std::endl;
    // Sorting tracks to account for multi thread desync
    std::vector<TrackEntry> tracks;
    for (Long64_t i = 0; i < nTrackEntries; i++) 
    {
        trackChain->GetEntry(i);
        double vertexX = static_cast<double>(vertexX_f);
        double vertexY = static_cast<double>(vertexY_f);
        double vertexZ = static_cast<double>(vertexZ_f);
        double globalTime = static_cast<double>(globalTime_f);
        TrackEntry tr;
        tr.x = vertexX;
        tr.y = vertexY;
        tr.z = vertexZ;
        tr.t = globalTime;
        tracks.push_back(tr);
    }

    // sort by time
    std::sort(tracks.begin(), tracks.end(),
          [](const TrackEntry &a, const TrackEntry &b) {
              return a.t < b.t;
          });
    // Save sorted tracks to a new tree
    TTree *sortedTracks = new TTree("TracksSorted", "Time-sorted tracks");
    int sID;
    double sx, sy, sz, st;
    sortedTracks->Branch("trackID", &sID, "trackID/I");
    sortedTracks->Branch("vertexX", &sx, "vertexX/D");
    sortedTracks->Branch("vertexY", &sy, "vertexY/D");
    sortedTracks->Branch("fGlobalTime", &st, "fGlobalTime/D");
    int trackID = 0;
    for (auto &tr : tracks) 
    {
        sID = trackID;
        trackID++;
        sx = tr.x;
        sy = tr.y;
        st = tr.t;
        sortedTracks->Fill();
    }


    // Next we load the clustered hits.
    TFile *clusteredFile = TFile::Open((inputSubPath + "LocalClusteredHitsThr" + std::to_string(int(threshold)) + ".root").c_str(), "READ");
    // Get tree
    TTree *clusteredTree = (TTree*) clusteredFile->Get("ClusteredHits");

    int clID, hitPixX, hitPixY, hitSize;
    double hitTiming;

    clusteredTree->SetBranchAddress("clusterID", &clID);
    clusteredTree->SetBranchAddress("pixX", &hitPixX);
    clusteredTree->SetBranchAddress("pixY", &hitPixY);
    clusteredTree->SetBranchAddress("clSize", &hitSize);
    clusteredTree->SetBranchAddress("timing", &hitTiming);

    Long64_t nClusterEntries = clusteredTree->GetEntries();
    
    std::map <int, std::vector<hitCluster>> clusterMap;

    // Save to map for easy ID match later on between hit and track
    /*
    for (int i = 0; i< nClusterEntries; i++)
    {
        clusteredTree->GetEntry(i);

        //std::cout << "i: " << i << "; clID: " << clID  << std::endl;

        clusterMap[clID].push_back({hitPixX, hitPixY, hitSize, hitTiming});
        
        //if(verbose) std::cout << "clID: " << clID << "; pixX: " << hitPixX << "; pixY: " << hitPixY << "; clSize: " << hitSize << "; timing: " << hitTiming << std::endl;
    }
    */
   int lastClID = -1;

    for (int i = 0; i < nClusterEntries; i++)
    {
        clusteredTree->GetEntry(i);

        //if(clID >0 && clID != int(hitTiming / 1000))
        /* 
        if (clID == 4774)
        {
            std::cout << "clID: " << clID << "; hitTiming: " << hitTiming << std::endl;
            break;
        }
        */

        // Fill gaps between lastClID and current clID
        for (int missingID = lastClID + 1; missingID < clID; ++missingID) {
            //clusterMap[missingID].push_back({-1, -1, 0, -1}); // creates an empty entry
            std::cout << "⚠️ Missing cluster ID: " << missingID << std::endl;
        }

        // Now push the actual hit
        clusterMap[clID].push_back({hitPixX, hitPixY, hitSize, hitTiming});

        lastClID = clID;
    }




    // Create save file and tree
    TFile *outfile = new TFile((inputSubPath + "analysisHitsThr" + std::to_string(int(threshold)) + ".root").c_str(), "RECREATE");

    // Create a TTree
    TTree *trackedTree = new TTree("analyzedHits", "analyzed Hits");

    // Variables for branches
    double reconstructedVertexX, reconstructedVertexY, reconstructedTiming;
    int reconstructedClSize;

    // Create branches
    trackedTree->Branch("analysisVertexX", &reconstructedVertexX, "analysisVertexX/D");
    trackedTree->Branch("analysisVertexY", &reconstructedVertexY, "analysisVertexY/D");
    trackedTree->Branch("clSize", &reconstructedClSize, "clSize/I");
    trackedTree->Branch("timing", &reconstructedTiming, "timing/D");    



    // minor BUG found: We are consistenly missing the last 3 tracks in the hits. Significant only for very low statistics
    // This is probably why we cant run through Proteus there is a desync at some point between data and tracks.
    for (int i =0; i< nTrackEntries; i++)
    {
        sortedTracks->GetEntry(i);
        
        if(verbose) std::cout << "trackID: " << sID << "; vertexX: " << sx << "; vertexY: " << sy << "; timing: " << st << std::endl;

        auto it = clusterMap.find({sID});
        if (it != clusterMap.end()) 
        {
            const auto &clusterVec = it->second;
            // Populate the tree with the true vertex position
            reconstructedVertexX = sx;
            reconstructedVertexY = sy;
            // Save the leading hit timing
            reconstructedTiming = clusterVec[0].hitTiming; // beacuse they are time ordered
            reconstructedClSize = clusterVec[0].clSize;
            bool isMatched = false;
            for (int j = 0; j < clusterVec.size(); j ++)
            {
                int clPixX = clusterVec[j].pixX;
                int clPixY = clusterVec[j].pixY;
                std::pair<double, double> pixelGlobalPosition = PixelPositionReconstruction(clPixX, clPixY, cfg);

                if (std::abs(pixelGlobalPosition.first  - sx) < dCut / 1000. && std::abs(pixelGlobalPosition.second - sy) < dCut / 1000.)
                //if(true)
                {
                    if(verbose) std::cout << "Matched hit at Pixel (" << clPixX << ", " << clPixY << ") with Global Position (" << pixelGlobalPosition.first << ", " << pixelGlobalPosition.second << ")" << "; Timing:" << reconstructedTiming << std::endl;
                    isMatched = true;
                    break;
                }
            }

            if( !isMatched)
            {
                reconstructedClSize = 0;
            }
        }
        else
        {
            // No cluster found for this track → mark as unmatched
            reconstructedClSize = 0;
            reconstructedTiming = -1; 
        }
        trackedTree->Fill();
    }

    outfile->cd();
    trackedTree->Write();
    outfile->Close();

    
}
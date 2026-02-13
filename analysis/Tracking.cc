#include "Tracking.hh"

// TODO: Move defaults to header only
void Tracking(double threshold, int runNumber, std::string saveName)
{

    auto start = std::chrono::high_resolution_clock::now();
    // Set all the analysis flags for the digital processing
    auto analysisFlags = new SimFlags;
    const char* configPath = std::getenv("ANALYSIS_CONFIG");
    LoadAnalysisFlagsFromFile(configPath, *analysisFlags);
    ////////// Function can be used for custom analysis paths
    //std::string localPath = getVarFromConfig();
    //////////////////////////////////////////////////////////
    std::string localPath = analysisFlags->localPath;
    std::string inputPath = analysisFlags->inputPath+Form("_%04d/", runNumber);
    std::string inputSubPath = analysisFlags->inputPath+Form("_%04d/", runNumber) + saveName + "/";
    std::string directoryPath = localPath + "Plots/";
    std::string runPath = Form("local_%04d/", runNumber);

    DetectorConfig cfg = LoadConfig(inputPath + "flags.cfg");

    std::cout << "############################# Tracking started for:" << std::endl;
    std::cout << inputPath << std::endl;


    bool verbose = analysisFlags->verboseTracking;

    double dCut = analysisFlags->distCut;
    double matchWindow = analysisFlags->timeCut;

    TChain *trackChain = new TChain("TruthVertex");

    for (int t = 0; t <= analysisFlags->numThreads - 1; ++t) 
    {
        trackChain->Add(Form("%soutput0_t%d.root", inputPath.c_str() , t));
    }

    float vertexX_float, vertexY_float, vertexZ_float, globalTime_float;

    // Connect branches
    trackChain->SetBranchAddress("trueVertexX", &vertexX_float);
    trackChain->SetBranchAddress("trueVertexY", &vertexY_float);
    trackChain->SetBranchAddress("trueVertexZ", &vertexZ_float);
    trackChain->SetBranchAddress("trueGlobalTime", &globalTime_float);

    // Old format. DEPRECATED. Will be deleted on 01.03.2026
    /*
    trackChain->SetBranchAddress("vertexX", &vertexX);
    trackChain->SetBranchAddress("vertexY", &vertexY);
    trackChain->SetBranchAddress("vertexZ", &vertexZ);
    trackChain->SetBranchAddress("fGlobalTime", &globalTime);
    */

    Long64_t nTrackEntries = trackChain->GetEntries();
    // Once again multiThreading is a pain the ***. I need to first time order my hits.


    std::vector<TrackEntry> tracks;
    for (Long64_t i = 0; i < nTrackEntries; i++) 
    {
        trackChain->GetEntry(i);

        double vertexX = static_cast<double>(vertexX_float);
        double vertexY = static_cast<double>(vertexY_float);
        double vertexZ = static_cast<double>(vertexZ_float);
        double globalTime = static_cast<double>(globalTime_float);
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

    double sx, sy, sz, st;
    sortedTracks->Branch("vertexX", &sx, "vertexX/D");
    sortedTracks->Branch("vertexY", &sy, "vertexY/D");
    sortedTracks->Branch("vertexZ", &sz, "vertexZ/D");
    sortedTracks->Branch("fGlobalTime", &st, "fGlobalTime/D");

    for (auto &tr : tracks) {
        sx = tr.x;
        sy = tr.y;
        sz = tr.z;
        st = tr.t;
        sortedTracks->Fill();
    }
    //sortedTracks->Write();

    TFile *reconstructedFile = TFile::Open((inputSubPath + "Plane0ReconstructedHitsThr" + std::to_string(int(threshold)) + ".root").c_str(), "READ");


    // Get tree
    TTree *reconstructedTree = (TTree*) reconstructedFile->Get("ReconstructedHits");

    int reconstructedPixX, reconstructedPixY;
    double reconstructedTiming;
    reconstructedTree->SetBranchAddress("PixX", &reconstructedPixX);
    reconstructedTree->SetBranchAddress("PixY", &reconstructedPixY);
    reconstructedTree->SetBranchAddress("timing", &reconstructedTiming);
    Long64_t nReconstructedEntries = reconstructedTree->GetEntries();

    // Save to file

    TFile *outfile = new TFile((inputSubPath + "LocalTrackedHitsThr" + std::to_string(int(threshold)) + ".root").c_str(), "RECREATE");

    // Create a TTree
    TTree *trackedTree = new TTree("TrackedHits", "Tracked Hits");

    // Variables for branches
    double vertexX, vertexY, vertexTime, DUTLocalTime;
    int DUTPixX, DUTPixY, trackID, DUTnHits;

    // Create branches
    trackedTree->Branch("trackID", &trackID, "trackID/I");
    trackedTree->Branch("vertexX", &vertexX, "vertexX/D");
    trackedTree->Branch("vertexY", &vertexY, "vertexY/D");
    trackedTree->Branch("vertexTime", &vertexTime, "vertexTime/D");
    trackedTree->Branch("DUTPixX", &DUTPixX, "DUTPixX/I");
    trackedTree->Branch("DUTPixY", &DUTPixY, "DUTPixY/I");
    trackedTree->Branch("DUTnHits", &DUTnHits, "DUTnHits/I");
    trackedTree->Branch("DUTLocalTime", &DUTLocalTime, "DUTLocalTime/D");


    // Save monitoring plots + save hits in vectors to avoid further repeated ROOT calls
    std::vector<double> vDUTPixX, vDUTPixY, vDUTLocalTiming;
    vDUTPixX.resize(nReconstructedEntries);
    vDUTPixY.resize(nReconstructedEntries);
    vDUTLocalTiming.resize(nReconstructedEntries);
    TH2D *h2DUTHits    = new TH2D("h2DUTHits", "h2DUTHits", 512, 0, 512, 512, 0, 512);
    for (int i = 0; i<nReconstructedEntries; i++)
    {
        reconstructedTree->GetEntry(i);
        h2DUTHits->Fill(reconstructedPixX, reconstructedPixY, 1);

        vDUTPixX[i] = reconstructedPixX;
        vDUTPixY[i] = reconstructedPixY;
        vDUTLocalTiming[i] = reconstructedTiming;
        //std::cout << "i: " << i << " ;reconstructedX" <<reconstructedPixX << " ;reconstructedY: " << reconstructedPixY << std::endl;
    }
    

    savePlot(directoryPath, runPath, threshold, saveName, h2DUTHits, "h2DUTHits");

    TH1D *h1ResidualX = new TH1D("h1ResidualX", "h1ResidualX", 100, -2, 2);
    TH1D *h1ResidualY = new TH1D("h1ResidualY", "h1ResidualY", 100, -2, 2);

    outfile->cd();
    
    // To avoid O(NxN) I will use a sliding window
    Long64_t detIdx = 0; // pointer in detector tree
    bool foundHit;
    
    for (int i =0; i< nTrackEntries; i++)
    {
        sortedTracks->GetEntry(i);
        
        if(verbose) std::cout << "Track Entry: " << i << "; VertexX: " << sx << "; VertexY: " << sy << "; VertexZ: " << sz << "; GlobalTime: " << st << std::endl;
        // Now find matching reconstructed hits
        trackID = i;
        vertexX = sx;
        vertexY = sy;
        vertexTime = st;
        // First we set up the sliding window
        while (detIdx < nReconstructedEntries) 
        {
            if (vDUTLocalTiming[detIdx] >= st) break;
            detIdx++;
        }
        // now check all hits in [t, t+Δt]
        Long64_t j = detIdx;
        foundHit = false;
        DUTnHits = 0;
        while (j < nReconstructedEntries) 
        {
            if (vDUTLocalTiming[j] >= st + matchWindow) break; // left the window
            DUTPixX = vDUTPixX[j];
            DUTPixY = vDUTPixY[j];
            
            DUTLocalTime = vDUTLocalTiming[j] - st;

            // Now do position cut
            std::pair<double, double> pixelGlobalPosition = PixelPositionReconstruction(DUTPixX, DUTPixY, cfg);

            h1ResidualX->Fill(pixelGlobalPosition.first  - vertexX);
            h1ResidualY->Fill(pixelGlobalPosition.second - vertexY);
            if(verbose) std::cout << "(?) Candidate found at Pixel (" << DUTPixX << ", " << DUTPixY << ") with Global Position (" << pixelGlobalPosition.first << ", " << pixelGlobalPosition.second << ")" << "; Timing:" << DUTLocalTime << std::endl;               

            //if ( ( pixelGlobalPosition.first >= reconstructedVertexX - dCut / 1000. && pixelGlobalPosition.first <= reconstructedVertexX + dCut / 1000. ) 
            //&&  ( pixelGlobalPosition.second >= reconstructedVertexY - dCut / 1000. && pixelGlobalPosition.second <= reconstructedVertexY + dCut / 1000. ))
            if(std::abs(pixelGlobalPosition.first - vertexX) <= dCut / 1000 && std::abs(pixelGlobalPosition.second - vertexY) <= dCut / 1000)
            {
                if(verbose) std::cout << "(!) Matched hit at Pixel (" << DUTPixX << ", " << DUTPixY << ") with Global Position (" << pixelGlobalPosition.first << ", " << pixelGlobalPosition.second << ")" << "; Timing:" << DUTLocalTime << std::endl;
                DUTnHits++;
                trackedTree->Fill();   // <-- one Fill per matching hit
                foundHit = true;
            }
            j++;
        }    
        if (!foundHit) 
        {
            // no hit matched: fill with sentinel values
            DUTPixX = -1;
            DUTPixY = -1;
            DUTLocalTime = -1;

            trackedTree->Fill();   // <-- one Fill per track with no hit
        } 
    }
    savePlot(directoryPath, runPath, threshold, saveName, h1ResidualX, "h1ResidualX");
    savePlot(directoryPath, runPath, threshold, saveName, h1ResidualY, "h1ResidualY");
    outfile->cd();
    auto end = std::chrono::high_resolution_clock::now(); 
    std::chrono::duration<double, std::milli> elapsed = end - start;     

    trackedTree->Write();
    outfile->Close();

    std::cout << "############################# Tracking stopped after " << elapsed.count() << " ms" << std::endl;

}
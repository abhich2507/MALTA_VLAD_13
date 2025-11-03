#include "Tracking.hh"

DetectorConfig LoadConfig(const std::string& configPath) {
    std::ifstream infile(configPath);
    std::map<std::string, double> config;
    std::string line;

    while (std::getline(infile, line)) {
        if (line.empty() || line[0] == '#' || line.rfind("//",0) == 0) continue;
        std::istringstream iss(line);
        std::string key, eq;
        double value;
        if (iss >> key >> eq >> value && eq == "=") {
            config[key] = value;
        }
    }

    DetectorConfig dc;
    dc.detectorXOffset = config["detectorXOffset"] * 10;
    dc.detectorYOffset = config["detectorYOffset"] * 10;
    dc.pixelSize       = config["pixelSize"];
    dc.detectorSizeX   = config["detectorSizeX"] * 10;
    dc.detectorSizeY   = config["detectorSizeY"] * 10;
    return dc;
}

inline std::pair<double,double> PixelPositionReconstruction(int pixelX, int pixelY, const DetectorConfig& cfg)
{
    double xGlobal = pixelX * cfg.pixelSize + cfg.detectorXOffset - cfg.detectorSizeX / 2;
    double yGlobal = pixelY * cfg.pixelSize + cfg.detectorYOffset - cfg.detectorSizeY / 2;
    return {xGlobal, yGlobal};
}

void Tracking(double threshold = 1, int runNumber = 91, std::string saveName = "default")
{

    auto start = std::chrono::high_resolution_clock::now();

    std::string localPath = getVarFromConfig();
    std::string inputPath = localPath +  Form("Results/local_%04d/", runNumber);
    std::string inputSubPath = localPath +  Form("Results/local_%04d/", runNumber) + saveName + "/";
    std::string directoryPath = localPath + "Plots/";
    std::string runPath = Form("local_%04d/", runNumber);

    DetectorConfig cfg = LoadConfig(localPath + Form("Results/local_%04d/flags.cfg", runNumber));

    std::cout << "############################# Tracking started for:" << std::endl;
    std::cout << inputPath << std::endl;

    bool debug = false;

    double dCut = 100; // in um
    double matchWindow = 60; // ns
    

    TChain *trackChain = new TChain("TruthVertex");

    for (int t = 0; t <= 5; ++t) 
    {
        trackChain->Add(Form("%soutput0_t%d.root", inputPath.c_str() , t));
    }

    double vertexX, vertexY, vertexZ, globalTime;

    // Connect branches
    trackChain->SetBranchAddress("vertexX", &vertexX);
    trackChain->SetBranchAddress("vertexY", &vertexY);
    trackChain->SetBranchAddress("vertexZ", &vertexZ);
    trackChain->SetBranchAddress("fGlobalTime", &globalTime);

    //TODO: Problem with this tree I add every hit to it. Keep it to only truth.

    Long64_t nTrackEntries = trackChain->GetEntries();
    //std::cout << "Number of track entries: " << nTrackEntries << std::endl;

    // Once again multiThreading is a pain the ***. I need to first time order my hits.


    std::vector<TrackEntry> tracks;
    for (Long64_t i = 0; i < nTrackEntries; i++) 
    {
        trackChain->GetEntry(i);
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
    double reconstructedVertexX, reconstructedVertexY, reconstructedGlobalTime, reconstructedLocalTime;
    int pixX, pixY, trackID, nHits;

    // Create branches
    trackedTree->Branch("trackID", &trackID, "trackID/I");
    trackedTree->Branch("reconstructedVertexX", &reconstructedVertexX, "reconstructedVertexX/D");
    trackedTree->Branch("reconstructedVertexY", &reconstructedVertexY, "reconstructedVertexY/D");
    trackedTree->Branch("reconstructedGlobalTime", &reconstructedGlobalTime, "reconstructedGlobalTime/D");
    trackedTree->Branch("PixX", &pixX, "PixX/I");
    trackedTree->Branch("PixY", &pixY, "PixY/I");
    trackedTree->Branch("nHits", &nHits, "nHits/I");
    trackedTree->Branch("reconstructedLocalTime", &reconstructedLocalTime, "reconstructedLocalTime/D");


    // Save monitoring plots + save hits in vectors to avoid further repeated ROOT calls
    std::vector<double> recoPixX, recoPixY, recoTiming;
    recoPixX.resize(nReconstructedEntries);
    recoPixY.resize(nReconstructedEntries);
    recoTiming.resize(nReconstructedEntries);
    TH2D *h2DUTHits    = new TH2D("h2DUTHits", "h2DUTHits", 512, 0, 512, 512, 0, 512);
    for (int i = 0; i<nReconstructedEntries; i++)
    {
        reconstructedTree->GetEntry(i);
        h2DUTHits->Fill(reconstructedPixX, reconstructedPixY, 1);

        recoPixX[i] = reconstructedPixX;
        recoPixY[i] = reconstructedPixY;
        recoTiming[i] = reconstructedTiming;
        //std::cout << "i: " << i << " ;reconstructedX" <<reconstructedPixX << " ;reconstructedY: " << reconstructedPixY << std::endl;
    }
     // Plot save path
     // TODO: Move me to a function
    

    savePlot(directoryPath, runPath, threshold, saveName, h2DUTHits, "h2DUTHits");



    outfile->cd();
    
    // To avoid O(NxN) I will use a sliding window
    Long64_t detIdx = 0; // pointer in detector tree
    bool foundHit;
    
    for (int i =0; i< nTrackEntries; i++)
    {
        sortedTracks->GetEntry(i);
        // 0.018906 ms
        if(debug) std::cout << "Track Entry: " << i << "; VertexX: " << sx << "; VertexY: " << sy << "; VertexZ: " << sz << "; GlobalTime: " << st << std::endl;
        // Now find matching reconstructed hits
        trackID = i;
        reconstructedVertexX = sx;
        reconstructedVertexY = sy;
        reconstructedGlobalTime = st;
        // First we set up the sliding window
        while (detIdx < nReconstructedEntries) 
        {
            //reconstructedTree->GetEntry(detIdx);
            //if(debug) std::cout << "Particle " << detIdx << " PixX=" <<  recoPixX[detIdx] << " PixY=" << recoPixY[detIdx] << " timing=" << recoTiming[detIdx] << std::endl;

            if (recoTiming[detIdx] >= st) break;
            detIdx++;
        }
        // 0.0622 ms

        // now check all hits in [t, t+Δt]
        Long64_t j = detIdx;
        foundHit = false;
        nHits = 0;
        while (j < nReconstructedEntries) 
        {
            //reconstructedTree->GetEntry(j);
            if (recoTiming[j] >= st + matchWindow) break; // left the window
            //if(debug) std::cout << "Particle " << i << " PixX=" <<  recoPixX[j] << " PixY=" << recoPixY[j] << " timing=" << recoTiming[j] << " st= " << st << std::endl;
            pixX = recoPixX[j];
            pixY = recoPixY[j];
            reconstructedLocalTime = recoTiming[j] - st;

            // Now do position cut
            std::pair<double, double> pixelGlobalPosition = PixelPositionReconstruction(pixX, pixY, cfg);

            if ( ( pixelGlobalPosition.first > reconstructedVertexX - dCut / 1000. && pixelGlobalPosition.first < reconstructedVertexX + dCut / 1000. ) 
            &&  ( pixelGlobalPosition.second > reconstructedVertexY - dCut / 1000. && pixelGlobalPosition.second < reconstructedVertexY + dCut / 1000. ))
            {
                if(debug) std::cout << "Matched hit at Pixel (" << pixX << ", " << pixY << ") with Global Position (" << pixelGlobalPosition.first << ", " << pixelGlobalPosition.second << ")" << "; Timing:" << reconstructedLocalTime << std::endl;
                nHits++;
                trackedTree->Fill();   // <-- one Fill per matching hit
                foundHit = true;
            }
            j++;
        }    
        if (!foundHit) 
        {
            // no hit matched: fill with sentinel values
            pixX = -1;
            pixY = -1;
            reconstructedLocalTime = -1;

            trackedTree->Fill();   // <-- one Fill per track with no hit
        }  
        //34 ms
    }
    

    auto end = std::chrono::high_resolution_clock::now(); 
    std::chrono::duration<double, std::milli> elapsed = end - start;     

    trackedTree->Write();
    outfile->Close();

    std::cout << "############################# Tracking stopped after " << elapsed.count() << " ms" << std::endl;

}
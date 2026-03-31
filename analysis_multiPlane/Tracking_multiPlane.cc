#include "Tracking_multiPlane.hh"

// TODO: Move defaults to header only
void Tracking_multiPlane(double threshold, int runNumber, std::string saveName)
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
    std::string inputPath = analysisFlags->inputPath+Form("_%04d/", runNumber);
    std::string inputSubPath = analysisFlags->inputPath+Form("_%04d/", runNumber) + saveName + "/";
    std::string directoryPath = localPath + "Plots/";
    std::string runPath = Form("local_%04d/", runNumber);
    std::string geometry = analysisFlags->geometry;

    DetectorConfig cfg = LoadConfig(inputPath + "flags.cfg"); // todo: does this need to be generalized?

    std::cout << "############################# Tracking started for:" << std::endl;
    std::cout << inputPath << std::endl;


    bool verbose = analysisFlags->verboseTracking;

    double dCut = analysisFlags->distCut;
    double matchWindow = analysisFlags->timeCut;

    std::string MCTrueTreeName = analysisFlags->MCTrueTree;
    TChain *trackChain = new TChain(MCTrueTreeName.c_str());
    std::string fileName = analysisFlags->fileName;
    for (int t = 0; t <= analysisFlags->numThreads - 1; ++t) 
    {
        trackChain->Add(Form("%s%s_t%d.root", inputPath.c_str(), fileName.c_str(), t));
    }

    int evID;
    float vertexX_float, vertexY_float, vertexZ_float, globalTime_float;

    // Connect branches
    trackChain->SetBranchAddress("trueVertexX", &vertexX_float);
    trackChain->SetBranchAddress("trueVertexY", &vertexY_float);
    trackChain->SetBranchAddress("trueVertexZ", &vertexZ_float);
    trackChain->SetBranchAddress("trueGlobalTime", &globalTime_float);

    Long64_t nTrackEntries = trackChain->GetEntries();
    // For multiThreading I need to first time order the hits.

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
    
    // read in the MALTA hits from tree:
    TFile *reconstructedFile = TFile::Open((inputSubPath + "ReconstructedHitsThr" + std::to_string(int(threshold)) + ".root").c_str(), "READ");

    TTree *reconstructedTree = (TTree*) reconstructedFile->Get("ReconstructedHits");

    int reconstructedPixX, reconstructedPixY, planeID;
    double reconstructedTiming;
    reconstructedTree->SetBranchAddress("planeID", &planeID);
    reconstructedTree->SetBranchAddress("PixX", &reconstructedPixX);
    reconstructedTree->SetBranchAddress("PixY", &reconstructedPixY);
    reconstructedTree->SetBranchAddress("timing", &reconstructedTiming);
    Long64_t nReconstructedEntries = reconstructedTree->GetEntries();

    // Save to file

    TFile *outfile = new TFile((inputSubPath + "LocalTrackedHitsThr" + std::to_string(int(threshold)) + ".root").c_str(), "RECREATE");

    int nPlanes_100 = analysisFlags->nPlanes_100;
    for (int planeZ = 0; planeZ<nPlanes_100; planeZ++)
    //TODO: Not very efficient O(N^2)
    {
        std::cout << "Tracking PlaneZ: " << planeZ << std::endl;
        
        // Create a TTree for each planeZ
        TTree *trackedTree = new TTree(Form("TrackedHits_planeZ%d",planeZ), Form("Tracked Hits in PlaneZ %d",planeZ));

        // Variables for branches
        double vertexX, vertexY, vertexZ, vertexTime, DUTLocalTime;
        int DUTPixX, DUTPixY, trackID, DUTnHits, planeData;

        // Create branches
        trackedTree->Branch("planeID", &planeData, "planeID/I");
        trackedTree->Branch("trackID", &trackID, "trackID/I");
        trackedTree->Branch("vertexX", &vertexX, "vertexX/D");
        trackedTree->Branch("vertexY", &vertexY, "vertexY/D");
        trackedTree->Branch("vertexTime", &vertexTime, "vertexTime/D");
        trackedTree->Branch("DUTPixX", &DUTPixX, "DUTPixX/I");
        trackedTree->Branch("DUTPixY", &DUTPixY, "DUTPixY/I");
        trackedTree->Branch("DUTnHits", &DUTnHits, "DUTnHits/I");
        trackedTree->Branch("DUTLocalTime", &DUTLocalTime, "DUTLocalTime/D");

        // store hits for each z-plane and fill into hitentry that will be time-ordered.
        struct HitEntry {
        double vDUTLocalTiming;
        int vDUTPixX;
        int vDUTPixY;
        int vDUTplaneID;
        };

        std::vector<HitEntry> hits;
        hits.reserve(nReconstructedEntries);  // optional (performance only)

        
        TH2D *h2DUTHits    = new TH2D(Form("h2DUTHits_planeZ%d",planeZ), Form("h2DUTHits_planeZ%d",planeZ), 512, 0, 512, 512, 0, 512);

        for (int i = 0; i < nReconstructedEntries; i++)
        {
            reconstructedTree->GetEntry(i);
            //std::cout << "planeID: " << planeID << "; val: " << planeID%1000000/10000 << "; planeZ: " << planeZ << std::endl;

            if (planeID%1000000/10000!= planeZ) // only store explicit z - layer
                continue;

            hits.push_back({ reconstructedTiming,
                            reconstructedPixX,
                            reconstructedPixY,
                            planeID });
            h2DUTHits->Fill(reconstructedPixX, reconstructedPixY, 1);
        }

        // sort by time. Different x-y planes have mixed up the timing.
        std::sort(hits.begin(), hits.end(),
                [](const HitEntry &a, const HitEntry &b) {
                    return a.vDUTLocalTiming < b.vDUTLocalTiming;
                });

        savePlot(directoryPath, runPath, threshold, saveName, h2DUTHits, Form("h2DUTHits_planeZ%d",planeZ));

        TH1D *h1ResidualX = new TH1D(Form("h1ResidualX_planeZ%d",planeZ), Form("h1ResidualX_planeZ%d",planeZ), 100, -2, 2);
        TH1D *h1ResidualY = new TH1D(Form("h1ResidualY_planeZ%d",planeZ), Form("h1ResidualY_planeZ%d",planeZ), 100, -2, 2);

        outfile->cd();
        
        // To avoid O(NxN) I will use a sliding window
        Long64_t detIdx = 0; // pointer in detector tree
        Long64_t nHits = hits.size();
        bool foundHit;
        auto geoMaps = LoadGeometry(analysisFlags->geoFile, cfg);
        for (int i =0; i< nTrackEntries; i++)
        {
            sortedTracks->GetEntry(i);
            
            if(verbose) std::cout << "Track Entry: " << i << "; VertexX: " << sx << "; VertexY: " << sy << "; VertexZ: " << sz << "; GlobalTime: " << st << std::endl;
            // Now find matching reconstructed hits
            trackID = i;
            vertexX = sx;
            vertexY = sy;
            vertexZ = sz;
            vertexTime = st;
            // First we set up the sliding window
            while (detIdx < nHits && hits[detIdx].vDUTLocalTiming < st) // take first MALTA hit that is at least the vertexTime
                {
                    detIdx++;
                }
            
            // now check all hits in [t, t+Δt]
            Long64_t j = detIdx;
            foundHit = false;
            DUTnHits = 0;
            while (j < nHits && hits[j].vDUTLocalTiming < st + matchWindow) // check whether inside window
            {

                DUTPixX = hits[j].vDUTPixX;
                DUTPixY = hits[j].vDUTPixY;
                // correct pixel coordinates by planeID: // this assumes there is no gap between sensors
                // Assumption no longer correct. TODO 

                
                int filePlane = hits[j].vDUTplaneID;

                //std::cout << "filePlane: " << filePlane  << " hits[j].vDUTPixX: " << hits[j].vDUTPixX 
                //<< " OffsetX: " << geoMaps[filePlane].x<< "OffsetY: " << geoMaps[filePlane].y<< "OffsetZ: " << geoMaps[filePlane].z << std::endl;
                //std::cout << "filePlane: " << filePlane << std::endl;


                //DUTPixX += hits[j].vDUTplaneID%10*512; // shift by 512 per X_planeID = nPlanes_1
                //DUTPixY += (hits[j].vDUTplaneID%100/10)*512; // shift by 512 per Y_planeID = nPlanes_10



                DUTLocalTime = hits[j].vDUTLocalTiming - st;

                // Now do position cut
                Vec3 pixelPosition = PixelPositionReconstruction(DUTPixX, DUTPixY, cfg);

                Vec3 vertex = {vertexX, vertexY, vertexZ};
                //std::cout << vertexX << " ; " << vertexY << " ; " << vertexZ << std::endl;
                Vec3 trackPlaneIntercept = IntersectTrackPlane(vertex, cfg, geoMaps[filePlane]);

                //std::cout << "vx: " <<  vertex.x << "; vy: " << vertex.y << "; vz: " << vertex.z << "; trx: " << trackPlaneIntercept.x 
                //          << "; try: " << trackPlaneIntercept.y << "; trz: " << trackPlaneIntercept.z << std::endl;


                //std::pair<double, double> addPosition = GetSpecificPlaneOffset(hits[j].vDUTplaneID, geometry);
                //pixelGlobalPosition.first += addPosition.first;
                //pixelGlobalPosition.second += addPosition.second;

                //std::cout << "plane: " << filePlane << " x1: " << pixelGlobalPosition.first << " x2: " << pixelGlobalPosition.first + geoMaps[filePlane].x *10 << std::endl;
                // Add corrections to plane position
                auto rotTransPixelPositions = ApplyGeometry3D(pixelPosition, geoMaps[filePlane]);
                //Vec3 trackLocal = ApplyInverseGeometry3D(trackPlaneIntercept, geoMaps[filePlane]);

                double rx = pixelPosition.x - trackPlaneIntercept.x;
                double ry = pixelPosition.y - trackPlaneIntercept.y;

                //std::cout << "x1: " << pixelGlobalPosition.x << "; y1: " << pixelGlobalPosition.y << "; x2: " << rotTransPixelPositions.x << "; y2: " << rotTransPixelPositions.y << std::endl;

                //std::cout << "rx: " << rx << "; ry: " << ry << std::endl;

                //pixelGlobalPosition.x += geoMaps[filePlane].x *10;
                //pixelGlobalPosition.y += geoMaps[filePlane].y *10;
                //std::cout << "xR: " << geoMaps[filePlane].xrot << "; yR: " << geoMaps[filePlane].yrot << std::endl;
                
                planeData = filePlane;

                //h1ResidualX->Fill(rotTransPixelPositions.x - vertexX);
                //h1ResidualY->Fill(rotTransPixelPositions.y - vertexY);
                h1ResidualX->Fill(rx);
                h1ResidualY->Fill(ry);
                //if (true) std::cout << "Vertex X: " << trackPlaneIntercept.x << " VertexY: " << trackPlaneIntercept.y << std::endl;
                //if(true) std::cout << "(?) Candidate found in Plane: " << filePlane  << ", at Pixel (" << DUTPixX << ", " << DUTPixY << ") with Global Position (" << pixelPosition.x << ", " << pixelPosition.y << ")" << "; Timing difference: " << DUTLocalTime << std::endl;               
                //if (true) std::cout << "Residual X: " << rx << " ; Residual Y: " << ry << "; rx^2+ry^2: " << rx*rx +ry*ry << "; dCut^2: " << (dCut/1000)*(dCut/1000) << std::endl;
                //if ( ( pixelGlobalPosition.first >= reconstructedVertexX - dCut / 1000. && pixelGlobalPosition.first <= reconstructedVertexX + dCut / 1000. ) 
                //&&  ( pixelGlobalPosition.second >= reconstructedVertexY - dCut / 1000. && pixelGlobalPosition.second <= reconstructedVertexY + dCut / 1000. ))
                //if(std::abs(rotTransPixelPositions.x - vertexX) <= dCut / 1000 && std::abs(rotTransPixelPositions.y - vertexY) <= dCut / 1000)
                // Change to radial cut instead of axis based.
                if(rx*rx + ry*ry <= (dCut/1000)*(dCut/1000))
                
                {
                    //if(true) std::cout << "(!) Matched hit at Pixel (" << DUTPixX << ", " << DUTPixY << ") with Global Position (" << pixelPosition.x << ", " << pixelPosition.y << ")" << "; Timing difference: " << DUTLocalTime << std::endl;
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
        savePlot(directoryPath, runPath, threshold, saveName, h1ResidualX, Form("h1ResidualX_planeZ%d",planeZ));
        savePlot(directoryPath, runPath, threshold, saveName, h1ResidualY, Form("h1ResidualY_planeZ%d",planeZ));
        outfile->cd();  

        trackedTree->Write();
    }
    outfile->Close();

    auto end = std::chrono::high_resolution_clock::now(); 
    std::chrono::duration<double, std::milli> elapsed = end - start;   

    std::cout << "############################# Tracking stopped after " << elapsed.count() << " ms" << std::endl;

}
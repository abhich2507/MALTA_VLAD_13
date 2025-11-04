#include "SimulationToProteus.hh"

void SimulationToProteus(int runNumber = 96)
{
    std::string inputPath = Form("/home/vlad/Documents/Simu/Geant4/DECAL_REPO/Results/local_%04d/", runNumber);

    TChain *trackChain = new TChain("TruthVertex");

    for (int t = 0; t <= 5; ++t) 
    {
        trackChain->Add(Form("%soutput0_t%d.root", inputPath.c_str() , t));
    }

    double vertexX, vertexY, vertexZ, globalTime;

    // Connect branches
    trackChain->SetBranchAddress("trueVertexX", &vertexX);
    trackChain->SetBranchAddress("trueVertexY", &vertexY);
    trackChain->SetBranchAddress("trueVertexZ", &vertexZ);
    trackChain->SetBranchAddress("trueGlobalTime", &globalTime);


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


    for (int i = 0; i<7; i++)
    {
        // Loop over planes
    


        TFile *reconstructedFile = TFile::Open((inputPath + "Plane" + std::to_string(i) + "ReconstructedHitsThr2000.root").c_str(), "READ");


        // Get tree
        TTree *reconstructedTree = (TTree*) reconstructedFile->Get("ReconstructedHits");

        int reconstructedPixX, reconstructedPixY, nHits;
        double reconstructedTiming;
        reconstructedTree->SetBranchAddress("PixX", &reconstructedPixX);
        reconstructedTree->SetBranchAddress("PixY", &reconstructedPixY);
        reconstructedTree->SetBranchAddress("timing", &reconstructedTiming);
        reconstructedTree->SetBranchAddress("NHits", &nHits);
        Long64_t nReconstructedEntries = reconstructedTree->GetEntries();

        // Save to file

        TFile *outfile = new TFile((inputPath + "Plane" +std::to_string(i) +  "trackedHits.root").c_str(), "RECREATE");

        // Create a TTree
        TTree *trackedTree = new TTree("Hits", "Tracked Hits");

        // Variables for branches
        double reconstructedVertexX, reconstructedVertexY, reconstructedGlobalTime, reconstructedLocalTime, Value;
        int pixX, pixY, NHits, hitInCluster;

        // Create branches
        trackedTree->Branch("runNumber", &runNumber, "runNumber/I");
        trackedTree->Branch("NHits", &NHits, "NHits/I");
        trackedTree->Branch("Value", &Value, "Value/D");
        trackedTree->Branch("PixX", &pixX, "PixX/I");
        trackedTree->Branch("PixY", &pixY, "PixY/I");
        trackedTree->Branch("Timing", &reconstructedLocalTime, "Timing/D");
        trackedTree->Branch("HitInCluster", &hitInCluster, "HitInCluster/I");


        TTree *eventsTree = new TTree("Events", "Event Info");
        int frameNumber, timeStamp, triggerTime, triggerInfo, triggerOffset, triggerL1ID, triggerBCID, invalid;
        // Most of these entries are bullshit from MALTA
        eventsTree->Branch("FrameNumber", &frameNumber, "FrameNumber/I");
        eventsTree->Branch("TimeStamp", &timeStamp, "TimeStamp/I");
        eventsTree->Branch("TriggerTime", &triggerTime, "TriggerTime/I");
        eventsTree->Branch("TriggerInfo", &triggerInfo, "TriggerInfo/I");
        eventsTree->Branch("TriggerOffset", &triggerOffset, "TriggerOffset/I");
        eventsTree->Branch("TriggerL1ID", &triggerL1ID, "TriggerL1ID/I");
        eventsTree->Branch("TriggerBCID", &triggerBCID, "TriggerBCID/I");
        eventsTree->Branch("Invalid", &invalid, "Invalid/I");




        double matchWindow = 200; // ns
        // To avoid O(NxN) I will use a sliding window
        Long64_t detIdx = 0; // pointer in detector tree
        bool foundHit;
        
        for (int i =0; i< nTrackEntries; i++)
        {
            sortedTracks->GetEntry(i);
            //std::cout << "Track Entry: " << i << "; VertexX: " << sx << "; VertexY: " << sy << "; VertexZ: " << sz << "; GlobalTime: " << st << std::endl;
            // Now find matching reconstructed hits
            reconstructedVertexX = sx;
            reconstructedVertexY = sy;
            reconstructedGlobalTime = st;

            frameNumber = i;
            timeStamp = 0;
            triggerTime = 0;
            triggerInfo = -1;
            triggerOffset = -1;
            triggerL1ID = i;
            triggerBCID = 0;
            invalid = 0;
            eventsTree->Fill();

            // First we set up the sliding window
            while (detIdx < nReconstructedEntries) 
            {
                reconstructedTree->GetEntry(detIdx);
                if (reconstructedTiming > st) break;
                detIdx++;
            }

            // now check all hits in [t, t+Δt]
            Long64_t j = detIdx;
            foundHit = false;
            while (j < nReconstructedEntries) 
            {
                reconstructedTree->GetEntry(j);
                if (reconstructedTiming > st + matchWindow) break; // left the window
                //std::cout << "Particle " << i << " PixX=" <<  reconstructedPixX << " PixY=" << reconstructedPixY << " timing=" << reconstructedTiming << std::endl;
                pixX = reconstructedPixX;
                pixY = reconstructedPixY;
                NHits = nHits;
                Value = 1.0; // Placeholder for now
                hitInCluster = 1; // Placeholder for now
                reconstructedLocalTime = reconstructedTiming - st;

                trackedTree->Fill();   // <-- one Fill per matching hit
                foundHit = true;
                j++;
            }
        }

        trackedTree->Write();
        eventsTree->Write();
        outfile->Close();
    }
    // Lastly we merge all files. 

    TFile* out=new TFile((inputPath + "Merged.root").c_str(),"RECREATE");
    // Reshuffle plane names to match MALTA mapping. DUT plane 6/7
    TFile* tel0  =new TFile((inputPath + "Plane1trackedHits.root").c_str(),"READ");
    TFile* tel1  =new TFile((inputPath + "Plane2trackedHits.root").c_str(),"READ");
    TFile* tel2  =new TFile((inputPath + "Plane3trackedHits.root").c_str(),"READ");
    TFile* tel3  =new TFile((inputPath + "Plane4trackedHits.root").c_str(),"READ");
    TFile* tel4  =new TFile((inputPath + "Plane5trackedHits.root").c_str(),"READ");
    TFile* tel5  =new TFile((inputPath + "Plane6trackedHits.root").c_str(),"READ");
    TFile* tel6  =new TFile((inputPath + "Plane0trackedHits.root").c_str(),"READ");

    TTree* mEvent=(TTree*)tel0->Get("Events");
    TTree* mTree0=(TTree*)tel0->Get("Hits");
    TTree* mTree1=(TTree*)tel1->Get("Hits");
    TTree* mTree2=(TTree*)tel2->Get("Hits");
    TTree* mTree3=(TTree*)tel3->Get("Hits");
    TTree* mTree4=(TTree*)tel4->Get("Hits");
    TTree* mTree5=(TTree*)tel5->Get("Hits");
    TTree* mTree6=(TTree*)tel6->Get("Hits"); 

    out->cd();
    TDirectory *out0= out->mkdir("Plane0");
    TDirectory *out1= out->mkdir("Plane1");
    TDirectory *out2= out->mkdir("Plane2");
    TDirectory *out3= out->mkdir("Plane3");
    TDirectory *out4= out->mkdir("Plane4");
    TDirectory *out5= out->mkdir("Plane5");
    TDirectory *out6= out->mkdir("Plane6");

    ULong64_t allTel   =mEvent->GetEntries();
    ULong64_t allMALTA0=mTree0->GetEntries();
    ULong64_t allMALTA1=mTree1->GetEntries();
    ULong64_t allMALTA2=mTree2->GetEntries();
    ULong64_t allMALTA3=mTree3->GetEntries();
    ULong64_t allMALTA4=mTree4->GetEntries();
    ULong64_t allMALTA5=mTree5->GetEntries();
    ULong64_t allMALTA6=mTree6->GetEntries();

    ULong64_t allMin  = allTel;
    if (allMALTA0<=allMin) allMin = allMALTA0;
    if (allMALTA1<=allMin) allMin = allMALTA1;
    if (allMALTA2<=allMin) allMin = allMALTA2;
    if (allMALTA3<=allMin) allMin = allMALTA3;
    if (allMALTA4<=allMin) allMin = allMALTA4;
    if (allMALTA5<=allMin) allMin = allMALTA5;
    if (allMALTA6<=allMin) allMin = allMALTA6;


    TTree* NEvent=mEvent->CloneTree(allMin);
    out0->cd();
    TTree* N0=mTree0->CloneTree(allMin);
    out1->cd();
    TTree* N1=mTree1->CloneTree(allMin);
    out2->cd();
    TTree* N2=mTree2->CloneTree(allMin);
    out3->cd();
    TTree* N3=mTree3->CloneTree(allMin);
    out4->cd();
    TTree* N4=mTree4->CloneTree(allMin);
    out5->cd();
    TTree* N5=mTree5->CloneTree(allMin);
    out6->cd();
    TTree* N6=mTree6->CloneTree(allMin);

    out->Write();
    out->Close();

    tel0->Close();
    tel1->Close();
    tel2->Close();
    tel3->Close();
    tel4->Close();
    tel5->Close();
    tel6->Close();



}
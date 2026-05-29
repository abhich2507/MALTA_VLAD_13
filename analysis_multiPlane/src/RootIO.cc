#include "RootIO.hh"
#include "Utils.hh"
#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>
#include "TChain.h"
#include <TCanvas.h>
#include <TH2.h>
#include <TROOT.h>

void FillReconstructedTree(std::vector<ProcessedHit> allProcessHits, double inputThreshold, int runNumber, std::string saveName, AnaFlags cfg)
{
    std::string inputPath = cfg.inputPath+Form("_%04d/", runNumber);
    mkdir((inputPath + saveName).c_str(), 0777);   
    TFile *outfile = new TFile((inputPath + saveName + "/ReconstructedHitsThr" + std::to_string(int(inputThreshold)) + ".root").c_str(), "RECREATE");
    outfile->cd();
    // Create a TTree
    TTree *reconstructedTree = new TTree("ReconstructedHits", "Reconstructed Hits");
    reconstructedTree->SetDirectory(nullptr);
    // Variables for branches
    int reconstructedPixX, reconstructedPixY, nHits, planeID;
    double reconstructedTiming;
    // Create branches
    reconstructedTree->Branch("planeID", &planeID, "planeID/I");
    reconstructedTree->Branch("PixX", &reconstructedPixX, "PixX/I");
    reconstructedTree->Branch("PixY", &reconstructedPixY, "PixY/I");
    reconstructedTree->Branch("timing", &reconstructedTiming, "timing/D");
    reconstructedTree->Branch("NHits", &nHits, "NHits/I");

    for (const auto& el: allProcessHits)
    {
        planeID = el.planeID;
        reconstructedPixX = el.x;
        reconstructedPixY = el.y;
        reconstructedTiming = el.time;
        nHits = el.nHit;
        reconstructedTree->Fill();
    }
    outfile->cd();
    reconstructedTree->Write("", TObject::kOverwrite);
    outfile->Close();    
}
TFile* CreateTrackedTree(double inputThreshold, int runNumber, std::string saveName, AnaFlags cfg)
{
    std::string inputSubPath = cfg.inputPath+Form("_%04d/", runNumber) + saveName + "/";
    TFile *outfile = new TFile((inputSubPath + "LocalTrackedHitsThr" + std::to_string(int(inputThreshold)) + ".root").c_str(), "RECREATE");

    return outfile;
}
void FillTrackedTree(std::vector<FullTrackInfo> trackedHits, TFile* outfile, int planeZ)
{
    outfile->cd();

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

    for (const auto& el : trackedHits)
    {
        planeData = el.planeID;
        trackID = el.trackID;
        vertexX = el.vertexX;
        vertexY = el.vertexY;
        vertexTime = el.vertexTime;
        DUTPixX = el.dutX;
        DUTPixY = el.dutY;
        DUTnHits = el.dutNHits;
        DUTLocalTime = el.dutTime;
        trackedTree->Fill();
    }
    outfile->cd();  
    trackedTree->Write();
}
void CloseFile(TFile* outfile)
{
    outfile->Close();
}
void SaveResidualHisto(std::vector<Residual> residuals, int planeZ, double threshold, int runNumber, std::string saveName, AnaFlags cfg)
{
    std::string directoryPath = cfg.localPath + "Plots/";
    std::string runPath = Form("local_%04d/", runNumber);
    TH1D *h1ResidualX = new TH1D(Form("h1ResidualX_planeZ%d",planeZ), Form("h1ResidualX_planeZ%d",planeZ), 100, -2, 2);
    TH1D *h1ResidualY = new TH1D(Form("h1ResidualY_planeZ%d",planeZ), Form("h1ResidualY_planeZ%d",planeZ), 100, -2, 2);

    for (const auto& el : residuals)
    {
        h1ResidualX->Fill(el.rx);
        h1ResidualY->Fill(el.ry);
    }


    savePlot(directoryPath, runPath, threshold, saveName, h1ResidualX, Form("h1ResidualX_planeZ%d",planeZ));
    savePlot(directoryPath, runPath, threshold, saveName, h1ResidualY, Form("h1ResidualY_planeZ%d",planeZ));
}
void FillTrackedTree(std::vector<ClusteredHit> allClusters, TFile* outfile, int planeZ)
{
    outfile->cd();
    TTree *analysisTree = new TTree(Form("analyzedHits_planeZ%d",planeZ), Form("analyzedHits_planeZ%d",planeZ));
    // Variables for branches
    double analysisVertexX, analysisVertexY, timing, correctedTiming;
    int analysisClSize, clPlaneID;
    // Create branches
    analysisTree->Branch("planeID", &clPlaneID, "planeID/I");
    analysisTree->Branch("analysisVertexX", &analysisVertexX, "analysisVertexX/D");
    analysisTree->Branch("analysisVertexY", &analysisVertexY, "analysisVertexY/D");
    analysisTree->Branch("clSize", &analysisClSize, "clSize/I");
    analysisTree->Branch("timing", &timing, "timing/D");
    analysisTree->Branch("correctedTiming", &correctedTiming, "correctedTiming/D");

    for (const auto& el : allClusters)
    {
        clPlaneID = el.planeID;
        analysisVertexX = el.x;
        analysisVertexY = el.y;
        analysisClSize = el.clSize;
        timing = el.timing;
        correctedTiming = el.corrTiming;
        analysisTree->Fill();
    }
    outfile->cd();  
    analysisTree->Write();
}
void SaveHistograms(std::vector<TH2D*> histograms2D, std::vector<TH1D*> histograms1D, TH2D* auxiliaryHisto, AnaFlags cfg, double threshold, int runNumber, std::string saveName)
{

    // Plot save path
    std::string directoryPath = cfg.localPath +"/Plots/";
    std::string runPath = Form("local_%04d/", runNumber);

    // Write histograms into the directory
    savePlot(directoryPath, runPath, threshold, saveName, histograms2D[kPASS], histograms2D[kPASS]->GetName());
    savePlot(directoryPath, runPath, threshold, saveName, histograms2D[kClSize], histograms2D[kClSize]->GetName());
    savePlot(directoryPath, runPath, threshold, saveName, histograms2D[kTiming], histograms2D[kTiming]->GetName());
    savePlot(directoryPath, runPath, threshold, saveName, histograms1D[kTiming1D], "h1Timing");
    savePlot(directoryPath, runPath, threshold, saveName, histograms1D[kCorrectedTiming], "h1CorrectedTiming");
    savePlot(directoryPath, runPath, threshold, saveName, histograms2D[kPASSInPixel], histograms2D[kPASSInPixel]->GetName());
    savePlot(directoryPath, runPath, threshold, saveName, histograms2D[kClSizeInPixel], histograms2D[kClSizeInPixel]->GetName());
    savePlot(directoryPath, runPath, threshold, saveName, histograms2D[kTimingInPixel], histograms2D[kTimingInPixel]->GetName());
    savePlot(directoryPath, runPath, threshold, saveName, auxiliaryHisto, auxiliaryHisto->GetName());
    savePlot(directoryPath, runPath, threshold, saveName, histograms2D[kALLInPixel], histograms2D[kALLInPixel]->GetName());
    savePlot(directoryPath, runPath, threshold, saveName, histograms1D[kPASSInPixelXProj], histograms1D[kPASSInPixelXProj]->GetName());
    savePlot(directoryPath, runPath, threshold, saveName, histograms1D[kPASSInPixelYProj], histograms1D[kPASSInPixelYProj]->GetName());
}
void SaveSummaryRoot(AnaFlags cfg, int runNumber, std::string saveName, double threshold, AnalyzedHit statistics)
{
    std::string directoryPath = cfg.localPath +"/Plots/";
    std::string runPath = Form("local_%04d/", runNumber);

    // Lastly populate a root tree with the average values for later summary plotting
    // Try opening the file in UPDATE mode (read + write)
    std::string summaryPath = (directoryPath + runPath + saveName + "/summary.root").c_str();
    double summaryThreshold, summaryEff, summaryEffErr, summaryClSize, summaryClSizeErr, summaryTiming;

    TFile *f = TFile::Open(summaryPath.c_str(), "UPDATE");
    if (!f || f->IsZombie()) {
        std::cout << "Creating new file: " << summaryPath << std::endl;
        f = new TFile(summaryPath.c_str(), "RECREATE");
    }
    // Check if the tree exists
    TTree *summaryTree = (TTree*) f->Get("summaryTree");
    if (!summaryTree) 
    {
        std::cout << "Creating new summary tree" << std::endl;
        summaryTree = new TTree("summaryTree", "summary Tree");
        summaryTree->Branch("threshold", &summaryThreshold, "threshold/D");
        summaryTree->Branch("efficiency", &summaryEff, "efficiency/D");
        summaryTree->Branch("effError", &summaryEffErr, "effError/D");
        summaryTree->Branch("clSize", &summaryClSize, "clSize/D");
        summaryTree->Branch("clSizeError", &summaryClSizeErr, "clSizeError/D");
        summaryTree->Branch("timing", &summaryTiming, "timing/D");
    } 
    else 
    {
        std::cout << "Appending to existing tree" << std::endl;
    }

    summaryTree->SetBranchAddress("threshold", &summaryThreshold);
    summaryTree->SetBranchAddress("efficiency",  &summaryEff);
    summaryTree->SetBranchAddress("effError",  &summaryEffErr);
    summaryTree->SetBranchAddress("clSize", &summaryClSize);
    summaryTree->SetBranchAddress("clSizeError", &summaryClSizeErr);
    summaryTree->SetBranchAddress("timing", &summaryTiming);

    summaryThreshold = threshold;
    summaryEff = statistics.avgEff;
    summaryEffErr = statistics.errEff;
    summaryClSize = statistics.avgClSize;
    summaryClSizeErr = statistics.errClSize;
    summaryTiming = statistics.avgTiming;
    summaryTree->Fill();

    std::cout << "Saving values: " << "Threshold: " << threshold << "; avgEff: " << summaryEff << "; summaryEffErr: " 
              << summaryEffErr << "; avgClSize: " << summaryClSize << ";avgTiming: " << summaryTiming << std::endl;

    f->cd();
    summaryTree->Write("", TObject::kOverwrite); // overwrite tree object in directory
    f->Close();
    delete f;
}
void SaveCalorimetryHistograms(RawCalorimetryPerMap rawCaloMap, std::vector<FitCalorimetryInfo> fitCaloMap, int runNumber, std::string saveName, double threshold)
{
    gROOT->SetBatch(kTRUE);
    TFile *outHistFile = new TFile(Form("Plots/local_%04d/%s/CaloHistos.root",runNumber, saveName.c_str()), "RECREATE");
    // Secondaries Histo
    auto secondariesPerEvent = rawCaloMap.secPerEvent;
    auto secondariesPerPlane = rawCaloMap.secPerPlane;
    auto clustersPerPlane = rawCaloMap.clPerPlane;
    auto xPosPerPlane = rawCaloMap.xPosPerPlane;
    auto yPosPerPlane = rawCaloMap.yPosPerPlane;


    auto [minIt, maxIt] = minMaxByProjection(secondariesPerEvent, [](const auto& v) { return v; });
    double minSecSum = minIt->second;
    double maxSecSum = maxIt->second;
    
    TH1D* hSecSum = new TH1D(Form("hSecSum_run%d_file%s_thr%d", runNumber, saveName.c_str(), (int)threshold), Form(";Secondaries;Counts"), 50, minSecSum, maxSecSum);

    for (const auto& [pid, secSum] : secondariesPerEvent)
    {
        hSecSum->Fill(secSum);
    }

    auto [minTmax, maxTmax]     = fieldRange(fitCaloMap, [](const auto& e) { return e.tmax; });
    auto [minEnergy, maxEnergy] = fieldRange(fitCaloMap, [](const auto& e) { return (float)e.energy; });
    auto [minChi2ndf, maxChi2ndf] = fieldRange(fitCaloMap, [](const auto& e) { return (float)e.chi2ndf; });
    auto [minP0, maxP0] = fieldRange(fitCaloMap, [](const auto& e) { return (float)e.p0; });
    auto [minP1, maxP1] = fieldRange(fitCaloMap, [](const auto& e) { return (float)e.p1; });
    auto [minP2, maxP2] = fieldRange(fitCaloMap, [](const auto& e) { return (float)e.p2; });
    TH1D* hTmax = new TH1D("hTmax", "tmax", 100, minTmax, maxTmax);
    TH1D* hEnergyInt = new TH1D("hEnergyInt", "energy integral", 100, minEnergy, maxEnergy);
    TH1D* hChi2ndf = new TH1D("hChi2ndf", "chi2ndf", 100, minChi2ndf, maxChi2ndf);
    TH1D* hP0 = new TH1D("hP0", "p0", 100, minP0, maxP0);
    TH1D* hP1 = new TH1D("hP1", "p1", 100, minP1, maxP1);
    TH1D* hP2 = new TH1D("hP2", "p2", 100, minP2, maxP2);

    for (int i = 0; i < fitCaloMap.size(); i++)
    {
        auto fitCaloInfo = fitCaloMap[i]; 
        hTmax->Fill(fitCaloInfo.tmax);
        hEnergyInt->Fill(fitCaloInfo.energy);
        hChi2ndf->Fill(fitCaloInfo.chi2ndf);
        hP0->Fill(fitCaloInfo.p0);
        hP1->Fill(fitCaloInfo.p1);
        hP2->Fill(fitCaloInfo.p2);
    }
    outHistFile->cd();
    hSecSum->Write();
    hTmax->Write();
    hEnergyInt->Write();
    hChi2ndf->Write();
    hP0->Write();
    hP1->Write();
    hP2->Write();

    for (const auto& [pid, secVec] : secondariesPerPlane)
    {
        const auto& cluVec = clustersPerPlane[pid];
        const auto& xPosVec = xPosPerPlane[pid];
        const auto& yPosVec = yPosPerPlane[pid];
        if (secVec.empty()) continue;
        // Secondaries Histo
        double minSec = *std::min_element(secVec.begin(), secVec.end());
        double maxSec = *std::max_element(secVec.begin(), secVec.end());
        TH1D* hSec = new TH1D(
            Form("hSec_plane%d_run%d_file%s_thr%d", pid, runNumber, saveName.c_str(), (int)threshold),
            Form("Plane %d;Secondaries;Counts", pid),
            50, minSec, maxSec
        );

        for (auto v : secVec) hSec->Fill(v);
        if (cluVec.empty()) continue;

        // Cluster Histo
        double minClu = *std::min_element(cluVec.begin(), cluVec.end());
        double maxClu = *std::max_element(cluVec.begin(), cluVec.end());

        TH1D* hClu = new TH1D(
            Form("hClu_plane%d_run%d_file%s_thr%d", pid, runNumber, saveName.c_str(), (int)threshold),
            Form("Plane %d;Clusters;Counts", pid),
            50, minClu, maxClu
        );

        for (auto v : cluVec) hClu->Fill(v);

        if (xPosVec.empty()) continue;

        // X position Histo
        double minX = *std::min_element(xPosVec.begin(), xPosVec.end());
        double maxX = *std::max_element(xPosVec.begin(), xPosVec.end());
        if (minX == maxX) { minX -= 0.5; maxX += 0.5; }

        TH1D* hX = new TH1D(
            Form("hX_plane%d_run%d_file%s_thr%d", pid, runNumber, saveName.c_str(), (int)threshold),
            Form("Plane %d;X position;Counts", pid),
            50, minX, maxX
        );

        for (auto v : xPosVec) hX->Fill(v);

        if (yPosVec.empty()) continue;

        // Y position Histo
        double minY = *std::min_element(yPosVec.begin(), yPosVec.end());
        double maxY = *std::max_element(yPosVec.begin(), yPosVec.end());
        if (minY == maxY) { minY -= 0.5; maxY += 0.5; }

        TH1D* hY = new TH1D(
            Form("hY_plane%d_run%d_file%s_thr%d", pid, runNumber, saveName.c_str(), (int)threshold),
            Form("Plane %d;Y position;Counts", pid),
            50, minY, maxY
        );

        for (auto v : yPosVec) hY->Fill(v);

        // Save all to root file instead
        outHistFile->cd();
        hSec->Write();
        hClu->Write();
        hX->Write();
        hY->Write();
    }
    gDirectory->Delete("*");
    outHistFile->Write();
    outHistFile->Close();
}
void FillMALTA3HWCTree(std::pair<std::vector<ProcessedHit>, std::vector<ProcessedHit>> malta3Hits, AnaFlags cfg, double threshold, int runNumber, std::string saveName)
{
    auto hitTree = malta3Hits.first;
    auto positionTree = malta3Hits.second;

    std::string inputPath = cfg.inputPath + Form("_%04d/", runNumber);
    mkdir((inputPath + saveName).c_str(), 0777);
    TFile *outfile = new TFile((inputPath + saveName + "/Plane0ReconstructedHitsThr" + std::to_string(int(threshold)) + ".root").c_str(), "RECREATE");
    outfile->cd();
    // Create a TTree
    TTree *reconstructedTree = new TTree("ReconstructedHits", "Reconstructed Hits");
    // Variables for branches
    int reconstructedPixX, reconstructedPixY, nHits, planeVal;
    double reconstructedTiming;
    // Create branches
    reconstructedTree->Branch("planeID", &planeVal, "planeID/I");
    reconstructedTree->Branch("PixX", &reconstructedPixX, "PixX/I");
    reconstructedTree->Branch("PixY", &reconstructedPixY, "PixY/I");
    reconstructedTree->Branch("timing", &reconstructedTiming, "timing/D");
    reconstructedTree->Branch("NHits", &nHits, "NHits/I");
    // Create a TTree
    TTree *posTree = new TTree("PositionHits", "Position Hits");
    int stripSaveX, stripSaveY;
    double timingSave;
    // Create branches
    posTree->Branch("planeID", &planeVal, "planeID/I");
    posTree->Branch("stripX",   &stripSaveX, "stripX/I");
    posTree->Branch("stripY",   &stripSaveY, "stripY/I");
    posTree->Branch("timing",  &timingSave, "timing/D");

    for(const auto& el : hitTree)
    {
        planeVal = el.planeID;
        reconstructedPixX = el.x;
        reconstructedPixY = el.y;
        reconstructedTiming = el.time;
        nHits = el.nHit;
        reconstructedTree->Fill();
    }

    for(const auto& el: positionTree)
    {
        planeVal = el.planeID;
        stripSaveX = el.x;
        stripSaveY = el.y;
        timingSave = el.time;
        posTree->Fill();
    }
    outfile->cd();
    reconstructedTree->Write();
    posTree->Write();
    outfile->Close();
}
void savePlot (std::string directoryPath, std::string runPath, double threshold, std::string saveName, TH1* hist, std::string histName)
{
    set_style();
    TCanvas *c2D = new TCanvas(histName.c_str(), histName.c_str(), 800, 800);
    std::string filePath = directoryPath + runPath + saveName + "/histos.root";
    //std::cout << "Saving plot to: " << filePath << std::endl;

    mkdir(directoryPath.c_str(), 0777);
    mkdir((directoryPath + runPath).c_str(), 0777);
    mkdir((directoryPath + runPath + saveName).c_str(), 0777);
    
    TFile *histosFile = TFile::Open(filePath.c_str(), "UPDATE"); // create if missing
    if (!histosFile || histosFile->IsZombie()) {
        histosFile = TFile::Open(filePath.c_str(), "RECREATE");
    }

    std::string thrDirName = Form("Thr%i", int(threshold));
    TDirectory *thrDir = histosFile->GetDirectory(thrDirName.c_str());
    if (!thrDir) {
        thrDir = histosFile->mkdir(thrDirName.c_str());
    }
    thrDir->cd();

    hist->Write(histName.c_str(), TObject::kOverwrite);
    thrDir->Close();
    histosFile->cd();  
    histosFile->Close();
    delete histosFile;
}
std::vector<RawHit> GetRawHits(TTree* plane)
{
    int eventID, planeID, pixX, pixY;
    float time, energy;

    plane->SetBranchAddress("iEvent", &eventID);
    plane->SetBranchAddress("iPlane", &planeID);
    plane->SetBranchAddress("PixX", &pixX);
    plane->SetBranchAddress("PixY", &pixY);
    plane->SetBranchAddress("hitTime", &time);
    plane->SetBranchAddress("hitEnergy", &energy);
    Long64_t nRawEntries = plane->GetEntries();

    std::vector<RawHit> output;
    output.reserve(nRawEntries);

    for (Long64_t j = 0; j < nRawEntries; j++)
    {
        plane->GetEntry(j);
        output.push_back(RawHit{HitKey{planeID, eventID, pixX, pixY}, static_cast<double>(energy), static_cast<double>(time)});
    }
    return output;
}
std::vector<TrackEntry> GetVertex(AnaFlags cfg, int runNumber)
{
    std::vector<TrackEntry> tracks;
    std::string inputPath = cfg.inputPath+Form("_%04d/", runNumber);
    TChain *trackChain = new TChain("TruthVertex");
    std::string fileName = cfg.fileName;
    for (int t = 0; t <= cfg.numThreads - 1; ++t) 
    {
        trackChain->Add(Form("%s%s_t%d.root", inputPath.c_str(), fileName.c_str(), t));
    }

    float vertexX_float, vertexY_float, vertexZ_float, globalTime_float;
    // Connect branches
    trackChain->SetBranchAddress("trueVertexX", &vertexX_float);
    trackChain->SetBranchAddress("trueVertexY", &vertexY_float);
    trackChain->SetBranchAddress("trueVertexZ", &vertexZ_float);
    trackChain->SetBranchAddress("trueGlobalTime", &globalTime_float);
    Long64_t nTrackEntries = trackChain->GetEntries();
    // For multiThreading I need to first time order the hits.
    for (Long64_t i = 0; i < nTrackEntries; i++) 
    {
        trackChain->GetEntry(i);

        double vertexX = static_cast<double>(vertexX_float);
        double vertexY = static_cast<double>(vertexY_float);
        double vertexZ = static_cast<double>(vertexZ_float);
        double globalTime = static_cast<double>(globalTime_float);
        tracks.push_back({vertexX, vertexY, vertexZ, globalTime});
    }
    // sort by time
    std::sort(tracks.begin(), tracks.end(), [](const TrackEntry &a, const TrackEntry &b){return a.t < b.t;});

    return tracks;
}
std::vector<ProcessedHit> GetTrackHits(AnaFlags cfg, double threshold, int runNumber, std::string saveName, int planeZ)
{
    std::string inputSubPath = cfg.inputPath+Form("_%04d/", runNumber) + saveName + "/";
    std::vector<ProcessedHit> hits;

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

    hits.reserve(nReconstructedEntries);  // optional (performance only)

    for (int i = 0; i < nReconstructedEntries; i++)
    {
        reconstructedTree->GetEntry(i);
        if (planeID%1000000/10000!= planeZ) continue;
        hits.push_back({planeID, reconstructedPixX, reconstructedPixY, reconstructedTiming});
    }
    // sort by time. Different x-y planes have mixed up the timing.
    std::sort(hits.begin(), hits.end(), [](const ProcessedHit &a, const ProcessedHit &b) {return a.time < b.time;});

    return hits;
}
std::vector<FullTrackInfo> GetMatchedHits(AnaFlags cfg, double threshold, int runNumber, std::string saveName, int planeZ)
{
    std::string inputPath = cfg.inputPath+Form("_%04d/", runNumber) + saveName + "/";

    TFile *trackedFile = TFile::Open((inputPath + "LocalTrackedHitsThr" + std::to_string(int(threshold)) + ".root").c_str(), "READ");
    TTree *trackedTree = (TTree*) trackedFile->Get(Form("TrackedHits_planeZ%d",planeZ));
    std::vector<FullTrackInfo> output;
    double vertexX, vertexY, globalTrigger;
    double reconstructedTime;
    int trackID, pixX, pixY, nHits, planeID;
    trackedTree->SetBranchAddress("planeID", &planeID);
    trackedTree->SetBranchAddress("trackID", &trackID);
    trackedTree->SetBranchAddress("vertexX", &vertexX);
    trackedTree->SetBranchAddress("vertexY", &vertexY);
    trackedTree->SetBranchAddress("vertexTime", &globalTrigger);
    trackedTree->SetBranchAddress("trackID", &trackID);
    trackedTree->SetBranchAddress("DUTPixX", &pixX);  
    trackedTree->SetBranchAddress("DUTPixY", &pixY);  
    trackedTree->SetBranchAddress("DUTnHits", &nHits);
    trackedTree->SetBranchAddress("DUTLocalTime", &reconstructedTime);
    Long64_t nTrackedEntries = trackedTree->GetEntries();

    for (int i = 0; i < nTrackedEntries; i++)
    {
        trackedTree->GetEntry(i);
        output.push_back({planeID, trackID, vertexX, vertexY, globalTrigger, pixX, pixY, nHits, reconstructedTime});
    }

    return output;
}
TFile* CreateClusteredTree(AnaFlags cfg, double threshold, int runNumber, std::string saveName)
{
    std::string inputPath = cfg.inputPath+Form("_%04d/", runNumber) + saveName + "/";
    TFile *outfile = new TFile((inputPath + "analysisThr" + std::to_string(int(threshold)) + ".root").c_str(), "RECREATE");

    return outfile;
}
std::vector<AnalysisHits> GetAnalysisHits(AnaFlags cfg, double threshold, int runNumber, std::string saveName, int planeZ)
{
    std::vector<AnalysisHits> output;
    std::string inputPath = cfg.inputPath+Form("_%04d/", runNumber)+ saveName + "/";
    TFile *analysisFile = TFile::Open((inputPath + "analysisThr" + std::to_string(int(threshold)) + ".root").c_str(), "READ");

    TTree *analysisTree = (TTree*) analysisFile->Get(Form("analyzedHits_planeZ%d",planeZ));

    double fX, fY, timing, correctedTiming;
    int clSize, planeID;
    analysisTree->SetBranchAddress("planeID", &planeID);
    analysisTree->SetBranchAddress("analysisVertexX", &fX);
    analysisTree->SetBranchAddress("analysisVertexY", &fY);
    analysisTree->SetBranchAddress("clSize", &clSize);  
    analysisTree->SetBranchAddress("timing", &timing); 
    analysisTree->SetBranchAddress("correctedTiming", &correctedTiming);
    Long64_t nAnalyzedEntries = analysisTree->GetEntries();
    for (int i =0; i< nAnalyzedEntries; i++)
    {
        analysisTree->GetEntry(i);
        output.push_back({planeID, fX, fY, clSize, timing, correctedTiming});
    }

    return output;
} 
std::vector<CaloHits> GetCalorimetryHits(AnaFlags cfg, int runNumber, std::string saveName, double threshold)
{
    std::vector<CaloHits> output;

    std::string inputPath = cfg.inputPath+Form("_%04d/", runNumber);
    std::string inputSubPath = inputPath + saveName + "/";
    TFile *reconstructedFile = TFile::Open((inputSubPath + "Plane0ReconstructedHitsThr" + std::to_string(int(threshold)) + ".root").c_str(), "READ");


    // Get Calo tree
    TTree *reconstructedTree = (TTree*) reconstructedFile->Get("ReconstructedHits");

    int reconstructedPixX, reconstructedPixY, NHits, planeID;
    double reconstructedTiming_float;
    reconstructedTree->SetBranchAddress("planeID", &planeID);
    reconstructedTree->SetBranchAddress("PixX", &reconstructedPixX);
    reconstructedTree->SetBranchAddress("PixY", &reconstructedPixY);
    reconstructedTree->SetBranchAddress("timing", &reconstructedTiming_float);
    reconstructedTree->SetBranchAddress("NHits", &NHits);
    Long64_t nReconstructedEntries = reconstructedTree->GetEntries();

    for (Long64_t i = 0; i<nReconstructedEntries; i++)
    {
        reconstructedTree->GetEntry(i);
        output.push_back({planeID, reconstructedPixX, reconstructedPixY, static_cast<float>(reconstructedTiming_float), NHits});
    }

    return output;
}
std::vector<PositionHits> GetPositionHits(AnaFlags cfg, int runNumber, std::string saveName, double threshold)
{
    std::vector<PositionHits> output;

    std::string inputPath = cfg.inputPath+Form("_%04d/", runNumber);
    std::string inputSubPath = inputPath + saveName + "/";
    TFile *reconstructedFile = TFile::Open((inputSubPath + "Plane0ReconstructedHitsThr" + std::to_string(int(threshold)) + ".root").c_str(), "READ");

    // Get Pos tree
    TTree *positionTree = (TTree*) reconstructedFile->Get("PositionHits");

    int planeID, stripX, stripY;
    double posTiming;
    positionTree->SetBranchAddress("planeID", &planeID);
    positionTree->SetBranchAddress("stripX", &stripX);
    positionTree->SetBranchAddress("stripY", &stripY);
    positionTree->SetBranchAddress("timing", &posTiming);
    Long64_t nPositionEntries = positionTree->GetEntries();

    for (Long64_t i = 0; i<nPositionEntries; i++)
    {
        positionTree->GetEntry(i);
        output.push_back({planeID, stripX, stripY, static_cast<float>(posTiming)});
    }
    return output;
}

#include "CalorimetryUtils.hh"

#include <stack>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include "TGraph.h"
#include "TFile.h"
#include "TTree.h"
#include <TF1.h>
#include "TFitResult.h"
#include <TChain.h>

void SetClusterCalorimetry(PlaneState& state)
{
    const int dx[4] = { 1, -1,  0,  0 };
    const int dy[4] = { 0,  0,  1, -1 };

    while (!state.pixels.empty())
    {
        state.numClusters++;
        std::stack<Pixel> toVisit;
        Pixel start = *state.pixels.begin();
        toVisit.push(start);
        state.pixels.erase(start);

        while (!toVisit.empty())
        {
            Pixel p = toVisit.top();
            toVisit.pop();

            for (int d = 0; d < 4; ++d)
            {
                Pixel neighbor{p.x + dx[d], p.y + dy[d]};
                auto it = state.pixels.find(neighbor);
                if (it != state.pixels.end())
                {
                    toVisit.push(neighbor);
                    state.pixels.erase(it);
                }
            }
        }
    }
}
void ResetCalorimetry(PlaneState& state, AnaFlags cfg)
{
    state.numSecondaries = 0;
    state.numClusters = 0;
    state.pixels.clear();
    state.timeWindow += cfg.veto;
}
std::pair<std::vector<float>, std::vector<float>> GetPositionCalorimetry(int currentEvent, std::unordered_map<int, Long64_t>& posIndexMap, const std::vector<CaloHits>& caloHits, const std::vector<PositionHits>& positions, PlaneState& state)
{
    // compute X/Y stats using positionTree
    std::vector<float> xVals;
    std::vector<float> yVals;

    auto& posIndex = posIndexMap[caloHits[currentEvent].planeID];

    while (posIndex < positions.size())
    {
        
        //std::cout << posTiming << std::endl;
        if (positions[posIndex].planeID != caloHits[currentEvent].planeID) 
        {
            posIndex++;
            continue;
        }
        if (positions[posIndex].time < state.timeWindow)
        {
            xVals.push_back(positions[posIndex].stripX);
            yVals.push_back(positions[posIndex].stripY);
            posIndex++;
        }
        else break;
    }
    return {xVals, yVals};
}
std::vector<FullCalorimetryInfo> ProcessCalorimetry(std::vector<CaloHits> calorimetryHits, std::vector<PositionHits> positionHits, AnaFlags cfg)
{
    std::unordered_set<Pixel, PixelHash> pixelsToCluster{};
    std::unordered_map<int, PlaneState> planeStates;
    std::unordered_map<int, Long64_t> posIndexMap;
    std::unordered_map<int, Long64_t> eventIDMap;
    std::vector<FullCalorimetryInfo> output;
    float meanX, meanY, sigmaX, sigmaY;

    for (Long64_t i = 0; i < calorimetryHits.size(); i++)
    {

        //std::cout << " Event: " << i << " out of total: " << calorimetryHits.size() << std::endl;
        int planeID = calorimetryHits[i].planeID;
        int pixX = calorimetryHits[i].x;
        int pixY = calorimetryHits[i].y;
        float timing = static_cast<float>(calorimetryHits[i].time);
        auto& state = planeStates[calorimetryHits[i].planeID];
        //std::cout << "Index: " << i << "; timing: " << timing << "; window: " << state.timeWindow << "; planeID: " << planeID << "; pixX: " << pixX << "; pixY: " << pixY << " ; sec: " << state.numSecondaries << std::endl;

        // Initialize time window on first hit
        if (state.timeWindow == 0) state.timeWindow = cfg.veto;

        if (timing < state.timeWindow)
        {
            if (cfg.boolHWC) state.numSecondaries += calorimetryHits[i].nHit;
            else state.numSecondaries++;
            state.pixels.insert({pixX, pixY});
        }
        else
        {
            SetClusterCalorimetry(state);
            // Output
            auto [xVals, yVals] = GetPositionCalorimetry(i, posIndexMap, calorimetryHits, positionHits, state);
            computeStats(xVals, meanX, sigmaX);
            computeStats(yVals, meanY, sigmaY);
            auto& eventID = eventIDMap[planeID];            
            output.push_back({meanX, meanY, sigmaX, sigmaY, planeID, (int)eventID, state.numSecondaries, state.numClusters});
            eventID++;
            // Reset
            ResetCalorimetry(state, cfg);
        }
    }
    output = ProcessLastEventCalorimetry(output, planeStates, posIndexMap, eventIDMap, calorimetryHits, positionHits);

    return output;
}
std::vector<FullCalorimetryInfo> ProcessLastEventCalorimetry(std::vector<FullCalorimetryInfo> output, std::unordered_map<int, PlaneState>& planeStates, std::unordered_map<int, Long64_t>& posIndexMap, std::unordered_map<int, Long64_t>& eventIDMap, std::vector<CaloHits> caloHits, std::vector<PositionHits> positions)
{
    float meanX, meanY, sigmaX, sigmaY;
    // Go through also the last remaining window for each plane
    for (auto& [pid, state] : planeStates)
    {
        if (state.pixels.empty()) continue;
        SetClusterCalorimetry(state);

        auto& eventID = eventIDMap[pid];
        auto [xVals, yVals] = GetPositionCalorimetry((int)eventID, posIndexMap, caloHits, positions, state);

        computeStats(xVals, meanX, sigmaX);
        computeStats(yVals, meanY, sigmaY);
        output.push_back({meanX, meanY, sigmaX, sigmaY, pid, (int)eventID, state.numSecondaries, state.numClusters});
    }
    return output;
}
void FillClusterTree(AnaFlags cfg, int runNumber, std::string saveName, double threshold, std::vector<FullCalorimetryInfo> caloHits)
{
    std::string inputPath = cfg.inputPath+Form("_%04d/", runNumber);
    std::string inputSubPath = inputPath + saveName + "/";
    TFile *outfile = new TFile((inputSubPath + "CalorimetryThr" + std::to_string(int(threshold)) + ".root").c_str(), "RECREATE");

    // Create a TTree
    TTree *caloTree = new TTree("CaloHits", "Calorimetry Hits");

    // Variables for branches
    int numSecondaries, numClusters, eventNum, planeNum;
    float meanX, meanY, sigmaX, sigmaY;
    // Create branches
    caloTree->Branch("meanX", &meanX, "meanX/F");
    caloTree->Branch("meanY", &meanY, "meanY/F");
    caloTree->Branch("sigmaX", &sigmaX, "sigmaX/F");
    caloTree->Branch("sigmaY", &sigmaY, "sigmaY/F");
    caloTree->Branch("planeID", &planeNum, "planeID/I");
    caloTree->Branch("eventID", &eventNum, "eventID/I");
    caloTree->Branch("numSecondaries", &numSecondaries, "numSecondaries/I");
    caloTree->Branch("numClusters", &numClusters, "numClusters/I");

    for (const auto& el : caloHits)
    {
        meanX = el.meanX;
        meanY = el.meanY;
        sigmaX = el.sigmaX;
        sigmaY = el.sigmaY;
        planeNum = el.planeID;
        eventNum = el.eventID;
        numSecondaries = el.numSec;
        numClusters = el.numCl;
        caloTree->Fill();
    }

    caloTree->Write();
    outfile->Close();
}
RawCalorimetryPerMap GetCalorimetryAnalyzedHits( int runNumber, std::string saveName, double threshold)
{
    RawCalorimetryPerMap output;
    std::string summaryPath = Form("Results/local_%04d/%s/CalorimetryThr%i.root", runNumber, saveName.c_str(), int(threshold));
    TFile *summaryFile = TFile::Open(summaryPath.c_str(), "READ");
    TTree *summaryTree = (TTree*) summaryFile->Get("CaloHits");
    Long64_t nSummaryEntries = summaryTree->GetEntries();
    int numSecondaries, numClusters, planeID, eventID;
    float meanX, meanY;
    std::vector<int> histoSecondaries;
    histoSecondaries.reserve(nSummaryEntries);
    std::vector<int> histoClusters;
    histoClusters.reserve(nSummaryEntries);

    std::unordered_map<int, std::vector<int>> secondariesPerPlane;
    std::unordered_map<int, int> secondariesPerEvent;
    std::unordered_map<int, std::vector<int>> secondariesVecPerEvent;
    std::unordered_map<int, std::vector<int>> clustersPerPlane;
    std::unordered_map<int, std::vector<int>> xPosPerPlane;
    std::unordered_map<int, std::vector<int>> yPosPerPlane;

    summaryTree->SetBranchAddress("planeID", &planeID);
    summaryTree->SetBranchAddress("eventID", &eventID);
    summaryTree->SetBranchAddress("meanX", &meanX);
    summaryTree->SetBranchAddress("meanY", &meanY);
    summaryTree->SetBranchAddress("numSecondaries", &numSecondaries);
    summaryTree->SetBranchAddress("numClusters", &numClusters);

    for (Long64_t i = 0; i < nSummaryEntries; i++)
    {
        summaryTree->GetEntry(i);
        secondariesPerEvent[eventID] += numSecondaries;
        secondariesVecPerEvent[eventID].push_back(numSecondaries);
        secondariesPerPlane[planeID].push_back(numSecondaries);
        clustersPerPlane[planeID].push_back(numClusters);
        xPosPerPlane[planeID].push_back(meanX);
        yPosPerPlane[planeID].push_back(meanY);
    }
    return {secondariesPerPlane, secondariesPerEvent, secondariesVecPerEvent, clustersPerPlane, xPosPerPlane, yPosPerPlane};
}
std::vector<FitCalorimetryInfo> GetCalorimetryMultiLayerFitInformation(RawCalorimetryPerMap rawCaloMap)
{
    auto secondariesVecPerEvent = rawCaloMap.secVecPerEvent;
    std::vector<FitCalorimetryInfo> output;
    double radLength = 0.86;
    for (Long64_t i = 0; i< secondariesVecPerEvent.size(); i++)
    {
        std::vector<double> vsecondaries{};
        std::vector<int> vlayer{};
        std::vector<double> vradLength{};
        for (int j = 0 ; j< secondariesVecPerEvent[i].size() ; j+=2)
        {
            // layer index (each pair corresponds to one layer)
            vlayer.push_back(j / 2);
            vradLength.push_back((j/2 + 1)* radLength);

            // make sure we don't go out of bounds
            int sum = secondariesVecPerEvent[i][j];

            if (j + 1 < secondariesVecPerEvent[i].size())
                sum += secondariesVecPerEvent[i][j + 1];

            vsecondaries.push_back(sum);
        }
        TGraph *gSecondaries = new TGraph(vradLength.size(), vradLength.data(), vsecondaries.data());

        TF1 *f = new TF1("f", "[0]*pow([1]*x,[2]-1)*exp(-[1]*x)", 0, vradLength.back());
        const double A_init = 500;
        const double beta_init = 0.5;
        const double alpha_init = 1.7;
        f->SetParameters(A_init, beta_init, alpha_init);
        TFitResultPtr r = gSecondaries->Fit(f,"QS");
        double chi2 = f->GetChisquare();
        double ndf  = f->GetNDF();
        double chi2ndf = chi2 / ndf;

        if (r->Status() != 0) 
        {
            std::cout << "Fit for event " << i << " failed with status " << r->Status() << std::endl;
            delete gSecondaries;
            delete f;
            continue;  // skip pushing bad fits
        }

        double alpha = f->GetParameter(2);
        double beta  = f->GetParameter(1);
        double A = f->GetParameter(0);
        // Shower max position
        double tmax{};
        if (beta!=0) tmax = (alpha - 1)/beta;
        // Total energy deposition
        double totEnergy = f->Integral(0, vradLength.back());
        output.push_back({tmax, totEnergy, chi2ndf, A, beta, alpha});
    }
    return output;
}

std::vector<TTree*> CaloPreProcessing(double inputThreshold, int runNumber, std::string saveName)
{
    // Set all the analysis flags for the digital processing
    auto analysisFlags = new AnaFlags{};
    const char* configPath = std::getenv("ANALYSIS_CONFIG");
    LoadAnalysisFlagsFromFile(configPath, *analysisFlags);
    std::string localPath = analysisFlags->localPath;
    std::string inputPath = analysisFlags->inputPath+Form("_%04d/", runNumber);
    std::string directoryPath = localPath + "Plots/";
    //std::string runPath = Form("local_%04d/", runNumber);

    std::cout << "############################# Calo PreProcessing started for:" << std::endl;
    std::cout << inputPath << std::endl;
    int numThreads = analysisFlags->numThreads; 

    int nPlanes_100 = analysisFlags->nPlanes_100;
    int nPlanes_10 = analysisFlags->nPlanes_10;
    int nPlanes_1 = analysisFlags->nPlanes_1;
    //int nPlanes = nPlanes_100*nPlanes_10*nPlanes_1; // total number of planes

    
    // Extract raw data
    TChain *chainPixel = new TChain("RawPixelHits");
    std::string fileName = analysisFlags->fileName;
    for (int t = 0; t <= numThreads - 1; ++t) 
    {
        chainPixel->Add(Form("%s%s_t%d.root", inputPath.c_str(), fileName.c_str(), t));
    }


    std::vector<int> planes;
    for (int iz = 0; iz < nPlanes_100; ++iz) {
        for (int iy = 0; iy < nPlanes_10; ++iy) {
            for (int ix = 0; ix < nPlanes_1; ++ix) {
                planes.push_back(iz*10000 + iy*100 + ix); // decoded position (works for up to 100 planes in each dimension)
            }
        }
    }
    std::vector<int> modules{};
    for (int i = 0; i< analysisFlags->modules; i++)
    {
        modules.push_back(i);
    }
    //std::vector<TTree*> forest(planes.size());
    std::vector<TTree*> forest;
    forest.resize(modules.size());

    //std::cout << "modules.size(): " << modules.size() << std::endl;

    if (analysisFlags->simProc == "MALTA2")
    {
        for (size_t i = 0; i < modules.size(); i++)
        {
            int m = modules[i];
            TTree* treeSplit = chainPixel->CopyTree(Form("iModule==%d", m));

            //Long64_t nRawEntries = treeSplit->GetEntries();
            //std::cout << "nRawEntries: " << nRawEntries << std::endl;

            treeSplit->SetName(Form("Module%dHits", m));
            forest[i] = treeSplit;
            treeSplit->SetDirectory(nullptr);
        }
    }
    else
    {
        for (int i = 0; i < planes.size(); i++)
        {
            TTree* treeSplit = chainPixel->CopyTree(Form("iPlane==%d", planes[i]));

            //Long64_t nRawEntries = treeSplit->GetEntries();
            //std::cout << "nRawEntries: " << nRawEntries << std::endl;

            treeSplit->SetName(Form("Plane%dHits", i));
            forest[i] = treeSplit;
            treeSplit->SetDirectory(nullptr);
        }
    }
    return forest;

}
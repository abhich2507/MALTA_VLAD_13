#pragma once
#include "ConfigAnalysis.hh"
#include "Clustering_multiPlane.hh"
#include "Calorimetry_multiPlane.hh"
#include "Analysis_multiPlane.hh"
#include "PRIOFIFOHWCProcessing_multiPlane.hh"
#include "Tracking_multiPlane.hh"
#include <TFile.h>
#include <TH1.h>
#include <TH2.h>
#include <map>
#include <string>
#include <unordered_map>
#include <utility>     

void FillReconstructedTree(std::vector<ProcessedHit> allProcessHits, double inputThreshold, int runNumber, std::string saveName, AnaFlags cfg);
TFile* CreateTrackedTree(double inputThreshold, int runNumber, std::string saveName, AnaFlags cfg);
void FillTrackedTree(std::vector<FullTrackInfo> trackedHits, TFile* outfile, int planeZ);
void CloseFile(TFile* outfile);
void SaveResidualHisto(std::vector<Residual> residuals, int planeZ, double threshold, int runNumber, std::string saveName, AnaFlags cfg);
void FillTrackedTree(std::vector<ClusteredHit> allClusters, TFile* outfile, int planeZ);
void SaveHistograms(std::vector<TH2D*> histograms2D, std::vector<TH1D*> histograms1D, TH2D* auxiliaryHisto, AnaFlags cfg, double threshold, int runNumber, std::string saveName);
void SaveSummaryRoot(AnaFlags cfg, int runNumber, std::string saveName, double threshold, int planeZ, AnalyzedHit statistics);
void SaveCalorimetryHistograms(RawCalorimetryPerMap rawCaloMap, std::vector<FitCalorimetryInfo> fitCaloMap, int runNumber, std::string saveName, double threshold);
void FillMALTA3HWCTree(std::pair<std::vector<ProcessedHit>, std::vector<ProcessedHit>> malta3Hits, AnaFlags cfg, double threshold, int runNumber, std::string saveName);
void savePlot (std::string directoryPath, std::string runPath, double threshold, std::string saveName, TH1* hist, std::string histName);
std::vector<RawHit> GetRawHits(TTree* plane);
std::vector<TrackEntry> GetVertex(AnaFlags cfg, int runNumber);
std::vector<ProcessedHit> GetTrackHits(AnaFlags cfg, double threshold, int runNumber, std::string saveName, int planeZ);
std::vector<FullTrackInfo> GetMatchedHits(AnaFlags cfg, double threshold, int runNumber, std::string saveName, int planeZ);
TFile* CreateClusteredTree(AnaFlags cfg, double threshold, int runNumber, std::string saveName);
std::vector<AnalysisHits> GetAnalysisHits(AnaFlags cfg, double threshold, int runNumber, std::string saveName, int planeZ);
std::vector<CaloHits> GetCalorimetryHits(AnaFlags cfg, int runNumber, std::string saveName, double threshold);
std::pair<std::vector<float>, std::vector<float>> GetPositionCalorimetry(int currentEvent, std::unordered_map<int, Long64_t>& posIndexMap, const std::vector<CaloHits>& caloHits, const std::vector<PositionHits>& positions, PlaneState& state);

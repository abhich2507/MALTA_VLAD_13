#pragma once

#include "ConfigAnalysis.hh"
#include <utility>
#include <iostream>
#include <ROOT/RNTuple.hxx>
#include <TH3D.h>
#include <TFile.h>
#include <TTree.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <chrono>
#include "DigitalProcessing_multiPlane.hh"
#include "Tracking_multiPlane.hh"
#include "Clustering_multiPlane.hh"
#include "Analysis_multiPlane.hh"
#include "Calorimetry_multiPlane.hh"
#include "PRIOFIFOHWCProcessing_multiPlane.hh"

AnaFlags GetDigitalConfig();

void set_style();
// Save diagnostic plots
void savePlot (std::string directoryPath, std::string runPath, double threshold, std::string saveName, TH1* hist, std::string histName);

std::string getVarFromConfig();

// Returns timing offset for a given amplitude
double GetTimingOffset(double amplitude, double threshold, double T, double Tdiv, double TrefThr, double x0, double n, double t0);


double GetFrontEndJitter(double charge);

__uint128_t encodeWord(int pixX, int pixY, int groupSizeX, int groupSizeY , int groupLeng, int parityLeng, int dColLeng, bool verbose);

// Customizable mask for decoding digital subwords in a MALTA word.
std::vector<__uint128_t> decodingMaskMSB(__uint128_t word, const std::vector<int>& field_sizes);

// Decodes a digital word into pixel positions and hit counts
std::vector<DecodedHit> decodedDigitalWord(__uint128_t word, AnaFlags cfg);

//void digitalTest(std::vector<std::pair<int,int>> hits);

    
ThresholdMap generateThrMap(double inputThreshold, int runNumber, std::string saveName, AnaFlags cfg, unsigned int seed);

// This is a user specific function that changes the pixel position based on the plane number.
// In this case the XOffset and YOffset is different for every second planeZ position.
std::pair<double,double> GetSpecificPlaneOffset(int plane, std::string geometry);

std::vector<uint32_t> CompressWords(std::vector<uint32_t> vals, int targetWidth);


double dot(const Vec3& a, const Vec3& b);


void BuildRotationMatrix(Offset& g);

Vec3 ApplyGeometry3D(const Vec3& p, Offset& g);

Vec3 ApplyInverseGeometry3D(const Vec3& global, const Offset& g);

std::vector<Module> LoadModules(const std::string& filename);

std::map<int, Offset> LoadGeometry(const std::string& geoPath, const DetectorConfig& cfg);


DetectorConfig LoadConfig(const std::string& configPath);


// Reconstruct position of planes from the config file
// return global coordinates in mm of the pixel center in which hit occured.
Vec3 PixelPositionReconstruction(int pixelX, int pixelY, const DetectorConfig& cfg);

Vec3 IntersectTrackPlane(const Vec3& V,  const DetectorConfig& cfg, Offset& g);
//---------------------- DIGITAL
std::vector<RawHit> GetRawHits(TTree* plane);

std::pair<EnergyMap, TimeMap> BuildEnergyTimeMap(std::vector<RawHit> rawHits);

SortedTimeVector CorrectAndSortTimeMap(EnergyMap enMap, TimeMap timeMap, ThresholdMap thresholdMap , AnaFlags cfg, unsigned int seed);

std::vector<WordBucket> AssignMALTA2WordBuckets(EnergyMap enMap, SortedTimeVector sortedTimings ,ThresholdMap thresholdMap, AnaFlags cfg);

CoincidentWords MergeMALTA2Words(std::vector<WordBucket> digitizedWords);

std::vector<ProcessedHit> ProcessDecodedHits(CoincidentWords mergedWords, AnaFlags cfg, unsigned int seed);

void FillReconstructedTree(std::vector<ProcessedHit> allProcessHits, double inputThreshold, int runNumber, std::string saveName, AnaFlags cfg);
//----------------------- TRACKING
std::vector<TrackEntry> GetVertex(AnaFlags cfg, int runNumber);

std::vector<ProcessedHit> GetTrackHits(AnaFlags cfg, double threshold, int runNumber, std::string saveName, int planeZ);

std::pair<std::vector<FullTrackInfo>, std::vector<Residual>> MatchHits(std::vector<TrackEntry> tracks, std::vector<ProcessedHit> hits, AnaFlags cfg, int runNumber);

TFile* CreateTrackedTree(double inputThreshold, int runNumber, std::string saveName, AnaFlags cfg);

void FillTrackedTree(std::vector<FullTrackInfo> trackedHits, TFile* outfile, int planeZ);

void CloseFile(TFile* outfile);

void SaveResidualHisto(std::vector<Residual> residuals, int planeZ, double threshold, int runNumber, std::string saveName, AnaFlags cfg);
//----------------------- CLUSTERING
bool hasHitAt(const std::vector<Hit>& cluster, int x, int y);

std::vector<FullTrackInfo> GetMatchedHits(AnaFlags cfg, double threshold, int runNumber, std::string saveName, int planeZ);

Cluster ValidateCluster(std::vector<Hit>& cluster, const DetectorConfig& cfg);

std::pair<double,double> GetClusterPosition(const Cluster& cl, ClusterState& state, const std::string&   clPosMode);

ClusteredHit GetValidCluster(const Cluster& cl, ClusterState& state, const std::pair<double,double> vertex);

void ResetClusterState(ClusterState& state, FullTrackInfo track);

TFile* CreateClusteredTree(AnaFlags cfg, double threshold, int runNumber, std::string saveName);

void FillTrackedTree(std::vector<ClusteredHit> allClusters, TFile* outfile, int planeZ);

std::vector<std::vector<FullTrackInfo>> GroupHitsByTrack(const std::vector<FullTrackInfo>& hits);

ClusterState BuildClusterState(const std::vector<FullTrackInfo>& track);
//----------------------- ANALYSIS
double getEffErr(int Npassed, int Nall);

double getEff(int Npassed, int Nall);

std::vector<AnalysisHits> GetAnalysisHits(AnaFlags cfg, double threshold, int runNumber, std::string saveName, int planeZ);

std::vector<TH2D*> Create2DHistograms(AnaFlags cfg, int planeZ);

std::vector<TH1D*> Create1DHistograms(int planeZ);

TH2D* FillHistograms(std::vector<AnalysisHits> analysisHits, std::vector<TH2D*> histograms2D, std::vector<TH1D*> histograms1D, AnaFlags cfg, DetectorConfig detCfg, int planeZ);

AnalyzedHit GetStatistics(std::vector<TH2D*> histograms2D, TH2D* auxiliaryHisto);

void ScaleHistograms(std::vector<TH2D*> histograms2D, std::vector<TH1D*> histograms1D, TH2D* auxiliaryHisto);

void SetHistogramStyle(std::vector<TH2D*> histograms2D, std::vector<TH1D*> histograms1D, AnalyzedHit statistics);

void SaveHistograms(std::vector<TH2D*> histograms2D, std::vector<TH1D*> histograms1D, TH2D* auxiliaryHisto, AnaFlags cfg, double threshold, int runNumber, std::string saveName);

void SaveSummaryRoot(AnaFlags cfg, int runNumber, std::string saveName, double threshold, AnalyzedHit statistics);
//----------------------- CALORIMETRY
std::vector<CaloHits> GetCalorimetryHits(AnaFlags cfg, int runNumber, std::string saveName, double threshold);

std::pair<std::vector<float>, std::vector<float>> GetPositionCalorimetry(int currentEvent, std::unordered_map<int, Long64_t>& posIndexMap, const std::vector<CaloHits>& caloHits, const std::vector<PositionHits>& positions, PlaneState& state);

std::vector<PositionHits> GetPositionHits(AnaFlags cfg, int runNumber, std::string saveName, double threshold);

void SetClusterCalorimetry(PlaneState& state);

void ResetCalorimetry(PlaneState& state, AnaFlags cfg);

std::vector<FullCalorimetryInfo> ProcessCalorimetry(std::vector<CaloHits> calorimetryHits, std::vector<PositionHits> positionHits, AnaFlags cfg);

std::vector<FullCalorimetryInfo> ProcessLastEventCalorimetry(std::vector<FullCalorimetryInfo> output, std::unordered_map<int, PlaneState>& planeStates, std::unordered_map<int, Long64_t>& posIndexMap, std::unordered_map<int, Long64_t>& eventIDMap, std::vector<CaloHits> caloHits, std::vector<PositionHits> positions);

void FillClusterTree(AnaFlags cfg, int runNumber, std::string saveName, double threshold, std::vector<FullCalorimetryInfo> caloHits);

RawCalorimetryPerMap GetCalorimetryAnalyzedHits( int runNumber, std::string saveName, double threshold);

std::vector<FitCalorimetryInfo> GetCalorimetryMultiLayerFitInformation(RawCalorimetryPerMap rawCaloMap);

void SaveCalorimetryHistograms(RawCalorimetryPerMap rawCaloMap, std::vector<FitCalorimetryInfo> fitCaloMap, int runNumber, std::string saveName, double threshold);
//----------------------- MALTA3 HWC
std::vector<MALTA3Word> EncodeMALTA3ReducedWord(EnergyMap enMap, SortedTimeVector sortedTimings, ThresholdMap thresholdMap, AnaFlags cfg);

std::unordered_map<int, std::vector<MALTA3Word>> SortWordsByBus(std::vector<MALTA3Word> words);

std::map<MatrixSection, std::vector<MALTA3Word>> SortWordsByGroup(std::unordered_map<int, std::vector<MALTA3Word>> busWords);

std::vector<MALTA3Word> GroupMerge(std::map<MatrixSection, std::vector<MALTA3Word>> groupWords, AnaFlags cfg);

std::vector<MALTA3Word> BusMerge(std::unordered_map<int, std::vector<MALTA3Word>> busWords, AnaFlags cfg);

int SelectSector(const SRAMState& state, AnaFlags cfg);

DrainResult DrainSector(int sector, SRAMState& state, AnaFlags cfg);

MALTA3Word BuildHWCWord(const DrainResult& drain, int sector, AnaFlags cfg);

void ProcessReadCycle(SRAMState& state, std::vector<MALTA3Word>& wordsAfterSRAM, AnaFlags cfg);

void WriteToMemory(int bus, __uint128_t word, double timing, SRAMState& state, AnaFlags cfg);

std::vector<MALTA3Word> MemorySynchronize(std::vector<MALTA3Word> wordsAfterBus, AnaFlags cfg);

std::vector<MALTA3Word> FIFOPass(std::vector<MALTA3Word> wordsAfterSRAM, AnaFlags cfg);

std::pair<std::vector<ProcessedHit>, std::vector<ProcessedHit>> DecodeMALTA3HWCHits( std::vector<MALTA3Word> wordsAfterFIFO, int planeID, Offset offset, AnaFlags cfg);

void FillMALTA3HWCTree(std::pair<std::vector<ProcessedHit>, std::vector<ProcessedHit>> malta3Hits, AnaFlags cfg, double threshold, int runNumber, std::string saveName);

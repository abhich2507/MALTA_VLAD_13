#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include "ConfigAnalysis.hh"
#include "Calorimetry_multiPlane.hh"
#include "TTree.h"

std::vector<PositionHits> GetPositionHits(AnaFlags cfg, int runNumber, std::string saveName, double threshold);
void SetClusterCalorimetry(PlaneState& state);
void ResetCalorimetry(PlaneState& state, AnaFlags cfg);
std::vector<FullCalorimetryInfo> ProcessCalorimetry(std::vector<CaloHits> calorimetryHits, std::vector<PositionHits> positionHits, AnaFlags cfg);
std::vector<FullCalorimetryInfo> ProcessLastEventCalorimetry(std::vector<FullCalorimetryInfo> output, std::unordered_map<int, PlaneState>& planeStates, std::unordered_map<int, Long64_t>& posIndexMap, std::unordered_map<int, Long64_t>& eventIDMap, std::vector<CaloHits> caloHits, std::vector<PositionHits> positions);
void FillClusterTree(AnaFlags cfg, int runNumber, std::string saveName, double threshold, std::vector<FullCalorimetryInfo> caloHits);
RawCalorimetryPerMap GetCalorimetryAnalyzedHits( int runNumber, std::string saveName, double threshold);
std::vector<FitCalorimetryInfo> GetCalorimetryMultiLayerFitInformation(RawCalorimetryPerMap rawCaloMap);
std::vector<TTree*> CaloPreProcessing(double inputThreshold, int runNumber, std::string saveName, AnaFlags cfg);
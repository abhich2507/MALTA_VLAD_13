#pragma once
#include "ConfigAnalysis.hh"
#include "PRIOFIFOHWCProcessing_multiPlane.hh"
#include <cstdint>
#include <map>
#include <unordered_map>
#include <utility>
#include <vector>
#include "DigitalProcessing_multiPlane.hh"
#include "Tracking_multiPlane.hh"

double ComputeGeometryDelay(int row);
double ComputeModuleDelay(int module);
double ComputeFrontEndJitter(unsigned int seed, double charge);
std::pair<EnergyMap, TimeMap> BuildEnergyTimeMap(std::vector<RawHit> rawHits);
SortedTimeVector CorrectAndSortTimeMap(EnergyMap enMap, TimeMap timeMap, ThresholdMap thresholdMap , AnaFlags cfg, unsigned int seed);
std::vector<WordBucket> AssignMALTA2WordBuckets(EnergyMap enMap, SortedTimeVector sortedTimings ,ThresholdMap thresholdMap, AnaFlags cfg);
CoincidentWords MergeMALTA2Words(std::vector<WordBucket> digitizedWords);
std::vector<ProcessedHit> ProcessDecodedHits(CoincidentWords mergedWords, AnaFlags cfg, unsigned int seed);
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
// Returns timing offset for a given amplitude
double GetTimeWalk(double amplitude, double threshold, double T, double Tdiv, double TrefThr, double x0, double n, double t0);
double GetFrontEndJitter(double charge);
__uint128_t encodeWord(int pixX, int pixY, int groupSizeX, int groupSizeY , int groupLeng, int parityLeng, int dColLeng, bool verbose);
// Customizable mask for decoding digital subwords in a MALTA word.
std::vector<__uint128_t> decodingMaskMSB(__uint128_t word, const std::vector<int>& field_sizes);
// Decodes a digital word into pixel positions and hit counts
std::vector<DecodedHit> decodedDigitalWord(__uint128_t word, AnaFlags cfg);
std::vector<uint32_t> CompressWords(std::vector<uint32_t> vals, int targetWidth);
ThresholdMap generateThrMap(double inputThreshold, int runNumber, std::string saveName, AnaFlags cfg, unsigned int seed);
#pragma once
#include "ConfigAnalysis.hh"
#include <cstdint>
#include <map>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

class TTree;

struct HitKey
{
    int plane;
    int event;
    int x;
    int y;
    
    bool operator<(const HitKey& other) const {
        return std::tie(plane, event, x, y) <
               std::tie(other.plane, other.event, other.x, other.y);
    }
};
using ThresholdMap     = std::map<std::pair<int,int>, double>;
using EnergyMap        = std::map<HitKey, double>;
using TimeMap          = std::map<HitKey, std::vector<double>>;
using SortedTimeVector = std::vector<std::pair<HitKey, double>>;
struct RawHit
{
    HitKey key;
    double energy;
    double time;
};
struct MALTA2Word
{
    __uint128_t word;
    double time;
    int planeID;
};
using WordBucket = std::vector<MALTA2Word>;
struct MALTA2ModuleWord
{
    __uint128_t word;
    double time;
    std::vector<int> planeID;
};
using CoincidentWords = std::vector<MALTA2ModuleWord>;
struct DecodedHit
{
    int x;
    int y;
    int nHit;
};
struct ProcessedHit
{
    int planeID;
    int x;
    int y;
    double time;
    int nHit;
};

// Main digital processing function
int DigitalProcessing_multiPlane(double threshold, int runNumber, std::string saveName, bool proteusFlag);
ThresholdMap generateThrMap(double inputThreshold, int runNumber, std::string saveName, AnaFlags cfg, unsigned int seed);
std::vector<RawHit> GetRawHits(TTree* plane);
std::pair<EnergyMap, TimeMap> BuildEnergyTimeMap(std::vector<RawHit> rawHits);
SortedTimeVector CorrectAndSortTimeMap(EnergyMap enMap, TimeMap timeMap, ThresholdMap thresholdMap , AnaFlags cfg, unsigned int seed);
std::vector<WordBucket> AssignMALTA2WordBuckets(EnergyMap enMap, SortedTimeVector sortedTimings, ThresholdMap thresholdMap, AnaFlags cfg);
CoincidentWords MergeMALTA2Words(std::vector<WordBucket> digitizedWords);
std::vector<ProcessedHit> ProcessDecodedHits(CoincidentWords mergedWords, AnaFlags cfg, unsigned int seed);

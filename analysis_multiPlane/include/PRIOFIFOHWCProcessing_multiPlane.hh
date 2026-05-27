#pragma once
#include <cstddef>
#include <utility>
#include <functional>
#include <queue>

struct PairHash 
{
    // Raw data sorting hash
    size_t operator()(const std::pair<int, std::pair<int,int>>& k) const {
        size_t h = std::hash<int>{}(k.first);
        h ^= std::hash<int>{}(k.second.first)  + 0x9e3779b9 + (h<<6) + (h>>2);
        h ^= std::hash<int>{}(k.second.second) + 0x9e3779b9 + (h<<6) + (h>>2);
        return h;
    }
};
struct MALTA3Word
{
    __uint128_t word;
    double time;
};
struct MatrixSection
{
    __int128 unsigned group;
    int bus;

    bool operator<(const MatrixSection& other) const
    {
        if (group != other.group) return group < other.group;
        return bus < other.bus;
    }
};
struct SRAMState
{
    std::vector<int> memoryModule;
    std::vector<std::vector<MALTA3Word>> memoryWordStore;
    std::vector<int> sectorOccupancy;
    std::queue<int> occupiedSectors;
    int missedHitCount = 0;
    double prevGlobalTiming = 0;
    double timeMEMSYNC = 0;

    SRAMState(int nSectors) :
        memoryModule(512, 0),
        memoryWordStore(512),
        sectorOccupancy(nSectors, 0)
    {}
};
struct DrainResult
{
    std::vector<uint32_t> reducedWords;
    double improvTiming;
    int numValids;
};

void PRIOFIFOHWCProcessing_multiPlane(double inputThreshold, int runNumber, std::string saveName);
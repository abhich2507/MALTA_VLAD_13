#include "DigitalUtils.hh"
#include "ConfigAnalysis.hh"
#include "PRIOFIFOHWCProcessing_multiPlane.hh"
#include "RootIO.hh"
#include "Utils.hh"
#include <algorithm>
#include <bitset>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <map>
#include <queue>
#include <random>
#include <unordered_map>
#include <utility>
#include <vector>

std::pair<EnergyMap, TimeMap> BuildEnergyTimeMap(std::vector<RawHit> rawHits)
{
    EnergyMap enMap;
    TimeMap timeMap;
    for (const auto& hit : rawHits)
    {
        enMap[HitKey{hit.key.plane, hit.key.event, hit.key.x, hit.key.y}] += hit.energy;
        timeMap[HitKey{hit.key.plane, hit.key.event, hit.key.x, hit.key.y}].push_back(hit.time);

    }
    return {enMap, timeMap};

}
SortedTimeVector CorrectAndSortTimeMap(EnergyMap enMap, TimeMap timeMap, ThresholdMap thresholdMap , AnaFlags cfg, unsigned int seed)
{
    std::mt19937 gen(seed);
    SortedTimeVector sortedTimings;
    for (const auto& entry : enMap) 
    {
        const HitKey& key = entry.first;
        auto itThr = thresholdMap.find({key.x, key.y});
        if (itThr == thresholdMap.end()) continue;

        auto it = timeMap.find(key);
        if(it == timeMap.end()) continue;
        // Row correction also of 7ns/ 512 rows + global GEANT4 timestamp + in-module chip id + front end jitter
        std::normal_distribution<double> gauss(0.0, GetFrontEndJitter(entry.second));    
        double timing    = GetTimingOffset(entry.second, itThr->second, cfg.T, cfg.Tdiv, cfg.TrefThr, cfg.x0, cfg.n, cfg.t0);     
        timing += *std::max_element(it->second.begin(), it->second.end()) + key.y * 0.0125 + (4 - (key.plane %100)%4) * 8 + std::abs(gauss(gen));
        sortedTimings.emplace_back(entry.first,timing);
    }
    std::sort(sortedTimings.begin(), sortedTimings.end(), [](auto &a, auto &b){ return a.second < b.second; });

    return sortedTimings;
}
std::vector<WordBucket> AssignMALTA2WordBuckets(EnergyMap enMap, SortedTimeVector sortedTimings , ThresholdMap thresholdMap, AnaFlags cfg)
{
    WordBucket merger;
    std::vector<WordBucket> digitizedWords;
    double t0 = sortedTimings.begin()->second;
    for (const auto& entry : sortedTimings) 
    {
        const HitKey& key = entry.first;
        auto itThr = thresholdMap.find({key.x, key.y});
        if (itThr == thresholdMap.end()) continue;

        double threshold = itThr->second;
        double timing = entry.second;
        auto it = enMap.find(key);
        double cenergy = it->second;

        if (cenergy < threshold) continue;

        __uint128_t word = encodeWord(key.x, key.y, cfg.groupSizeX, cfg.groupSizeY, cfg.groupLeng, cfg.parityLeng, cfg.dColLeng, false);

        //Now I digitized all my words. Next step is merging them based on timing
        if (timing >= t0 && timing < t0 + cfg.wordSpacing)
        {
            merger.push_back({word, timing, key.plane});
        }
        else
        {
            t0 = timing;
            digitizedWords.push_back(merger);
            merger.clear();
            merger.push_back({word, timing, key.plane});
        }
    }
    // Also push the very last merged word.
    if (!merger.empty()) digitizedWords.push_back(merger);

    return digitizedWords;
}
CoincidentWords MergeMALTA2Words(std::vector<WordBucket> digitizedWords)
{
    CoincidentWords mergedWords;
    
    for (int i =0; i< digitizedWords.size(); i++)
    {
        // This was not initialized before leading to weird first word
        __uint128_t mergedWord = 0; 
        std::vector<double> leadingTime{};
        std::vector<int> planeVec{};
        int aux = 0;
        for (const auto& pos : digitizedWords[i])
        {
            int word = pos.word;
            double timing = pos.time;
            int planeN = pos.planeID;
            planeVec.push_back(planeN);
            mergedWord |= word;
            leadingTime.push_back(timing);
        }
        // Save the merged word and the fastest hit time
        if (digitizedWords[i].size() == 0) continue; // Skip empty vectors
        mergedWords.push_back({mergedWord, *std::max_element(leadingTime.begin(), leadingTime.end()), planeVec});
        // Reset fot next iteration
        mergedWord = 0;
        leadingTime.clear();
        planeVec.clear();
    }
    return mergedWords;

}
std::vector<ProcessedHit> ProcessDecodedHits(CoincidentWords mergedWords, AnaFlags cfg, unsigned int seed)
{
    std::mt19937 gen(seed);
    std::vector<ProcessedHit> output;
    for (auto& entry: mergedWords) 
    {
        std::vector<int> planeN = entry.planeID;
        __uint128_t word = entry.word;
        std::vector<DecodedHit> pixelPositions = decodedDigitalWord(word, cfg);
        int i = 0;
        for (const auto& pos : pixelPositions) 
        {
            ///// Add off-chip READOUT jitter
            // Add in quadrature the un-correlated jitter
            double totalJitter = std::sqrt(cfg.scintillatorJitter*cfg.scintillatorJitter + cfg.samplingJitter*cfg.samplingJitter);
            std::normal_distribution<double> gauss(0.0, totalJitter);
            output.push_back({entry.planeID[i], pos.x, pos.y, entry.time + gauss(gen), pos.nHit});
            i++;
        }
    }
    return output;
}
std::vector<MALTA3Word> EncodeMALTA3ReducedWord(EnergyMap enMap, SortedTimeVector sortedTimings, ThresholdMap thresholdMap, AnaFlags cfg)
{
    std::vector<MALTA3Word> words{};
    double prevTiming{};
    __uint128_t groupMerger{};
    int count{};
    for (const auto& entry : sortedTimings) 
    {
        const HitKey& key = entry.first;
        int eventID = entry.first.plane;
        int pixX   = entry.first.x;
        int pixY   = entry.first.y;
        double timing = entry.second; 
        count++;
        auto itThr = thresholdMap.find({pixX, pixY});
        
        double threshold = itThr->second;
        auto it = enMap.find(key);
        double cenergy = it->second;
    
        if (cenergy < threshold) continue;
        
        __uint128_t word = encodeWord(pixX, pixY, cfg.groupSizeX, cfg.groupSizeY, cfg.groupLeng, cfg.parityLeng, cfg.dColLeng, false);
        if (count == 1) 
        {
            prevTiming = timing;
            groupMerger = word;
        }
        words.push_back({word, timing});
    }
    return words;
}
std::unordered_map<int, std::vector<MALTA3Word>> SortWordsByBus(std::vector<MALTA3Word> words)
{
    std::unordered_map<int, std::vector<MALTA3Word>> busWords;
    for (auto&[word, timing] : words)
    {
        // This is not generalized for any bit size
        int doubleColumn = word& 0xFF;
        int parity = (word & ((__uint128_t)1 << 8)) != 0;
        //int bus = ( parity + 1) * doubleColumn;
        int bus = 2 * doubleColumn + parity;
        busWords[bus].push_back({word,timing});
    }
    return busWords;
}
std::map<MatrixSection, std::vector<MALTA3Word>> SortWordsByGroup(std::unordered_map<int, std::vector<MALTA3Word>> busWords)
{
    std::map<MatrixSection, std::vector<MALTA3Word>> groupWords;
    for (auto& [bus, words] : busWords)
    {
        for (auto& word : words)
        {
            __uint128_t group = (word.word >> 9) & ((__uint128_t)0x1F); 
            groupWords[{group, bus}].push_back(word);
        }
    }
    return groupWords;
}
std::vector<MALTA3Word> GroupMerge(std::map<MatrixSection, std::vector<MALTA3Word>> groupWords, AnaFlags cfg)
{
    std::vector<MALTA3Word> wordsAfterGroup{};
    for (auto& [ID, words]: groupWords)
    {
        std::sort(words.begin(), words.end(), [](auto &a, auto &b) {return a.time < b.time;});
        std::vector<MALTA3Word> groupMerged;
        for (size_t i = 0; i < words.size(); i++)
        {
            
            auto [word, timing] = words[i];
            if (groupMerged.empty())
            {
                groupMerged.push_back({word, timing});
                continue;
            }
            auto &[lastWord, lastTime] = groupMerged.back();

            if(timing - lastTime <= cfg.slowcontrolDelay)
            {
                lastWord|=word;
            }
            else
            {
                groupMerged.push_back({word, timing});
            }
            
        }
        wordsAfterGroup.insert(wordsAfterGroup.end(), groupMerged.begin(), groupMerged.end());
    }
    return wordsAfterGroup;
}
std::vector<MALTA3Word> BusMerge(std::unordered_map<int, std::vector<MALTA3Word>> busWords, AnaFlags cfg)
{
    std::vector<MALTA3Word> wordsAfterBus{};
    for (auto& [busID,words]: busWords)
    {
        // Sort by timing
        std::sort(words.begin(), words.end(), [](auto &a, auto &b) {return a.time < b.time;});
        std::vector<MALTA3Word> busMerged;
        for (size_t i = 0; i < words.size(); i++)
        {
            auto [word, timing] = words[i];
            if (busMerged.empty())
            {
                busMerged.push_back({word, timing});
                continue;
            }
            auto &[lastWord, lastTime] = busMerged.back();

            if(timing - lastTime <= cfg.busMergingThreshold)
            {
                lastWord|=word;
            }
            else
            {
                busMerged.push_back({word, timing});
            }
            
        }

        wordsAfterBus.insert(wordsAfterBus.end(), busMerged.begin(), busMerged.end());
    }
    std::sort(wordsAfterBus.begin(), wordsAfterBus.end(), [](auto &a, auto &b){return a.time < b.time;});

    return wordsAfterBus;
}
int SelectSector(SRAMState& state, AnaFlags cfg)
{
    int nSectors = (512 + (2 * cfg.sectorSize) - 1) / (2 * cfg.sectorSize);
    if (cfg.prioAlgo == "RoundRobin")
    {
        int sector = state.occupiedSectors.front();
        state.occupiedSectors.pop();
        return sector;
    }

    std::vector<int> filledBuses(nSectors, 0);
    for (int i = 0; i < 512; i++)
    {
        int currentSector = static_cast<int>(i / (2 * cfg.sectorSize));
        if (cfg.prioAlgo == "MostFilled" && state.memoryModule[i] > 0) filledBuses[currentSector]++;
        if (cfg.prioAlgo == "MostFull"   && state.memoryModule[i] > 0) filledBuses[currentSector] += state.memoryModule[i];
    }
    state.occupiedSectors.pop();
    auto it = std::max_element(filledBuses.begin(), filledBuses.end());
    return std::distance(filledBuses.begin(), it);
}
DrainResult DrainSector(int sector, SRAMState& state, AnaFlags cfg)
{
    int start = sector * 2 * cfg.sectorSize;
    int end   = std::min(start + 2 * cfg.sectorSize, 512);

    DrainResult result{};
    for (int i = start; i < end; i++)
    {
        if (state.memoryModule[i] > 0 && !state.memoryWordStore[i].empty())
        {
            uint32_t fourBit = state.memoryWordStore[i].front().word & 0xF;
            result.reducedWords.push_back(fourBit);
            result.improvTiming = std::max(result.improvTiming, state.memoryWordStore[i].front().time);
            state.memoryWordStore[i].erase(state.memoryWordStore[i].begin());
            state.memoryModule[i]--;
            state.sectorOccupancy[sector]--;
            result.numValids++;
        }
        else result.reducedWords.push_back(0);
    }
    return result;
}
MALTA3Word BuildHWCWord(const DrainResult& drain, int sector, AnaFlags cfg)
{
    __uint128_t HWCWord{};
    auto vcompressedWords = CompressWords(drain.reducedWords, cfg.wordSize);
    for (const auto& compWord : vcompressedWords)
        HWCWord = (HWCWord << cfg.wordSize) | (compWord & ((1u << cfg.wordSize) - 1));

    uint8_t sixBit = sector & 0x3F;
    __uint128_t fullHWCWord = (HWCWord << 6) | sixBit;
    return {fullHWCWord, drain.improvTiming};
}
void ProcessReadCycle(SRAMState& state, std::vector<MALTA3Word>& wordsAfterSRAM, AnaFlags cfg)
{
    if (state.occupiedSectors.empty()) return;

    int sector  = SelectSector(state, cfg);
    auto drain  = DrainSector(sector, state, cfg);

    if (state.sectorOccupancy[sector] > 0)
        state.occupiedSectors.push(sector);

    auto hwcWord = BuildHWCWord(drain, sector, cfg);
    if (hwcWord.word != 0)
        wordsAfterSRAM.push_back(hwcWord);
}
void WriteToMemory(int bus, __uint128_t word, double timing, SRAMState& state, AnaFlags cfg)
{
    if (state.memoryModule[bus] >= cfg.sramDepth) { state.missedHitCount++; return; }

    int sector = bus / (2 * cfg.sectorSize);
    if (state.sectorOccupancy[sector] == 0) state.occupiedSectors.push(sector);

    int pixAddr = (word >> 14) & 0xFFFF;
    __uint128_t HWCWord = __builtin_popcount(pixAddr);
    state.memoryWordStore[bus].push_back({HWCWord, timing});
    state.memoryModule[bus]++;
    state.sectorOccupancy[sector]++;
}
std::vector<MALTA3Word> MemorySynchronize(std::vector<MALTA3Word> wordsAfterBus, AnaFlags cfg)
{
    int nSectors = (512 + (2 * cfg.sectorSize) - 1) / (2 * cfg.sectorSize);
    SRAMState  state{nSectors};
    std::vector<MALTA3Word> wordsAfterSRAM{};

    for (auto& [word, timing] : wordsAfterBus)
    {
        int doubleColumn = word & 0xFF;
        int parity       = (word & ((__uint128_t)1 << 8)) != 0;
        //int bus          = ( parity + 1) * doubleColumn;
        int bus          = 2 * doubleColumn + parity;
        state.timeMEMSYNC     += timing - state.prevGlobalTiming;
        state.prevGlobalTiming = timing;

        int nRead = floor(state.timeMEMSYNC / cfg.SRAMFrequency);
        for (int k = 0; k < nRead; k++)
        {
            ProcessReadCycle(state, wordsAfterSRAM, cfg);
            state.timeMEMSYNC -= cfg.SRAMFrequency;
        }
        WriteToMemory(bus, word, timing, state, cfg);
    }

    std::sort(wordsAfterSRAM.begin(), wordsAfterSRAM.end(), [](auto& a, auto& b) { return a.time < b.time; });
    return wordsAfterSRAM;
}
std::vector<MALTA3Word> FIFOPass(std::vector<MALTA3Word> wordsAfterSRAM, AnaFlags cfg)
{
    double prevFIFOTiming{};
    double timeFIFO{};
    int fifoFill{};
    std::vector<MALTA3Word> wordsAfterFIFO{};  

    for (auto& [word, timing]:wordsAfterSRAM)
    {
        double prevTiming = prevFIFOTiming ;
        double timeDiff = timing - prevTiming;
        timeFIFO += timeDiff;
        prevFIFOTiming = timing;
        int nRead = floor(timeFIFO / cfg.FIFOFrequency);
        if(nRead >= 1)
        {
            for(int k = 0; k < nRead; k++) 
            {
                if (fifoFill) fifoFill--;
            }
            timeFIFO -= nRead * cfg.FIFOFrequency;
        }
        if(fifoFill < cfg.fifoSize)
        {
            wordsAfterFIFO.push_back({word, timing});
            fifoFill++;
        }
    }
    return wordsAfterFIFO;
}
std::pair<std::vector<ProcessedHit>, std::vector<ProcessedHit>> DecodeMALTA3HWCHits( std::vector<MALTA3Word> wordsAfterFIFO, int planeID, Offset offset, AnaFlags cfg)
{
    std::vector<ProcessedHit> hitTree, positionTree;
    double dutAxisRotation = offset.zrot;

    for (const auto &word: wordsAfterFIFO)
    {
        int nHits = 0;
        double reconstructedTiming = word.time;
        int reconstructedPixX, reconstructedPixY;
        int stripSaveX, stripSaveY;
        double timingSave;
        // Sector position
        if (dutAxisRotation != 90)
        {
            reconstructedPixX =  (word.word & 0x3F) * cfg.sectorSize;
            reconstructedPixY = -1; // Only strip X info
        }
        else
        {
            reconstructedPixX =  -1;
            reconstructedPixY = (word.word & 0x3F) * cfg.sectorSize; // Only strip Y info
        }
        __uint128_t x = word.word >> 6; 
        int wrongnHits = __builtin_popcountll(x);
        uint32_t mask = (1u << cfg.wordSize) - 1;
        int count = 0;
        while (x) 
        {
            nHits += (x & mask);
            x >>= cfg.wordSize;

            if (dutAxisRotation != 90)
            {
                stripSaveX = reconstructedPixX + count;
                stripSaveY = -1;
            }
            else
            {
                stripSaveX = -1;
                stripSaveY = reconstructedPixY + count;
            }
            timingSave = reconstructedTiming;
            count++;

            positionTree.push_back({planeID, stripSaveX, stripSaveY, timingSave});
        }
        hitTree.push_back({planeID, reconstructedPixX, reconstructedPixY, timingSave, nHits});
    }
    return {hitTree, positionTree};
}
double GetTimingOffset(double amplitude, double threshold, double T, double Tdiv, double TrefThr, double x0, double n, double t0) 
{
    if (amplitude < threshold) // if less than than threshold 
    {
        return Tdiv; // set to 200 ns delay if less than threshold (function diverges at threshold)
    }
    return T / pow((amplitude * TrefThr/threshold) - x0, n) + t0;
}
double GetFrontEndJitter(double charge)
{
   double out = 5.14 * std::exp(-charge/149.13) + 2264.86 * std::exp(-charge/15.05) + 0.2;
   if (out < 0) out = 0;
   return out;
}
std::vector<int> getSetBitPositions(__uint128_t maltaPixel, int groupSize)
// Checks for all flipped bits in a 16-bit word and returns their positions in a vector
{
    std::vector<int> positions;
    for (int i = 0; i < groupSize; i++) {
        if (maltaPixel & (__uint128_t(1) << i)) {
            positions.push_back(i);
        }
    }
    return positions;
}
__uint128_t encodeWord(int pixX, int pixY, int groupSizeX, int groupSizeY , int groupLeng, int parityLeng, int dColLeng, bool verbose)
{
    __uint128_t maltaDColumn, maltaGroup, maltaDelay, maltaParity, maltaPixel;
    // Lesson learned UInt_t is 32 bit word. All the most meaningful bits above that get truncated. Use 64 bits ULong or even larger for future?
    
    maltaDColumn = pixX / groupSizeY;
    maltaGroup = pixY / (groupSizeX *2);
    maltaDelay = 1;
    maltaParity = (pixY /groupSizeX) %2; 
    maltaPixel = 0;
    maltaPixel ^= (__uint128_t(1) << ( (pixY % groupSizeX) + (groupSizeX * (pixX % groupSizeY)) ));
    //UInt_t PIXEL_MASK   = (1u << (groupSizeX *groupSizeY))   - 1;
    /*
    ULong64_t PIXEL_MASK   = (UInt_t)((1ULL << (groupSizeX * groupSizeY)) - 1ULL);
    ULong64_t GROUP_MASK   = (UInt_t)((1ULL << groupLeng) - 1ULL);
    ULong64_t PARITY_MASK  = (UInt_t)((1ULL << parityLeng) - 1ULL);
    ULong64_t DCOLUMN_MASK = (UInt_t)((1ULL << dColLeng) - 1ULL);
    ULong64_t word = 0;
    */
    __uint128_t PIXEL_MASK   = (__uint128_t(1) << (groupSizeX * groupSizeY)) - 1;
    __uint128_t GROUP_MASK   = (__uint128_t(1) << groupLeng) - 1;
    __uint128_t PARITY_MASK  = (__uint128_t(1) << parityLeng) - 1;
    __uint128_t DCOLUMN_MASK = (__uint128_t(1) << dColLeng) - 1;
    __uint128_t word = 0;

    word |= (maltaPixel & PIXEL_MASK)         << (dColLeng + parityLeng + groupLeng); // shift left by 14 bits
    word |= (maltaGroup & GROUP_MASK)         << (dColLeng + parityLeng);     // shift left by 9 bits
    word |= (maltaParity & PARITY_MASK)       << dColLeng;           // shift left by 8 bits
    word |= (maltaDColumn & DCOLUMN_MASK);                    // stays in lower 8 bits
    if (verbose)
    {
        std::cout << "DColumn: " << std::bitset<6>(maltaDColumn) << "; Group: " << std::bitset<5>(maltaGroup) << "; Parity: " << std::bitset<1>(maltaParity) << "; MaltaPixel: " << std::bitset<64>(maltaPixel) << std::endl;
        std::cout << "Encoded word: " << std::bitset<77>(word) << std::endl;
    }
    return word; 
}
std::vector<__uint128_t> decodingMaskMSB(__uint128_t word, const std::vector<int>& field_sizes)
{
    // Simplified description of the code operation for nominal MALTA parameters:
    /*
    UInt_t maltaPixel   = (word >> (5 + 1 + 8)) & 0xFFFF; // top 16 bits
    UInt_t maltaGroup   = (word >> (1 + 8))   & 0x1F;     // next 5 bits
    UInt_t maltaParity  = (word >> 8)         & 0x1;      // 1 bit
    UInt_t maltaDColumn =  word               & 0xFF;     // last 8 bits
    */
    std::vector<__uint128_t> fields;
    int total_bits = 0;
    for (int s : field_sizes) total_bits += s;
    int shift = total_bits;

    for (int size : field_sizes)
    {
        shift -= size;
        __uint128_t mask = (__uint128_t(1) << size) - 1;
        __uint128_t value = (word >> shift) & mask;
        fields.push_back(value);
    }

    return fields;
}
std::vector<DecodedHit> decodedDigitalWord(__uint128_t word, AnaFlags cfg)
{
    std::vector<int> field_sizes = {cfg.groupSize, cfg.groupLeng, cfg.parityLeng, cfg.dColLeng};
    auto decodedWords   = decodingMaskMSB(word, field_sizes);
    __uint128_t maltaPixel   = decodedWords[0];
    __uint128_t maltaGroup   = decodedWords[1];
    __uint128_t maltaParity  = decodedWords[2];
    __uint128_t maltaDColumn = decodedWords[3]; 

    std::vector<DecodedHit> pixelPositions;
    std::vector<int> hitInGroup = getSetBitPositions(maltaPixel, cfg.groupSize);

    int nHits = 0;
    for (int hit :hitInGroup)
    {
        nHits ++;
        int x_subgroup = hit / cfg.groupSizeX;      // 0 .. (groupSizeY-1)
        int y_in_subgroup = hit % cfg.groupSizeX;   // 0 .. (groupSizeX-1)

        int pixX = maltaDColumn * cfg.groupSizeY + x_subgroup;
        int pixY = maltaGroup * (cfg.groupSizeX * 2) + maltaParity * cfg.groupSizeX + y_in_subgroup;

        pixelPositions.push_back({pixX, pixY, nHits});
    }
    
    return pixelPositions;
}
std::vector<uint32_t> CompressWords(std::vector<uint32_t> vals, int targetWidth)
{
    int bitWidth = 4;
    while (vals.size() > 1 && bitWidth < targetWidth)
    {
        std::vector<uint32_t> next;

        for (size_t i = 0; i < vals.size(); i += 2)
        {
            if (i + 1 < vals.size())
                next.push_back(vals[i] + vals[i + 1]);
            else
                next.push_back(vals[i]);
        }

        vals = std::move(next);
        bitWidth++;
    }

    return vals;
}
ThresholdMap generateThrMap(double inputThreshold, int runNumber, std::string saveName, AnaFlags cfg, unsigned int seed)
{
    // This is an example data input for data sim threshold dispersion validation
    // Thr 967
    //std::vector<double> vThrMeanData = {959.2, 1004.8, 986.9, 1003.7, 1030.1, 1037.2, 1002.7, 983.7, 938.2, 952.7, 976.5, 943.4, 960.7, 930.8, 884.3, 875.2};
    // Thr 200
    std::vector<double> vThrMeanData = {228.594,220.652,223.642,236.398,230.506,242.855,235.05,228.462,233.891,226.979,218.171,214.329,223.578,207.099,209.558,210.827};

    // Initialize from config
    int pixXNum = cfg.xPix;
    int pixYNum = cfg.yPix;
    int groupRepetition = cfg.mirrorRepetition;
    double relativeThresholdSmearingCol = cfg.colSmearing;
    double relativeThresholdSmearingMean = cfg.meanSmearing;
    std::string directoryPath = cfg.localPath + "Plots/";
    std::string runPath = Form("local_%04d/", runNumber);

    // Save the threshold 
    TH1D *h1DThreshold =  new TH1D("h1DUTThreshold", "h1DThreshold", 100, inputThreshold - inputThreshold / 2,inputThreshold + inputThreshold / 2);
    TH2D *h2DThreshold    = new TH2D("h2DUTThreshold", "h2DUTThreshold", 512, 0, 512, 512, 0, 512);

    ThresholdMap result;

    std::mt19937 gen(seed);
    std::normal_distribution<> meanDist(1.0, relativeThresholdSmearingMean);
    std::normal_distribution<> colDist(1.0, relativeThresholdSmearingCol);

    //TODO: Improve Complexity
    for (int i = 0; i< pixXNum/groupRepetition; i++)
    {
        double thresholdMean = inputThreshold * meanDist(gen);
        for (int x = 0; x < pixXNum; x++)
        {
            for (int y = 0; y < pixYNum; y++)
            {
                if (x / groupRepetition == i)
                {
                    //static std::mt19937 gen(std::random_device{}());
                    double thresholdCol = thresholdMean * colDist(gen);
                    result[{x,y}] = thresholdCol;
                    h1DThreshold->Fill(thresholdCol);
                    h2DThreshold->Fill(x, y, thresholdCol);
                }
            }
        }
    }
    savePlot(directoryPath, runPath, inputThreshold, saveName, h1DThreshold, "h1DThreshold");
    savePlot(directoryPath, runPath, inputThreshold, saveName, h2DThreshold, "h2DThreshold");

    return result;
}
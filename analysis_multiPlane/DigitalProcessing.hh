#pragma once
#include <vector>
#include <utility>
#include <cstdint>
#include <iostream>
#include <ROOT/RNTuple.hxx>
#include <TH3D.h>
#include <TFile.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <chrono>
#include <random>
#include "ConfigAnalysis.hh"
//#include "Tracking.hh"
#include "CaloPreProcessing.hh"


void set_style() {
    gStyle->SetOptStat(0);
    gStyle->SetPalette(112);
    gStyle->SetNumberContours(255);
    // gStyle->SetPalette(1); // old default rainbow palette, optional
    gROOT->SetBatch(kTRUE);
}

// Save diagnostic plots
inline void savePlot (std::string directoryPath, std::string runPath, double threshold, std::string saveName, TH1* hist, std::string histName)
{
    // TODO this doesnt work as I want it to. 
    set_style();
    TCanvas *c2D = new TCanvas(histName.c_str(), histName.c_str(), 800, 800);
    std::string filePath = directoryPath + runPath + saveName + "/histos.root";

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

inline std::string getVarFromConfig()
{
    //Get path stored in config.sh
    std::string cmd = "bash -c 'source config.sh && echo $LOCAL_PATH'";

    std::array<char, 128> buffer;
    std::string result;

    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return "";
    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        result += buffer.data();
    }
    pclose(pipe);

    // strip newline
    if (!result.empty() && result.back() == '\n')
        result.pop_back();
    //std::cout << result << std::endl;
    return result;
}

// Returns timing offset for a given amplitude
double GetTimingOffset(double amplitude, double threshold, double T, double Tdiv, double TrefThr, double x0, double n, double t0) 
{
    if (amplitude < threshold) // if less than than threshold 
    {
        return Tdiv; // set to 200 ns delay if less than threshold (function diverges at threshold)
    }
    return T / pow((amplitude * TrefThr/threshold) - x0, n) + t0;
}

// Returns positions of set bits in a 16-bit word
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

// Customizable mask for decoding digital subwords in a MALTA word.
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

// Decodes a digital word into pixel positions and hit counts
std::vector< std::pair<std::pair<int,int>, int> > decodedDigitalWord(__uint128_t word, int groupSize,int groupSizeX, int groupSizeY, int groupLeng, int parityLeng, int dColLeng)
{
    //Expert debug statement. Should not come up unless modifications on the digitization logic are made.
    bool debug = false;
    std::vector<int> field_sizes = {groupSize, groupLeng, parityLeng, dColLeng};
    auto decodedWords   = decodingMaskMSB(word, field_sizes);
    __uint128_t maltaPixel   = decodedWords[0];
    __uint128_t maltaGroup   = decodedWords[1];
    __uint128_t maltaParity  = decodedWords[2];
    __uint128_t maltaDColumn = decodedWords[3]; 

    std::vector <std::pair<std::pair<int,int>, int> > pixelPositions;
    std::vector<int> hitInGroup = getSetBitPositions(maltaPixel, groupSize);

    if(debug)
    {
        std::cout << "field_sizes: " << groupSize << " ; " << groupLeng << " ; " << parityLeng << " ; " << dColLeng << std::endl;

        std::cout << "-------------------------------" << std::endl;
        std::cout << "Input word: " << std::bitset<30>(word) << std::endl;
        std::cout << "Decoded Pixel word: " << std::bitset<16>(maltaPixel) << std::endl;
        std::cout << "Decoded Group: " << std::bitset<5>(maltaGroup) << std::endl; 
        std::cout << "Decoded Parity: " << std::bitset<1>(maltaParity) << std::endl;
        std::cout << "Decoded DColumn: " << std::bitset<8>(maltaDColumn) << std::endl;
    }
    
    int nHits = 0;
    for (int hit :hitInGroup)
    {
        nHits ++;
        /*
        int x_half = (hit >= groupSizeX) ? 1 : 0;
        int pixX = maltaDColumn *groupSizeY + x_half;
        int y_in_half = hit % groupSizeX;
        int pixY = maltaGroup *(groupSizeX * groupSizeY) + groupSizeX * maltaParity + y_in_half;
        */
        int x_subgroup = hit / groupSizeX;      // 0 .. (groupSizeY-1)
        int y_in_subgroup = hit % groupSizeX;   // 0 .. (groupSizeX-1)

        int pixX = maltaDColumn * groupSizeY + x_subgroup;
        int pixY = maltaGroup * (groupSizeX * 2) + maltaParity * groupSizeX + y_in_subgroup;

        pixelPositions.push_back(std::make_pair(std::make_pair(pixX, pixY), nHits));
        if (debug)    
        {
            std:cout << "Hit: " << hit << std::endl;
            std::cout << "Decoded pixel position: (" << pixX << ", " << pixY << ")" << std::endl;
        }
    }
    if (debug) std::cout << "-------------------------------" << std::endl;
    
    return pixelPositions;
}

void digitalTest(std::vector<std::pair<int,int>> hits)
{
    // Takes as input a set of hit positions and outputs the merged hit positions
    __uint128_t mergedWord = 0;
    for(auto& hit: hits)
    {
        std::cout << "Input Hits: X: " << hit.first << "; Y: " << hit.second << std::endl; 
        __uint128_t word = encodeWord(hit.first, hit.second, 8, 2, 5, 1, 8, false);
        std::cout << "Encoded word: " << std::bitset<30>(word) << std::endl;
        mergedWord |= word;
    }
    std::cout << "Merged word:  " << std::bitset<30>(mergedWord) << std::endl;

    std::vector< std::pair<std::pair<int,int>, int> > pixPos = decodedDigitalWord(mergedWord, 16, 8, 2, 5, 1, 8);
    for (const auto& pos : pixPos) 
    {
        int reconstructedPixX = pos.first.first;
        int reconstructedPixY = pos.first.second;

        std::cout << "Reconstructed hit: X: " << reconstructedPixX << " ; Y: " << reconstructedPixY << std::endl;
    }
}

    
    std::map<std::pair<int,int>, double> generateThrMap(double inputThreshold, int pixXNum, int pixYNum, 
                                                        int groupRepetition, double relativeThresholdSmearingCol, 
                                                        double relativeThresholdSmearingMean, std::string directoryPath, 
                                                        std::string runPath, std::string saveName)
    {
        // This is an example data input for data sim threshold dispersion validation
        // Thr 967
        //std::vector<double> vThrMeanData = {959.2, 1004.8, 986.9, 1003.7, 1030.1, 1037.2, 1002.7, 983.7, 938.2, 952.7, 976.5, 943.4, 960.7, 930.8, 884.3, 875.2};
        // Thr 200
        std::vector<double> vThrMeanData = {228.594,220.652,223.642,236.398,230.506,242.855,235.05,228.462,233.891,226.979,218.171,214.329,223.578,207.099,209.558,210.827};

        // Save the threshold 
        TH1D *h1DThreshold =  new TH1D("h1DUTThreshold", "h1DThreshold", 100, inputThreshold - inputThreshold / 2,inputThreshold + inputThreshold / 2);
        TH2D *h2DThreshold    = new TH2D("h2DUTThreshold", "h2DUTThreshold", 512, 0, 512, 512, 0, 512);
        std::map<std::pair<int,int>, double> thresholdMap;
        for (int i = 0; i< pixXNum/groupRepetition; i++)
        {
            static std::mt19937 gen(std::random_device{}());
            std::normal_distribution<> dist(1.0, relativeThresholdSmearingMean);
            double thresholdMean = inputThreshold * dist(gen);
            //std::cout << thresholdMean << std::endl;
            //thresholdMean = vThrMeanData[i];
            for (int x = 0; x < pixXNum; x++)
            {
                for (int y = 0; y < pixYNum; y++)
                {
                    if (x / groupRepetition == i)
                    {
                        static std::mt19937 gen(std::random_device{}());
                        std::normal_distribution<> dist(1.0, relativeThresholdSmearingCol);
                        double thresholdCol = thresholdMean * dist(gen);
                        thresholdMap[{x,y}] = thresholdCol;
                        h1DThreshold->Fill(thresholdCol);
                        h2DThreshold->Fill(x, y, thresholdCol);
                    }
                }
            }
        }
        savePlot(directoryPath, runPath, inputThreshold, saveName, h1DThreshold, "h1DThreshold");
        savePlot(directoryPath, runPath, inputThreshold, saveName, h2DThreshold, "h2DThreshold");

        return thresholdMap;
    }

// This is a user specific function that changes the pixel position based on the plane number.
// In this case the XOffset and YOffset is different for every second planeZ position.
inline std::pair<double,double> GetSpecificPlaneOffset(int plane, std::string geometry)
{
    int plane_1 = plane%10;
    int plane_10 = plane%100/10;
    int plane_100 = plane%1000/100;
    double XOffset_global = 0.;
    double YOffset_global = 0.;
    if (geometry=="LHCf"){
        if (plane_100%2==1){// only apply for every second z-coordinate
            XOffset_global = 0.2; // 0.2 mm
            YOffset_global = -0.2; // 0.2 mm
        }
    }
    return {XOffset_global, YOffset_global};
}

// Main digital processing function
void DigitalProcessing(double threshold = 2, int runNumber = 91, std::string saveName = "default", bool proteusFlag = false);
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
#include "Tracking.hh"


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
std::vector<int> getSetBitPositions(uint16_t maltaPixel, int groupSize)
// Checks for all flipped bits in a 16-bit word and returns their positions in a vector
{
    std::vector<int> positions;
    for (int i = 0; i < groupSize; i++) {
        if (maltaPixel & (1 << i)) {
            positions.push_back(i);
        }
    }
    return positions;
}

UInt_t encodeWord(int pixX, int pixY, bool verbose)
{
    UInt_t maltaDColumn, maltaGroup, maltaDelay, maltaParity, maltaPixel;
    // TODO: Bit length not yet generalized
    maltaDColumn = pixX / 2;
    maltaGroup = pixY / 16;
    maltaDelay = 1;
    maltaParity = (pixY /8) %2; 
    maltaPixel = 0b0000000000000000;
    //maltaPixel ^= (1 << pixY % 8 + 8 *(pixX %2)); //* (pixX % 2 +1));
    maltaPixel ^= (1 << ( (pixY % 8) + (8 * (pixX % 2)) ));
    UInt_t word = 0;
    word |= (maltaPixel & 0xFFFF) << (5 + 1 + 8); // shift left by 14 bits
    word |= (maltaGroup & 0x1F)       << (1 + 8);     // shift left by 9 bits
    word |= (maltaParity & 0x1)       << 8;           // shift left by 8 bits
    word |= (maltaDColumn & 0xFF);                    // stays in lower 8 bits
    if (verbose)
    {
        std::cout << "DColumn: " << std::bitset<8>(maltaDColumn) << "; Group: " << std::bitset<5>(maltaGroup) << "; Parity: " << std::bitset<1>(maltaParity) << "; MaltaPixel: " << std::bitset<16>(maltaPixel) << std::endl;
        std::cout << "Encoded word: " << std::bitset<32>(word) << std::endl;
    }
    return word; 
}

// Customizable mask for decoding digital subwords in a MALTA word.
std::vector<UInt_t> decodingMaskMSB(UInt_t word, const std::vector<int>& field_sizes)
{
    // Simplified description of the code operation for nominal MALTA parameters:
    /*
    UInt_t maltaPixel   = (word >> (5 + 1 + 8)) & 0xFFFF; // top 16 bits
    UInt_t maltaGroup   = (word >> (1 + 8))   & 0x1F;     // next 5 bits
    UInt_t maltaParity  = (word >> 8)         & 0x1;      // 1 bit
    UInt_t maltaDColumn =  word               & 0xFF;     // last 8 bits
    */
    std::vector<UInt_t> fields;
    int total_bits = 0;
    for (int s : field_sizes) total_bits += s;
    int shift = total_bits;

    for (int size : field_sizes)
    {
        shift -= size;
        UInt_t mask = (1u << size) - 1u;
        UInt_t value = (word >> shift) & mask;
        fields.push_back(value);
    }

    return fields;
}

// Decodes a digital word into pixel positions and hit counts
std::vector< std::pair<std::pair<int,int>, int> > decodedDigitalWord(UInt_t word, int groupSize, int groupLeng, int parityLeng, int dColLeng)
{
    //Expert debug statement. Should not come up unless modifications on the digitization logic are made.
    bool debug = false;
    std::vector<int> field_sizes = {groupSize, groupLeng, parityLeng, dColLeng};
    auto decodedWords   = decodingMaskMSB(word, field_sizes);
    UInt_t maltaPixel   = decodedWords[0];
    UInt_t maltaGroup   = decodedWords[1];
    UInt_t maltaParity  = decodedWords[2];
    UInt_t maltaDColumn = decodedWords[3]; 

    std::vector <std::pair<std::pair<int,int>, int> > pixelPositions;
    std::vector<int> hitInGroup = getSetBitPositions(maltaPixel, groupSize);

    if(debug)
    {
        std::cout << "-------------------------------" << std::endl;
        std::cout << "Input word: " << std::bitset<32>(word) << std::endl;
        std::cout << "Decoded Pixel word: " << std::bitset<16>(maltaPixel) << std::endl;
        std::cout << "Decoded Group: " << std::bitset<5>(maltaGroup) << std::endl; 
        std::cout << "Decoded Parity: " << std::bitset<1>(maltaParity) << std::endl;
        std::cout << "Decoded DColumn: " << std::bitset<8>(maltaDColumn) << std::endl;
    }
    
    int nHits = 0;
    for (int hit :hitInGroup)
    {
        nHits ++;
        // TODO: Source of errors when the bit sizes change. Revisit for further implementation
        int pixX = maltaDColumn *2 + hit /8;
        int pixY = maltaGroup *16 + 8 * maltaParity + hit %8;
        pixelPositions.push_back(std::make_pair(std::make_pair(pixX, pixY), nHits));
        if (debug)    
        {
            std::cout << "Decoded pixel position: (" << pixX << ", " << pixY << ")" << std::endl;
        }
    }
    if (debug) std::cout << "-------------------------------" << std::endl;
    
    return pixelPositions;
}

void digitalTest(std::vector<std::pair<int,int>> hits)
{
    // Takes as input a set of hit positions and outputs the merged hit positions
    UInt_t mergedWord = 0;
    for(auto& hit: hits)
    {
        std::cout << "Input Hits: X: " << hit.first << "; Y: " << hit.second << std::endl; 
        UInt_t word = encodeWord(hit.first,hit.second, false);
        std::cout << "Encoded word: " << std::bitset<30>(word) << std::endl;
        mergedWord |= word;
    }
    std::cout << "Merged word:  " << std::bitset<30>(mergedWord) << std::endl;

    std::vector< std::pair<std::pair<int,int>, int> > pixPos = decodedDigitalWord(mergedWord, 16, 5, 1, 8);
    for (const auto& pos : pixPos) 
    {
        int reconstructedPixX = pos.first.first;
        int reconstructedPixY = pos.first.second;

        std::cout << "Reconstructed hit: X: " << reconstructedPixX << " ; Y: " << reconstructedPixY << std::endl;
    }
}




// Main digital processing function
void DigitalProcessing(double threshold = 2, int runNumber = 91, std::string saveName = "default", bool proteusFlag = false);
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
// Returns timing offset for a given amplitude
double GetTimingOffset(double amplitude, double threshold);

// Save diagnostic plots
inline void savePlot (std::string directoryPath, std::string runPath, double threshold, std::string saveName, TH1* hist, std::string histName)
{
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

// Returns positions of set bits in a 16-bit word
std::vector<int> getSetBitPositions(uint16_t maltaPixel);

// Decodes a digital word into pixel positions and hit counts
std::vector< std::pair<std::pair<int,int>, int> > decodedDigitalWord(unsigned int word);

inline std::string getVarFromConfig()
{
    //Get path stored in config.sh
    
    //std::string cmd = "bash -c 'source " + configFile + " && echo $" + var + "'";

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

// Main digital processing function
void DigitalProcessing(double threshold = 2, int runNumber = 91, std::string saveName = "default", bool proteusFlag = false);
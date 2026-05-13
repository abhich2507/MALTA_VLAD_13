#pragma once
#include <string>
#include <array>
#include <vector>
#include <filesystem>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <algorithm>
#include <thread>
#include <chrono>
#include <iostream>



struct AnaFlags
{
    double T = 0.;
    double Tdiv = 0.;
    double TrefThr = 0.;
    double x0 = 0.;
    double n = 0.;
    double t0 = 0.;
    double scintillatorJitter = 0.;
    double samplingJitter = 0.;
    int groupSize = 0;
    int groupSizeX = 0;
    int groupSizeY = 0;
    int groupLeng = 0;
    int parityLeng = 0;
    int dColLeng = 0;
    int numThreads = 0;
    double chLoss = 0.;
    int xPix = 0;
    int yPix = 0;
    int mirrorRepetition = 0;
    double meanSmearing = 0.;
    double colSmearing = 0.;
    double wordSpacing = 0.;
    double distCut = 0.;
    double timeCut = 0.;
    bool trkUnc = false;
    std::string clPos = "";
    bool verboseDigital = false;
    bool verboseTracking = false;
    bool verboseClustering = false;
    bool verboseAnalysis = false;
    double trackOffsetX = 0.;
    double trackOffsetY = 0.;
    double veto = 0.;
    double slowcontrolDelay = 0.;
    double busMergingThreshold = 0.;
    double SRAMFrequency = 0.;
    int sramDepth = 0;
    double FIFOFrequency = 0.;
    int FIFOSize = 0;
    bool boolHWC = false;
    int sectorSize = 0;
    int wordSize = 0;
    double fifoSize = 0.;
    std::string prioAlgo = "";

    double fifoFrequency = 0.;
    int fifoMultiplicity = 0;
    int nPlanes_100 = 0;
    int nPlanes_10 = 0;
    int nPlanes_1 = 0;
    int modules = 0;
    std::string simProc = "";
    std::string geoFile = "";
    std::string localPath = "";
    std::string inputPath = "";
    std::string fileName = "";
    std::string MCTrueTree = "TruthVertex";
    std::string geometry = "";
    double Analysis_XCenter = 0.;
    double Analysis_YCenter = 0.;
    double Analysis_XWidth = 0.;
    double Analysis_YWidth = 0.;

};

void LoadAnalysisFlagsFromFile(const std::string& filename, AnaFlags& flags);
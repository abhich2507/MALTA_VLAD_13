#pragma once
#include <utility>
#include <iostream>
#include <ROOT/RNTuple.hxx>
#include <TH3D.h>
#include <TFile.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <chrono>
#include "DigitalProcessing.hh"

// Structure to hold track information
struct TrackEntry {
    double x, y, z, t;
};

struct DetectorConfig {
    double detectorXOffset;
    double detectorYOffset;
    double pixelSize;
    double detectorSizeX;
    double detectorSizeY;
};

DetectorConfig LoadConfig(const std::string& configPath);

// Reconstruct position of planes from the config file
inline std::pair<double,double> PixelPositionReconstruction(int pixelX, int pixelY, const DetectorConfig& cfg);

// Main tracking function
void Tracking(int runNumber, std::string saveName);
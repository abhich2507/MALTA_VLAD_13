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
struct TrackEntry 
{
    double x, y, z, t;
};

struct DetectorConfig 
{
    double detectorXOffset;
    double detectorYOffset;
    double pixelSize;
    double detectorSizeX;
    double detectorSizeY;
};

DetectorConfig LoadConfig(const std::string& configPath) 
{
    std::ifstream infile(configPath);
    std::map<std::string, double> config;
    std::string line;

    while (std::getline(infile, line)) {
        if (line.empty() || line[0] == '#' || line.rfind("//",0) == 0) continue;
        std::istringstream iss(line);
        std::string key, eq;
        double value;
        if (iss >> key >> eq >> value && eq == "=") {
            config[key] = value;
        }
    }
    // Import detector configuration 
    DetectorConfig dc;
    dc.detectorXOffset = config["detectorXOffset"] * 10;
    dc.detectorYOffset = config["detectorYOffset"] * 10;
    dc.pixelSize       = config["pixelSize"];
    dc.detectorSizeX   = config["detectorSizeX"] * 10;
    dc.detectorSizeY   = config["detectorSizeY"] * 10;
    return dc;
}

// Reconstruct position of planes from the config file
// return global coordinates in mm of the pixel center in which hit occured.
inline std::pair<double,double> PixelPositionReconstruction(int pixelX, int pixelY, const DetectorConfig& cfg)
{
    // detectorXOffset, detectorYOffset is that of the center of plane0
    double xGlobal = pixelX * cfg.pixelSize + cfg.detectorXOffset - cfg.detectorSizeX / 2 + cfg.pixelSize /2;
    double yGlobal = pixelY * cfg.pixelSize + cfg.detectorYOffset - cfg.detectorSizeY / 2 + cfg.pixelSize /2;
    return {xGlobal, yGlobal};
}

// Main tracking function
void Tracking_multiPlane(int runNumber, std::string saveName);
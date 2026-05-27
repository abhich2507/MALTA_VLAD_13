#include "Utils.hh"
#include <TStyle.h>
#include <TROOT.h>
#include <TFile.h>
#include <TTree.h>
#include <array>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>


void set_style() 
{
    gStyle->SetOptStat(0);
    gStyle->SetPalette(112);
    gStyle->SetNumberContours(255);
    // gStyle->SetPalette(1); // old default rainbow palette, optional
    gROOT->SetBatch(kTRUE);
}
AnaFlags GetDigitalConfig()
{
    AnaFlags analysisFlags{};
    const char* configPath = std::getenv("ANALYSIS_CONFIG");
    LoadAnalysisFlagsFromFile(configPath, analysisFlags);
    return analysisFlags;
}
std::string getVarFromConfig()
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
std::vector<Module> LoadModules(const std::string& filename)
{
    std::ifstream file("configs/geometry/" + filename);
    std::vector<Module> modules;

    if (!file) 
    {
        throw std::runtime_error("Cannot open config file!!!!!");
    }

    std::string line;


    // skip header
    std::getline(file, line);

    while (std::getline(file, line))
    {
        if (line.empty()) continue;

        std::stringstream ss(line);
        std::string value;
        Module m;

        // read comma-separated values
        std::getline(ss, value, ','); m.x    = std::stoi(value);
        std::getline(ss, value, ','); m.y    = std::stoi(value);
        std::getline(ss, value, ','); m.z    = std::stoi(value);
        std::getline(ss, value, ','); m.xoff = std::stod(value);
        std::getline(ss, value, ','); m.yoff = std::stod(value);
        std::getline(ss, value, ','); m.zoff = std::stod(value);
        std::getline(ss, value, ','); m.xrot = std::stod(value);
        std::getline(ss, value, ','); m.yrot = std::stod(value);
        std::getline(ss, value, ','); m.zrot = std::stod(value);
        std::getline(ss, value, ','); m.modID   = std::stoi(value);  // "mod" column

        modules.push_back(m);
    }

    return modules;
}
std::map<int, Offset> LoadGeometry(const std::string& geoPath, const DetectorConfig& cfg)
{
    auto planes = LoadModules(geoPath);
    std::map<int, Offset> moduleMap;
    for (auto& plane: planes)
    {
        int planeX = plane.x;
        int planeY = plane.y;
        int planeZ = plane.z;
        double xoff = plane.xoff;
        double yoff = plane.yoff;
        double zoff = plane.zoff;
        double xrot = plane.xrot;
        double yrot = plane.yrot;
        double zrot = plane.zrot;
        int planeID = planeZ *10000 + planeY * 100 + planeX;

        Offset off;
        off.x = xoff + cfg.detectorXOffset;
        off.y = yoff + cfg.detectorYOffset;
        off.z = zoff;
        off.xrot = xrot;
        off.yrot = yrot;
        off.zrot = zrot;
        //std::cout << "planeID: " << planeID << " x: " << xoff << " y: " << yoff << " z: " << zoff << std::endl;
        moduleMap[planeID] = off;
    }
    return moduleMap;
}
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
    dc.momX            = config["particleMomentumX"];
    dc.momY            = config["particleMomentumY"];
    dc.momZ            = config["particleMomentumZ"];
    return dc;
}
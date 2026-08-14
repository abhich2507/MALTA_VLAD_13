#include "Config.h"
#include <array>
#include <cassert>
#include <filesystem>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <algorithm>
#include <chrono>
#include <iostream>
#include "G4UnitsTable.hh"
#include "G4SystemOfUnits.hh"

void DumpConfigToFile(const std::string& filename) 
{
    // Copy the config file used as input (from SIMU_CONFIG) to 'filename'
    const std::string configPath = std::getenv("SIMU_CONFIG");
    if (configPath.c_str() && std::filesystem::exists(configPath)) 
    {
        try 
        {
            std::filesystem::copy_file(configPath, filename, std::filesystem::copy_options::overwrite_existing);
            std::cout << "Copied config file from " << configPath << " to " << filename << std::endl;
        } catch (const std::exception& e) 
        {
            std::cerr << "Failed to copy config file: " << e.what() << std::endl;
        }
    } 
    else 
    {
        std::cerr << "SIMU_CONFIG not set or file does not exist, skipping config copy." << std::endl;
    }
}


std::string CreateNextRunDirectory(bool flag, const SimFlags* fFlags) 
{   
    namespace fs = std::filesystem;
    std::string path =  "";
    std::string saveFile;
    std::ostringstream newRunDir;
    // Check if the run directory needs to be saved locally or on the cloud. This flag is set in sim.cc based on the directory. 
    // It is not fully generalized so it might trow errors depending on the users home directory.
    assert(fFlags != nullptr);

    if(fFlags->runMode == "local") 
    {
        path = fFlags->outputPathLocal;
        saveFile = "/local_";
    }
    else
    {
        path = fFlags->outputPathNAF;
        saveFile = "/naf_";
    }
    int maxRun = 0;

    for (const auto& entry : fs::directory_iterator(path)) 
    {
        std::string name = entry.path().filename().string();
        try
        {
            int digits = std::stoi(name.substr(name.size() - 4));
            if (maxRun < digits)
            {
                maxRun = digits;
            }
        }
        catch(const std::exception& e)
        {
            continue;
        }
    }

    if (flag)
    {   
        // Do this only in the Master thread
        newRunDir << path << saveFile << std::setw(4) << std::setfill('0') << maxRun + 1;
        std::cout <<"Creating Directory: " << newRunDir.str() << std::endl;
        fs::create_directories(newRunDir.str());
    }
    else
    {
        newRunDir << path << saveFile << std::setw(4) << std::setfill('0') << maxRun;
    }

    return newRunDir.str();
}

void LoadSimFlagsFromFile(const std::string& filename, SimFlags& flags)
{
    std::cout << "Loading flags from file: " << filename << std::endl;
    std::ifstream infile(filename);
    std::string line;
    while (std::getline(infile, line)) {
        // Remove whitespace
        line.erase(std::remove_if(line.begin(), line.end(), ::isspace), line.end());
        if (line.empty() || line[0] == '#') continue; // skip comments/empty lines
        auto pos = line.find('=');
        if (pos == std::string::npos) continue;
        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1);

        if (key == "preDefinedGeometryFlag") flags.preDefinedGeometryFlag = value;
        else if (key == "largeScaleFlag") flags.largeScaleFlag = value;
        else if (key == "geoFile") flags.geoFile = value;
        else if (key == "detectorXOffset") flags.detectorXOffset = std::stof(value);
        else if (key == "detectorYOffset") flags.detectorYOffset = std::stof(value);
        else if (key == "detectorZOffset") flags.detectorZOffset = std::stof(value);
        else if (key == "pixelSize") flags.pixelSize = std::stof(value);
        else if (key == "detectorSizeX") flags.detectorSizeX = std::stof(value);
        else if (key == "detectorSizeY") flags.detectorSizeY = std::stof(value);
        else if (key == "detectorDepth") flags.detectorDepth = std::stof(value);
        else if (key == "CCModelSigmaX") flags.CCModelSigmaX = std::stof(value);
        else if (key == "CCModelSigmaY") flags.CCModelSigmaY = std::stof(value);
        else if (key == "outsideMaterial") flags.outsideMaterial = value;
        else if (key == "beamGeometry") flags.beamGeometry = value;
        else if (key == "gausSmearing") flags.gausSmearing = std::stof(value);
        else if (key == "beamXOffset") flags.beamXOffset = std::stof(value);
        else if (key == "beamYOffset") flags.beamYOffset = std::stof(value);
        else if (key == "beamZOffset") flags.beamZOffset = std::stof(value);
        else if (key == "sourceRadius") flags.sourceRadius = std::stof(value);
        else if (key == "sourceRadiusX") flags.sourceRadiusX = std::stof(value);
        else if (key == "sourceRadiusY") flags.sourceRadiusY = std::stof(value);
        else if (key == "particleCount") flags.particleCount = std::stoi(value);
        else if (key == "numEvents") flags.numEvents = std::stoi(value);
        else if (key == "intraSpillOffset") flags.intraSpillOffset = std::stof(value);
        else if (key == "beamVeto") flags.beamVeto = std::stof(value);
        else if (key == "particleType") flags.particleType = value;
        else if (key == "particleEnergy") flags.particleEnergy = value;
        else if (key == "bkgparticleType") flags.bkgparticleType = value;
        else if (key == "bkgparticleEnergy") flags.bkgparticleEnergy = value;
        else if (key == "energyDistribution") flags.energyDistribution = value;
        else if (key == "particleMomentumX") flags.particleMomentumX = std::stof(value);
        else if (key == "particleMomentumY") flags.particleMomentumY = std::stof(value);
        else if (key == "particleMomentumZ") flags.particleMomentumZ = std::stof(value);
        else if (key == "itkEnable") flags.itkEnable = (value == "true");
        else if (key == "pileUpScale") flags.pileUpScale = std::stod(value);
        else if (key == "itkInput") flags.itkInput = value;
        else if (key == "itkLayer") flags.itkLayer = std::stoi(value);
        else if (key == "itkZ") flags.itkZ = std::stod(value);
        else if (key == "EMPhysics") flags.EMPhysics = (value == "true");
        else if (key == "hadronPhysics") flags.hadronPhysics = (value == "true");
        else if (key == "dutTungstenAbsorberFlag") flags.dutTungstenAbsorberFlag = (value == "true");
        else if (key == "absorberThickness") flags.absorberThickness = std::stof(value);
        else if (key == "setGEANT4Cuts") flags.setGEANT4Cuts = (value == "true");
        else if (key == "GEANT4CutValue") flags.GEANT4CutValue = std::stof(value);
        else if (key == "outputPathLocal") flags.outputPathLocal = value;
        else if (key == "outputPathNAF") flags.outputPathNAF = value;
        else if (key == "macroFileLocal") flags.macroFileLocal = value;
        else if (key == "macroFileNAF") flags.macroFileNAF = value;
        else if (key == "numThreadsLocal") flags.numThreadsLocal = std::stoi(value);
        else if (key == "numThreadsNAF") flags.numThreadsNAF = std::stoi(value);
        else if (key == "runMode") flags.runMode = value;
        else if (key == "gdmlBool") flags.gdmlBool = (value == "true");
        else if (key == "gdmlStr") flags.gdmlStr = value;
        else if (key == "verbosePL") flags.verbosePL = (value == "true");
        else if (key == "verbosePG") flags.verbosePG = (value == "true");
        else if (key == "verboseSD") flags.verboseSD = (value == "true");
        else if (key == "verboseSA") flags.verboseSA = (value == "true");

        std::cout << "Loaded flag: " << key << " = " << value << std::endl;
    }
}

std::vector<Module> LoadModules(const std::string& filename)
{
    std::ifstream file("../configs/geometry/" + filename);
    std::vector<Module> modules;

    if (!file) {
        throw std::runtime_error("Cannot open config file");
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

int GetRequestCpusFromSubmitFile(const std::string& submitFilePath) {
    std::ifstream infile(submitFilePath);
    std::string line;
    while (std::getline(infile, line)) {
        std::istringstream iss(line);
        std::string key;
        if (std::getline(iss, key, '=')) {
            if (key.find("request_cpus") != std::string::npos) {
                std::string value;
                if (std::getline(iss, value)) {
                    return std::stoi(value);
                }
            }
        }
    }
    return -1; // Not found
}

void throwWarning (const std::string& origin, 
                   const std::string& exceptionCode, 
                   const std::string& description)
{
    G4cout << "⚠️   ⚠️   ⚠️   ⚠️   ⚠️   ⚠️   ⚠️   ⚠️   ⚠️   ⚠️   ⚠️   ⚠️   ⚠️   ⚠️   ⚠️   ⚠️   ⚠️   ⚠️   ⚠️   ⚠️   ⚠️   ⚠️   ⚠️" << G4endl;
    G4Exception(origin.c_str(), exceptionCode.c_str(), JustWarning, description.c_str());
}

void throwError (const std::string& origin, 
                 const std::string& exceptionCode, 
                 const std::string& description)
{
    G4cout << "🛑  🛑  🛑  🛑  🛑  🛑  🛑  🛑  🛑  🛑  🛑  🛑  🛑  🛑  🛑  🛑  🛑  🛑  🛑  🛑  🛑  🛑  🛑" << G4endl;
    G4Exception(origin.c_str(), exceptionCode.c_str(), FatalException, description.c_str());
}
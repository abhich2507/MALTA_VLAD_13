#ifndef CONFIG_HH
#define CONFIG_HH

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
#include "G4UnitsTable.hh"
#include "G4SystemOfUnits.hh"

struct SimFlags
{
    bool isBatch = false; // This is not read from the config file, but set in the main function
    std::string preDefinedGeometryFlag = "DEBUG";
    double detectorXOffset = 0.;
    double detectorYOffset = 0.;
    double detectorZOffset = 0.;
    double pixelSize = 0.;
    double detectorSizeX =0.;
    double detectorSizeY =0.;
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 8659d76 (Updating include and configs)
=======
>>>>>>> 21a1a24 (Updating more folders with newest develop branch. Before was old version)
    double detectorDepth =0.;
=======
    double detetectorDepth =0.;
>>>>>>> 66f7594 (DEBUG)
=======
    double detectorDepth =0.;
>>>>>>> 8a2e4b8 (Cleaned up the simulation files. Removed the Truth En tree as it was not used further in the analysis chain. Additionally, general clean up in terms of branch renaming. It most probably will impact the in_pixel_plots script. However the default analysis chain has already been modified to account for these changes.)
    double CCModelSigmaX =0.;
    double CCModelSigmaY =0.;
=======
>>>>>>> b9fbad2 (Updating include and configs)
=======
    double detectorDepth =0.;
    double CCModelSigmaX =0.;
    double CCModelSigmaY =0.;
>>>>>>> b73720e (Updating more folders with newest develop branch. Before was old version)
    std::string outsideMaterial = "G4_Galactic";
    std::string beamGeometry = "";
    double beamXOffset = 0.;
    double beamYOffset = 0.;
    double beamZOffset = 0.;
    double sourceRadius =  0.;
    double sourceXOffset = 0.;
    double sourceYOffset = 0.;
    double sourceZOffset = 0.;
    int particleCount = 0;
    int numEvents = 0;
    double intraSpillOffset = 0;
    double beamVeto = 0;
    std::string particleType = "";
    std::string particleEnergy = "";
    double particleMomentumX = 0.;
    double particleMomentumY = 0.;
    double particleMomentumZ = 0.;
    bool EMPhysics = true;
    bool hadronPhysics = true;
    bool setGEANT4Cuts = false;
    double GEANT4CutValue = 0.01;
    std::string outputPathLocal = "";
    std::string outputPathNAF = "";
    std::string macroFileLocal = "";
    std::string macroFileNAF = "";
    int numThreadsLocal = 1;
    int numThreadsNAF =1;
    std::string runMode = "";
    bool verbosePL = false;
    bool verbosePG = false;
    bool verboseSD = false;
    bool verboseSA = false;
    //TODO: When I print thses flags to a file add also the right material/ ion configs
};

void DumpConfigToFile(const std::string& filename);
void LoadSimFlagsFromFile(const std::string& filename, SimFlags& flags);
std::string CreateNextRunDirectory(bool flag, SimFlags* fFlags);
int GetRequestCpusFromSubmitFile(const std::string& submitFilePath);
void trowWarning (G4String origin, G4String exceptionCode, G4String description);
void trowError (G4String origin, G4String exceptionCode, G4String description);


#endif
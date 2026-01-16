#ifndef CONFIG_H
#define CONFIG_H

#include <string>

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
    double detectorDepth =0.;
    double CCModelSigmaX =0.;
    double CCModelSigmaY =0.;
    std::string outsideMaterial = "G4_Galactic";
    std::string beamGeometry = "";
    double gausSmearing = 0.;
    double beamXOffset = 0.;
    double beamYOffset = 0.;
    double beamZOffset = 0.;
    double sourceRadius =  0.;
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
    bool dutTungstenAbsorberFlag = false;
    double absorberThickness = 0.;
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
std::string CreateNextRunDirectory(bool flag, const SimFlags* fFlags);
int GetRequestCpusFromSubmitFile(const std::string& submitFilePath);
void throwWarning (const std::string& origin, 
                   const std::string& exceptionCode, 
                   const std::string& description);
void throwError (const std::string& origin,
                 const std::string& exceptionCode,
                 const std::string& description);


#endif
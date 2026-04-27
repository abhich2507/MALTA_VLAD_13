#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <vector>

struct SimFlags
{
    bool isBatch = false; // This is not read from the config file, but set in the main function
    std::string preDefinedGeometryFlag = "DEBUG";
    std::string geoFile = "";
    float detectorXOffset = 0.;
    float detectorYOffset = 0.;
    float detectorZOffset = 0.;
    float pixelSize = 0.;
    float detectorSizeX =0.;
    float detectorSizeY =0.;
    float detectorDepth =0.;
    float CCModelSigmaX =0.;
    float CCModelSigmaY =0.;
    std::string outsideMaterial = "G4_Galactic";
    std::string beamGeometry = "";
    float gausSmearing = 0.;
    float beamXOffset = 0.;
    float beamYOffset = 0.;
    float beamZOffset = 0.;
    float sourceRadius =  0.;
    float sourceRadiusX =  0.;
    float sourceRadiusY =  0.;
    int particleCount = 0;
    int numEvents = 0;
    float intraSpillOffset = 0;
    float beamVeto = 0;
    std::string particleType = "";
    std::string particleEnergy = "";
    std::string energyDistribution = "";
    float particleMomentumX = 0.;
    float particleMomentumY = 0.;
    float particleMomentumZ = 0.;
    bool itkEnable = false;
    double pileUpScale = 0.;
    std::string itkInput = "";
    int itkLayer = 0;
    double itkZ = 0.;
    bool EMPhysics = true;
    bool hadronPhysics = true;
    bool dutTungstenAbsorberFlag = false;
    float absorberThickness = 0.;
    bool setGEANT4Cuts = false;
    float GEANT4CutValue = 0.01;
    std::string outputPathLocal = "";
    std::string outputPathNAF = "";
    std::string macroFileLocal = "";
    std::string macroFileNAF = "";
    int numThreadsLocal = 1;
    int numThreadsNAF =1;
    std::string runMode = "";
    bool gdmlBool = false;
    std::string gdmlStr = "";
    bool verbosePL = false;
    bool verbosePG = false;
    bool verboseSD = false;
    bool verboseSA = false;
    //TODO: When I print thses flags to a file add also the right material/ ion configs
};

struct Module 
{
    int x, y, z;
    double xoff, yoff, zoff, xrot, yrot, zrot;
    int modID;
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

std::vector<Module> LoadModules(const std::string& filename);

#endif
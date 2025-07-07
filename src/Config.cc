#include "Config.hh"






void DumpConfigToFile(const std::string& filename) 
{
    // Copy the config file used as input (from SIMU_CONFIG) to 'filename'
    const char* configPath = std::getenv("SIMU_CONFIG");
    if (configPath && std::filesystem::exists(configPath)) {
        try {
            std::filesystem::copy_file(configPath, filename, std::filesystem::copy_options::overwrite_existing);
            std::cout << "Copied config file from " << configPath << " to " << filename << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Failed to copy config file: " << e.what() << std::endl;
        }
    } else {
        std::cerr << "SIMU_CONFIG not set or file does not exist, skipping config copy." << std::endl;
    }
}


std::string CreateNextRunDirectory(bool flag, SimFlags* fFlags) 
{   
    namespace fs = std::filesystem;
    std::string path =  "";
    if(fFlags->runMode == "local") 
    {
        path = fFlags->outputPathLocal;
    }
    else
    {
        path = fFlags->outputPathNAF;
    }
    int runNumber = 0;
    std::vector<std::string> dirs;
    int maxRun = 0;

    for (const auto& entry : fs::directory_iterator(path)) 
    {
        std::string name = entry.path().filename().string();
        //std::cout << name << '\n';
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
    std::ostringstream newRunDir;
    std::string saveFile;
    if(fFlags->runMode == "local")
    {
        saveFile = "/local_";
    }
    else
    {
        saveFile = "/naf_";
    }


    if (flag)
    {   
        // Do this only in the Master thread
        newRunDir << path << saveFile << std::setw(4) << std::setfill('0') << maxRun + 1;
        std::cout <<"Creating Directory: ";
        fs::create_directories(newRunDir.str());
    }
    else
    {
        newRunDir << path << saveFile << std::setw(4) << std::setfill('0') << maxRun;

    }

    //std::cout << newRunDir.str() << "\n";

    
    
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
        else if (key == "detectorXOffset") flags.detectorXOffset = std::stod(value);
        else if (key == "detectorYOffset") flags.detectorYOffset = std::stod(value);
        else if (key == "detectorZOffset") flags.detectorZOffset = std::stod(value);
        else if (key == "pixelSize") flags.pixelSize = std::stod(value);
        else if (key == "outsideMaterial") flags.outsideMaterial = value;
        else if (key == "beamGeometry") flags.beamGeometry = value;
        else if (key == "beamXOffset") flags.beamXOffset = std::stod(value);
        else if (key == "beamYOffset") flags.beamYOffset = std::stod(value);
        else if (key == "beamZOffset") flags.beamZOffset = std::stod(value);
        else if (key == "sourceRadius") flags.sourceRadius = std::stod(value);
        else if (key == "sourceXOffset") flags.sourceXOffset = std::stod(value);
        else if (key == "sourceYOffset") flags.sourceYOffset = std::stod(value);
        else if (key == "sourceZOffset") flags.sourceZOffset = std::stod(value);
        else if (key == "particleCount") flags.particleCount = std::stoi(value);
        else if (key == "numEvents") flags.numEvents = std::stoi(value);
        else if (key == "particleType") flags.particleType = value;
        else if (key == "particleEnergy") flags.particleEnergy = value;
        else if (key == "particleMomentumX") flags.particleMomentumX = std::stod(value);
        else if (key == "particleMomentumY") flags.particleMomentumY = std::stod(value);
        else if (key == "particleMomentumZ") flags.particleMomentumZ = std::stod(value);
        else if (key == "EMPhysics") flags.EMPhysics = (value == "true");
        else if (key == "hadronPhysics") flags.hadronPhysics = (value == "true");
        else if (key == "setGEANT4Cuts") flags.setGEANT4Cuts = (value == "true");
        else if (key == "GEANT4CutValue") flags.GEANT4CutValue = std::stod(value);
        else if (key == "outputPathLocal") flags.outputPathLocal = value;
        else if (key == "outputPathNAF") flags.outputPathNAF = value;
        else if (key == "macroFileLocal") flags.macroFileLocal = value;
        else if (key == "macroFileNAF") flags.macroFileNAF = value;
        else if (key == "numThreadsLocal") flags.numThreadsLocal = std::stoi(value);
        else if (key == "numThreadsNAF") flags.numThreadsNAF = std::stoi(value);
        else if (key == "runMode") flags.runMode == value;

        std::cout << "Loaded flag: " << key << " = " << value << std::endl;
    }
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
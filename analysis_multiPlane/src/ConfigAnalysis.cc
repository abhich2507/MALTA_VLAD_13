#include "ConfigAnalysis.hh"




void LoadAnalysisFlagsFromFile(const std::string& filename, AnaFlags& flags)
{
    std::cout << "Loading flags from file: " << filename << std::endl;
    std::ifstream infile(filename);
    if (!infile.is_open()) 
    {
        std::cerr << "Error: could not open file " << filename << std::endl;
        return;
    }


    std::string line;
    while (std::getline(infile, line)) {
        // Remove whitespace
        line.erase(std::remove_if(line.begin(), line.end(), ::isspace), line.end());
        if (line.empty() || line[0] == '#') continue; // skip comments/empty lines
        auto pos = line.find('=');
        if (pos == std::string::npos) continue;
        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1);

        if (key == "T") flags.T = std::stod(value);
        else if (key == "Tdiv") flags.Tdiv = std::stod(value);
        else if (key == "TrefThr") flags.TrefThr = std::stod(value);
        else if (key == "x0") flags.x0 = std::stod(value);
        else if (key == "n") flags.n = std::stod(value);
        else if (key == "t0") flags.t0 = std::stod(value);
        else if (key == "scintillatorJitter") flags.scintillatorJitter = std::stod(value);
        else if (key == "samplingJitter") flags.samplingJitter = std::stod(value);   
        else if (key == "groupSize") flags.groupSize = std::stoi(value);
        else if (key == "groupSizeX") flags.groupSizeX = std::stoi(value);
        else if (key == "groupSizeY") flags.groupSizeY = std::stoi(value);
        else if (key == "groupLeng") flags.groupLeng = std::stoi(value);
        else if (key == "parityLeng") flags.parityLeng = std::stoi(value);
        else if (key == "dColLeng") flags.dColLeng = std::stoi(value);
        else if (key == "numThreads") flags.numThreads = std::stoi(value);
        else if (key == "chLoss") flags.chLoss = std::stod(value);
        else if (key == "xPix") flags.xPix = std::stoi(value);
        else if (key == "yPix") flags.yPix = std::stoi(value);
        else if (key == "mirrorRepetition") flags.mirrorRepetition = std::stoi(value);
        else if (key == "meanSmearing") flags.meanSmearing = std::stod(value);
        else if (key == "colSmearing") flags.colSmearing = std::stod(value);
        else if (key == "wordSpacing") flags.wordSpacing = std::stod(value);
        else if (key == "distCut") flags.distCut = std::stod(value);
        else if (key == "timeCut") flags.timeCut = std::stod(value);
        else if (key == "trkUnc") flags.trkUnc   = (value == "true");
        else if (key == "clPos") flags.clPos     = value;
        else if (key == "verboseDigital") flags.verboseDigital = (value == "true");
        else if (key == "verboseTracking") flags.verboseTracking = (value == "true");
        else if (key == "verboseClustering") flags.verboseClustering = (value == "true");
        else if (key == "verboseAnalysis") flags.verboseAnalysis = (value == "true");
        else if (key == "trackOffsetX") flags.trackOffsetX = std::stod(value);
        else if (key == "trackOffsetY") flags.trackOffsetY = std::stod(value);
        else if (key == "veto") flags.veto = std::stod(value);
        else if (key == "fifoFrequency") flags.fifoFrequency = std::stod(value);
        else if (key == "boolHWC") flags.boolHWC   = (value == "true");
        else if (key == "sectorSize") flags.sectorSize = std::stoi(value);
        else if (key == "wordSize") flags.wordSize = std::stoi(value);
        else if (key == "fifoSize") flags.fifoSize = std::stod(value);
        else if (key == "fifoMultiplicity") flags.fifoMultiplicity = std::stoi(value);
        else if (key == "nPlanes_100") flags.nPlanes_100 = std::stoi(value);
        else if (key == "nPlanes_10") flags.nPlanes_10 = std::stoi(value);
        else if (key == "nPlanes_1") flags.nPlanes_1 = std::stoi(value);
        else if (key == "modules") flags.modules = std::stoi(value);
        else if (key == "geoFile") flags.geoFile = value;
        else if (key == "localPath") flags.localPath = value;
        else if (key == "inputPath") flags.inputPath = value;
        else if (key == "fileName") flags.fileName = value;
        else if (key == "MCTrueTree") flags.MCTrueTree = value;
        else if (key == "geometry") flags.geometry = value;
        else if (key == "slowcontrolDelay") flags.slowcontrolDelay = std::stod(value);
        else if (key == "busMergingThreshold") flags.busMergingThreshold = std::stod(value);
        else if (key == "SRAMFrequency") flags.SRAMFrequency = std::stod(value);
        else if (key == "sramDepth") flags.sramDepth = std::stoi(value);
        else if (key == "FIFOFrequency") flags.FIFOFrequency = std::stod(value);
        else if (key == "FIFOSize") flags.fifoSize = std::stoi(value);
        else if (key == "prioAlgo") flags.prioAlgo = value;
        else if (key == "simProc") flags.simProc = value;
        else if (key == "Analysis_XCenter") flags.Analysis_XCenter = std::stod(value);
        else if (key == "Analysis_YCenter") flags.Analysis_YCenter = std::stod(value);
        else if (key == "Analysis_XWidth") flags.Analysis_XWidth = std::stod(value);
        else if (key == "Analysis_YWidth") flags.Analysis_YWidth = std::stod(value);
        //std::cout << "Loaded flag: " << key << " = " << value << std::endl;
    }
}

void ConfigAnalysis()
{

}
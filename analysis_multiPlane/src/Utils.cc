#include "TStyle.h"
#include "TROOT.h"
#include <TROOT.h>
#include <TStyle.h>
#include <TF1.h>
#include <TH1.h>
#include <TH2.h>
#include <TCanvas.h>
#include <vector>
#include <utility>
#include <cstdint>
#include <iostream>
#include <ROOT/RNTuple.hxx>
#include <TH3D.h>
#include <TFile.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <chrono>
#include <random>
#include <TTree.h>
#include "Utils.hh"
#include "TChain.h"
#include "TRandom3.h"
#include<stack>
#include "TGraph.h"
#include <ROOT/RNTuple.hxx>
#include <TH3D.h>
#include <TFile.h>
#include "TFitResult.h"


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

ThresholdMap generateThrMap(double inputThreshold, int runNumber, std::string saveName, AnaFlags cfg, unsigned int seed)
{
    // This is an example data input for data sim threshold dispersion validation
    // Thr 967
    //std::vector<double> vThrMeanData = {959.2, 1004.8, 986.9, 1003.7, 1030.1, 1037.2, 1002.7, 983.7, 938.2, 952.7, 976.5, 943.4, 960.7, 930.8, 884.3, 875.2};
    // Thr 200
    std::vector<double> vThrMeanData = {228.594,220.652,223.642,236.398,230.506,242.855,235.05,228.462,233.891,226.979,218.171,214.329,223.578,207.099,209.558,210.827};

    // Initialize from config
    int pixXNum = cfg.xPix;
    int pixYNum = cfg.yPix;
    int groupRepetition = cfg.mirrorRepetition;
    double relativeThresholdSmearingCol = cfg.colSmearing;
    double relativeThresholdSmearingMean = cfg.meanSmearing;
    std::string directoryPath = cfg.localPath + "Plots/";
    std::string runPath = Form("local_%04d/", runNumber);

    // Save the threshold 
    TH1D *h1DThreshold =  new TH1D("h1DUTThreshold", "h1DThreshold", 100, inputThreshold - inputThreshold / 2,inputThreshold + inputThreshold / 2);
    TH2D *h2DThreshold    = new TH2D("h2DUTThreshold", "h2DUTThreshold", 512, 0, 512, 512, 0, 512);

    ThresholdMap result;

    std::mt19937 gen(seed);
    std::normal_distribution<> meanDist(1.0, relativeThresholdSmearingMean);
    std::normal_distribution<> colDist(1.0, relativeThresholdSmearingCol);

    //TODO: Improve Complexity
    for (int i = 0; i< pixXNum/groupRepetition; i++)
    {
        double thresholdMean = inputThreshold * meanDist(gen);
        for (int x = 0; x < pixXNum; x++)
        {
            for (int y = 0; y < pixYNum; y++)
            {
                if (x / groupRepetition == i)
                {
                    //static std::mt19937 gen(std::random_device{}());
                    double thresholdCol = thresholdMean * colDist(gen);
                    result[{x,y}] = thresholdCol;
                    h1DThreshold->Fill(thresholdCol);
                    h2DThreshold->Fill(x, y, thresholdCol);
                }
            }
        }
    }
    savePlot(directoryPath, runPath, inputThreshold, saveName, h1DThreshold, "h1DThreshold");
    savePlot(directoryPath, runPath, inputThreshold, saveName, h2DThreshold, "h2DThreshold");

    return result;
}


// Save diagnostic plots
void savePlot (std::string directoryPath, std::string runPath, double threshold, std::string saveName, TH1* hist, std::string histName)
{
    // TODO this doesnt work as I want it to. 
    set_style();
    TCanvas *c2D = new TCanvas(histName.c_str(), histName.c_str(), 800, 800);
    std::string filePath = directoryPath + runPath + saveName + "/histos.root";
    //std::cout << "Saving plot to: " << filePath << std::endl;

    mkdir(directoryPath.c_str(), 0777);
    mkdir((directoryPath + runPath).c_str(), 0777);
    mkdir((directoryPath + runPath + saveName).c_str(), 0777);
    
    TFile *histosFile = TFile::Open(filePath.c_str(), "UPDATE"); // create if missing
    if (!histosFile || histosFile->IsZombie()) {
        histosFile = TFile::Open(filePath.c_str(), "RECREATE");
    }

    std::string thrDirName = Form("Thr%i", int(threshold));
    TDirectory *thrDir = histosFile->GetDirectory(thrDirName.c_str());
    if (!thrDir) {
        thrDir = histosFile->mkdir(thrDirName.c_str());
    }
    thrDir->cd();

    hist->Write(histName.c_str(), TObject::kOverwrite);
    thrDir->Close();
    histosFile->cd();  
    histosFile->Close();
    delete histosFile;
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

// Returns timing offset for a given amplitude
double GetTimingOffset(double amplitude, double threshold, double T, double Tdiv, double TrefThr, double x0, double n, double t0) 
{
    if (amplitude < threshold) // if less than than threshold 
    {
        return Tdiv; // set to 200 ns delay if less than threshold (function diverges at threshold)
    }
    return T / pow((amplitude * TrefThr/threshold) - x0, n) + t0;
}

double GetFrontEndJitter(double charge)
{
   double out = 5.14 * std::exp(-charge/149.13) + 2264.86 * std::exp(-charge/15.05) + 0.2;
   if (out < 0) out = 0;
   return out;
}

// Returns positions of set bits in a 16-bit word
std::vector<int> getSetBitPositions(__uint128_t maltaPixel, int groupSize)
// Checks for all flipped bits in a 16-bit word and returns their positions in a vector
{
    std::vector<int> positions;
    for (int i = 0; i < groupSize; i++) {
        if (maltaPixel & (__uint128_t(1) << i)) {
            positions.push_back(i);
        }
    }
    return positions;
}

__uint128_t encodeWord(int pixX, int pixY, int groupSizeX, int groupSizeY , int groupLeng, int parityLeng, int dColLeng, bool verbose)
{
    __uint128_t maltaDColumn, maltaGroup, maltaDelay, maltaParity, maltaPixel;
    // Lesson learned UInt_t is 32 bit word. All the most meaningful bits above that get truncated. Use 64 bits ULong or even larger for future?
    
    maltaDColumn = pixX / groupSizeY;
    maltaGroup = pixY / (groupSizeX *2);
    maltaDelay = 1;
    maltaParity = (pixY /groupSizeX) %2; 
    maltaPixel = 0;
    maltaPixel ^= (__uint128_t(1) << ( (pixY % groupSizeX) + (groupSizeX * (pixX % groupSizeY)) ));
    //UInt_t PIXEL_MASK   = (1u << (groupSizeX *groupSizeY))   - 1;
    /*
    ULong64_t PIXEL_MASK   = (UInt_t)((1ULL << (groupSizeX * groupSizeY)) - 1ULL);
    ULong64_t GROUP_MASK   = (UInt_t)((1ULL << groupLeng) - 1ULL);
    ULong64_t PARITY_MASK  = (UInt_t)((1ULL << parityLeng) - 1ULL);
    ULong64_t DCOLUMN_MASK = (UInt_t)((1ULL << dColLeng) - 1ULL);
    ULong64_t word = 0;
    */
    __uint128_t PIXEL_MASK   = (__uint128_t(1) << (groupSizeX * groupSizeY)) - 1;
    __uint128_t GROUP_MASK   = (__uint128_t(1) << groupLeng) - 1;
    __uint128_t PARITY_MASK  = (__uint128_t(1) << parityLeng) - 1;
    __uint128_t DCOLUMN_MASK = (__uint128_t(1) << dColLeng) - 1;
    __uint128_t word = 0;

    word |= (maltaPixel & PIXEL_MASK)         << (dColLeng + parityLeng + groupLeng); // shift left by 14 bits
    word |= (maltaGroup & GROUP_MASK)         << (dColLeng + parityLeng);     // shift left by 9 bits
    word |= (maltaParity & PARITY_MASK)       << dColLeng;           // shift left by 8 bits
    word |= (maltaDColumn & DCOLUMN_MASK);                    // stays in lower 8 bits
    if (verbose)
    {
        std::cout << "DColumn: " << std::bitset<6>(maltaDColumn) << "; Group: " << std::bitset<5>(maltaGroup) << "; Parity: " << std::bitset<1>(maltaParity) << "; MaltaPixel: " << std::bitset<64>(maltaPixel) << std::endl;
        std::cout << "Encoded word: " << std::bitset<77>(word) << std::endl;
    }
    return word; 
}


// Customizable mask for decoding digital subwords in a MALTA word.
std::vector<__uint128_t> decodingMaskMSB(__uint128_t word, const std::vector<int>& field_sizes)
{
    // Simplified description of the code operation for nominal MALTA parameters:
    /*
    UInt_t maltaPixel   = (word >> (5 + 1 + 8)) & 0xFFFF; // top 16 bits
    UInt_t maltaGroup   = (word >> (1 + 8))   & 0x1F;     // next 5 bits
    UInt_t maltaParity  = (word >> 8)         & 0x1;      // 1 bit
    UInt_t maltaDColumn =  word               & 0xFF;     // last 8 bits
    */
    std::vector<__uint128_t> fields;
    int total_bits = 0;
    for (int s : field_sizes) total_bits += s;
    int shift = total_bits;

    for (int size : field_sizes)
    {
        shift -= size;
        __uint128_t mask = (__uint128_t(1) << size) - 1;
        __uint128_t value = (word >> shift) & mask;
        fields.push_back(value);
    }

    return fields;
}

// Decodes a digital word into pixel positions and hit counts
std::vector<DecodedHit> decodedDigitalWord(__uint128_t word, AnaFlags cfg)
{
    std::vector<int> field_sizes = {cfg.groupSize, cfg.groupLeng, cfg.parityLeng, cfg.dColLeng};
    auto decodedWords   = decodingMaskMSB(word, field_sizes);
    __uint128_t maltaPixel   = decodedWords[0];
    __uint128_t maltaGroup   = decodedWords[1];
    __uint128_t maltaParity  = decodedWords[2];
    __uint128_t maltaDColumn = decodedWords[3]; 

    std::vector<DecodedHit> pixelPositions;
    std::vector<int> hitInGroup = getSetBitPositions(maltaPixel, cfg.groupSize);

    int nHits = 0;
    for (int hit :hitInGroup)
    {
        nHits ++;
        int x_subgroup = hit / cfg.groupSizeX;      // 0 .. (groupSizeY-1)
        int y_in_subgroup = hit % cfg.groupSizeX;   // 0 .. (groupSizeX-1)

        int pixX = maltaDColumn * cfg.groupSizeY + x_subgroup;
        int pixY = maltaGroup * (cfg.groupSizeX * 2) + maltaParity * cfg.groupSizeX + y_in_subgroup;

        pixelPositions.push_back({pixX, pixY, nHits});
    }
    
    return pixelPositions;
}

/*
void digitalTest(std::vector<std::pair<int,int>> hits)
{
    // Takes as input a set of hit positions and outputs the merged hit positions
    __uint128_t mergedWord = 0;
    for(auto& hit: hits)
    {
        std::cout << "Input Hits: X: " << hit.first << "; Y: " << hit.second << std::endl; 
        __uint128_t word = encodeWord(hit.first, hit.second, 8, 2, 5, 1, 8, false);
        std::cout << "Encoded word: " << std::bitset<30>(word) << std::endl;
        mergedWord |= word;
    }
    std::cout << "Merged word:  " << std::bitset<30>(mergedWord) << std::endl;

    std::vector< std::pair<std::pair<int,int>, int> > pixPos = decodedDigitalWord(mergedWord, 16, 8, 2, 5, 1, 8);
    for (const auto& pos : pixPos) 
    {
        int reconstructedPixX = pos.first.first;
        int reconstructedPixY = pos.first.second;

        std::cout << "Reconstructed hit: X: " << reconstructedPixX << " ; Y: " << reconstructedPixY << std::endl;
    }
}
*/

std::pair<double,double> GetSpecificPlaneOffset(int plane, std::string geometry)
{
    int plane_1 = plane%10;
    int plane_10 = plane%100/10;
    int plane_100 = plane%1000/100;
    double XOffset_global = 0.;
    double YOffset_global = 0.;
    if (geometry=="LHCf"){
        if (plane_100%2==1){// only apply for every second z-coordinate
            XOffset_global = 0.2; // 0.2 mm
            YOffset_global = -0.2; // 0.2 mm
        }
    }
    return {XOffset_global, YOffset_global};
}

std::vector<uint32_t> CompressWords(std::vector<uint32_t> vals, int targetWidth)
{
    int bitWidth = 4;
    while (vals.size() > 1 && bitWidth < targetWidth)
    {
        std::vector<uint32_t> next;

        for (size_t i = 0; i < vals.size(); i += 2)
        {
            if (i + 1 < vals.size())
                next.push_back(vals[i] + vals[i + 1]);
            else
                next.push_back(vals[i]);
        }

        vals = std::move(next);
        bitWidth++;
    }

    return vals;
}

double dot(const Vec3& a, const Vec3& b)
{
    return a.x*b.x + a.y*b.y + a.z*b.z;
}

void BuildRotationMatrix(Offset& g)
{
    double cx = std::cos(g.xrot);
    double sx = std::sin(g.xrot);

    double cy = std::cos(g.yrot);
    double sy = std::sin(g.yrot);

    double cz = std::cos(g.zrot);
    double sz = std::sin(g.zrot);

    // R = Rz * Ry * Rx  (standard tracking convention)
    //TODO: Double check
    g.R[0][0] = cz*cy;
    g.R[0][1] = cz*sy*sx - sz*cx;
    g.R[0][2] = cz*sy*cx + sz*sx;

    g.R[1][0] = sz*cy;
    g.R[1][1] = sz*sy*sx + cz*cx;
    g.R[1][2] = sz*sy*cx - cz*sx;

    g.R[2][0] = -sy;
    g.R[2][1] = cy*sx;
    g.R[2][2] = cy*cx;
}

Vec3 ApplyGeometry3D(const Vec3& p, Offset& g)
{
    BuildRotationMatrix(g);
    Vec3 out;
    //std::cout << "px: " << p.x << "; py: " << p.y;
    // rotate
    out.x = g.R[0][0]*p.x + g.R[0][1]*p.y + g.R[0][2]*p.z;
    out.y = g.R[1][0]*p.x + g.R[1][1]*p.y + g.R[1][2]*p.z;
    out.z = g.R[2][0]*p.x + g.R[2][1]*p.y + g.R[2][2]*p.z;
    //std::cout << " rx: " << out.x << "; ry: " << out.y;
    // translate
    out.x += g.x *10;
    out.y += g.y *10;
    out.z += g.z *10;
    //std::cout << " tx: " << out.x << "; ty: " << out.y << std::endl;

    

    return out;
}

Vec3 ApplyInverseGeometry3D(const Vec3& global, const Offset& g)
{
    // remove translation
    Vec3 shifted = {
        global.x - g.x * 10,
        global.y - g.y * 10,
        global.z - g.z * 10
    };

    // apply inverse rotation (transpose of R)
    Vec3 local;
    local.x = g.R[0][0]*shifted.x + g.R[1][0]*shifted.y + g.R[2][0]*shifted.z;
    local.y = g.R[0][1]*shifted.x + g.R[1][1]*shifted.y + g.R[2][1]*shifted.z;
    local.z = g.R[0][2]*shifted.x + g.R[1][2]*shifted.y + g.R[2][2]*shifted.z;

    return local;
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

// Reconstruct position of planes from the config file
// return global coordinates in mm of the pixel center in which hit occured.
Vec3 PixelPositionReconstruction(int pixelX, int pixelY, const DetectorConfig& cfg)
{
    // IF this breaks it could be due to the lack of offset here that was moved to rotate3d
    // detectorXOffset, detectorYOffset is that of the center of plane0
    Vec3 local;
    local.x = pixelX * cfg.pixelSize  - cfg.detectorSizeX / 2 + cfg.pixelSize /2;
    local.y= pixelY * cfg.pixelSize - cfg.detectorSizeY / 2 + cfg.pixelSize /2;
    local.z = 0.;

    return local;
}

Vec3 IntersectTrackPlane(const Vec3& V,  const DetectorConfig& cfg, Offset& g)
{
    // plane point
    BuildRotationMatrix(g);
    Vec3 P0 = { g.x, g.y, g.z };
    Vec3 D =  {cfg.momX, cfg.momY, cfg.momZ};


    // plane normal (local Z axis rotated)
    Vec3 n = {g.R[0][2], g.R[1][2], g.R[2][2]};
    double denom = dot(D, n);


    //std::cout << "gx: " << g.x << "; gy: " << g.y << "; gz: " << g.z << "; Dx: " << cfg.momX << "; Dy: " << cfg.momY << "; Dz: " << cfg.momZ
    //          <<"; nx: " << n.x << "; ny: " << n.y << "; nz: " << n.z << "; denom: " << denom<< std::endl;

    // avoid division by zero (parallel case)
    if (std::abs(denom) < 1e-9) 
    {
        return V;
    }

    Vec3 P0_minus_V = {P0.x - V.x, P0.y - V.y, P0.z - V.z};

    double t = dot(P0_minus_V, n) / denom;

    Vec3 X;
    X.x = V.x + t * D.x;
    X.y = V.y + t * D.y;
    X.z = V.z + t * D.z;

    //std::cout << "Vx: " << V.x << "; Vy: " << V.y << "; Vz: " << V.z << std::endl;
    //std::cout << "X: " << X.x << "; Y: " << X.y << "; Z: " << X.z << std::endl;


    return X;
}

std::vector<RawHit> GetRawHits(TTree* plane)
{
    int eventID, planeID, pixX, pixY;
    float time, energy;

    plane->SetBranchAddress("iEvent", &eventID);
    plane->SetBranchAddress("iPlane", &planeID);
    plane->SetBranchAddress("PixX", &pixX);
    plane->SetBranchAddress("PixY", &pixY);
    plane->SetBranchAddress("hitTime", &time);
    plane->SetBranchAddress("hitEnergy", &energy);
    Long64_t nRawEntries = plane->GetEntries();

    std::vector<RawHit> output;
    output.reserve(nRawEntries);

    for (Long64_t j = 0; j < nRawEntries; j++)
    {
        plane->GetEntry(j);
        output.push_back(RawHit{HitKey{planeID, eventID, pixX, pixY}, static_cast<double>(energy), static_cast<double>(time)});
    }
    return output;
}

std::pair<EnergyMap, TimeMap> BuildEnergyTimeMap(std::vector<RawHit> rawHits)
{
    EnergyMap enMap;
    TimeMap timeMap;
    for (const auto& hit : rawHits)
    {
        enMap[HitKey{hit.key.plane, hit.key.event, hit.key.x, hit.key.y}] += hit.energy;
        timeMap[HitKey{hit.key.plane, hit.key.event, hit.key.x, hit.key.y}].push_back(hit.time);

    }
    return {enMap, timeMap};

}

SortedTimeVector CorrectAndSortTimeMap(EnergyMap enMap, TimeMap timeMap, ThresholdMap thresholdMap , AnaFlags cfg, unsigned int seed)
{
    std::mt19937 gen(seed);
    SortedTimeVector sortedTimings;
    for (const auto& entry : enMap) 
    {
        const HitKey& key = entry.first;
        auto itThr = thresholdMap.find({key.x, key.y});
        if (itThr == thresholdMap.end()) continue;

        auto it = timeMap.find(key);
        if(it == timeMap.end()) continue;
        // Row correction also of 7ns/ 512 rows + global GEANT4 timestamp + in-module chip id + front end jitter
        std::normal_distribution<double> gauss(0.0, GetFrontEndJitter(entry.second));    
        double timing    = GetTimingOffset(entry.second, itThr->second, cfg.T, cfg.Tdiv, cfg.TrefThr, cfg.x0, cfg.n, cfg.t0);     
        timing += *std::max_element(it->second.begin(), it->second.end()) + key.y * 0.0125 + (4 - (key.plane %100)%4) * 8 + std::abs(gauss(gen));
        sortedTimings.emplace_back(entry.first,timing);
    }
    std::sort(sortedTimings.begin(), sortedTimings.end(), [](auto &a, auto &b){ return a.second < b.second; });

    return sortedTimings;
}

std::vector<WordBucket> AssignMALTA2WordBuckets(EnergyMap enMap, SortedTimeVector sortedTimings , ThresholdMap thresholdMap, AnaFlags cfg)
{
    WordBucket merger;
    std::vector<WordBucket> digitizedWords;
    double t0 = sortedTimings.begin()->second;
    for (const auto& entry : sortedTimings) 
    {
        const HitKey& key = entry.first;
        auto itThr = thresholdMap.find({key.x, key.y});
        if (itThr == thresholdMap.end()) continue;

        double threshold = itThr->second;
        double timing = entry.second;
        auto it = enMap.find(key);
        double cenergy = it->second;

        if (cenergy < threshold) continue;

        __uint128_t word = encodeWord(key.x, key.y, cfg.groupSizeX, cfg.groupSizeY, cfg.groupLeng, cfg.parityLeng, cfg.dColLeng, false);

        //Now I digitized all my words. Next step is merging them based on timing
        if (timing >= t0 && timing < t0 + cfg.wordSpacing)
        {
            merger.push_back({word, timing, key.plane});
        }
        else
        {
            t0 = timing;
            digitizedWords.push_back(merger);
            merger.clear();
            merger.push_back({word, timing, key.plane});
        }
    }
    // Also push the very last merged word.
    if (!merger.empty()) digitizedWords.push_back(merger);

    return digitizedWords;
}

CoincidentWords MergeMALTA2Words(std::vector<WordBucket> digitizedWords)
{
    CoincidentWords mergedWords;
    
    for (int i =0; i< digitizedWords.size(); i++)
    {
        // This was not initialized before leading to weird first word
        __uint128_t mergedWord = 0; 
        std::vector<double> leadingTime{};
        std::vector<int> planeVec{};
        int aux = 0;
        for (const auto& pos : digitizedWords[i])
        {
            int word = pos.word;
            double timing = pos.time;
            int planeN = pos.planeID;
            planeVec.push_back(planeN);
            mergedWord |= word;
            leadingTime.push_back(timing);
        }
        // Save the merged word and the fastest hit time
        if (digitizedWords[i].size() == 0) continue; // Skip empty vectors
        mergedWords.push_back({mergedWord, *std::max_element(leadingTime.begin(), leadingTime.end()), planeVec});
        // Reset fot next iteration
        mergedWord = 0;
        leadingTime.clear();
        planeVec.clear();
    }
    return mergedWords;

}

std::vector<ProcessedHit> ProcessDecodedHits(CoincidentWords mergedWords, AnaFlags cfg, unsigned int seed)
{
    std::mt19937 gen(seed);
    std::vector<ProcessedHit> output;
    for (auto& entry: mergedWords) 
    {
        std::vector<int> planeN = entry.planeID;
        __uint128_t word = entry.word;
        std::vector<DecodedHit> pixelPositions = decodedDigitalWord(word, cfg);
        int i = 0;
        for (const auto& pos : pixelPositions) 
        {
            ///// Add off-chip READOUT jitter
            // Add in quadrature the un-correlated jitter
            double totalJitter = std::sqrt(cfg.scintillatorJitter*cfg.scintillatorJitter + cfg.samplingJitter*cfg.samplingJitter);
            std::normal_distribution<double> gauss(0.0, totalJitter);
            output.push_back({entry.planeID[i], pos.x, pos.y, entry.time + gauss(gen), pos.nHit});
            i++;
        }
    }
    return output;
}

void FillReconstructedTree(std::vector<ProcessedHit> allProcessHits, double inputThreshold, int runNumber, std::string saveName, AnaFlags cfg)
{
    std::string inputPath = cfg.inputPath+Form("_%04d/", runNumber);


    mkdir((inputPath + saveName).c_str(), 0777);   
    TFile *outfile = new TFile((inputPath + saveName + "/ReconstructedHitsThr" + std::to_string(int(inputThreshold)) + ".root").c_str(), "RECREATE");
    outfile->cd();
    // Create a TTree
    TTree *reconstructedTree = new TTree("ReconstructedHits", "Reconstructed Hits");
    reconstructedTree->SetDirectory(nullptr);
    // Variables for branches
    int reconstructedPixX, reconstructedPixY, nHits, planeID;
    double reconstructedTiming;
    // Create branches
    reconstructedTree->Branch("planeID", &planeID, "planeID/I");
    reconstructedTree->Branch("PixX", &reconstructedPixX, "PixX/I");
    reconstructedTree->Branch("PixY", &reconstructedPixY, "PixY/I");
    reconstructedTree->Branch("timing", &reconstructedTiming, "timing/D");
    reconstructedTree->Branch("NHits", &nHits, "NHits/I");

    for (const auto& el: allProcessHits)
    {
        planeID = el.planeID;
        reconstructedPixX = el.x;
        reconstructedPixY = el.y;
        reconstructedTiming = el.time;
        nHits = el.nHit;
        reconstructedTree->Fill();
    }
    outfile->cd();
    reconstructedTree->Write("", TObject::kOverwrite);
    outfile->Close();    
}


std::vector<TrackEntry> GetVertex(AnaFlags cfg, int runNumber)
{
    std::vector<TrackEntry> tracks;
    std::string inputPath = cfg.inputPath+Form("_%04d/", runNumber);
    TChain *trackChain = new TChain("TruthVertex");
    std::string fileName = cfg.fileName;
    for (int t = 0; t <= cfg.numThreads - 1; ++t) 
    {
        trackChain->Add(Form("%s%s_t%d.root", inputPath.c_str(), fileName.c_str(), t));
    }

    float vertexX_float, vertexY_float, vertexZ_float, globalTime_float;
    // Connect branches
    trackChain->SetBranchAddress("trueVertexX", &vertexX_float);
    trackChain->SetBranchAddress("trueVertexY", &vertexY_float);
    trackChain->SetBranchAddress("trueVertexZ", &vertexZ_float);
    trackChain->SetBranchAddress("trueGlobalTime", &globalTime_float);
    Long64_t nTrackEntries = trackChain->GetEntries();
    // For multiThreading I need to first time order the hits.
    for (Long64_t i = 0; i < nTrackEntries; i++) 
    {
        trackChain->GetEntry(i);

        double vertexX = static_cast<double>(vertexX_float);
        double vertexY = static_cast<double>(vertexY_float);
        double vertexZ = static_cast<double>(vertexZ_float);
        double globalTime = static_cast<double>(globalTime_float);
        tracks.push_back({vertexX, vertexY, vertexZ, globalTime});
    }
    // sort by time
    std::sort(tracks.begin(), tracks.end(), [](const TrackEntry &a, const TrackEntry &b){return a.t < b.t;});

    return tracks;
}

std::vector<ProcessedHit> GetTrackHits(AnaFlags cfg, double threshold, int runNumber, std::string saveName, int planeZ)
{
    std::string inputSubPath = cfg.inputPath+Form("_%04d/", runNumber) + saveName + "/";
    std::vector<ProcessedHit> hits;

    // read in the MALTA hits from tree:
    TFile *reconstructedFile = TFile::Open((inputSubPath + "ReconstructedHitsThr" + std::to_string(int(threshold)) + ".root").c_str(), "READ");
    TTree *reconstructedTree = (TTree*) reconstructedFile->Get("ReconstructedHits");
    int reconstructedPixX, reconstructedPixY, planeID;
    double reconstructedTiming;
    reconstructedTree->SetBranchAddress("planeID", &planeID);
    reconstructedTree->SetBranchAddress("PixX", &reconstructedPixX);
    reconstructedTree->SetBranchAddress("PixY", &reconstructedPixY);
    reconstructedTree->SetBranchAddress("timing", &reconstructedTiming);
    Long64_t nReconstructedEntries = reconstructedTree->GetEntries();

    hits.reserve(nReconstructedEntries);  // optional (performance only)

    for (int i = 0; i < nReconstructedEntries; i++)
    {
        reconstructedTree->GetEntry(i);
        if (planeID%1000000/10000!= planeZ) continue;
        hits.push_back({planeID, reconstructedPixX, reconstructedPixY, reconstructedTiming});
    }
    // sort by time. Different x-y planes have mixed up the timing.
    std::sort(hits.begin(), hits.end(), [](const ProcessedHit &a, const ProcessedHit &b) {return a.time < b.time;});

    return hits;
}

std::pair<std::vector<FullTrackInfo>, std::vector<Residual>> MatchHits(std::vector<TrackEntry> tracks, std::vector<ProcessedHit> hits, AnaFlags cfg, int runNumber)
{
    std::vector<FullTrackInfo> TrackOut;
    std::vector<Residual> ResOut;
    int DUTPixX = 0;
    int DUTPixY = 0;
    double DUTLocalTime = 0;
    int filePlane = 0;
    std::string inputPath = cfg.inputPath+Form("_%04d/", runNumber);
    // BIG TODO:
    DetectorConfig detCfg = LoadConfig(inputPath + "flags.cfg"); // todo: does this need to be generalized? YES
    auto geoMaps = LoadGeometry(cfg.geoFile, detCfg);
    // To avoid O(NxN) I will use a sliding window
    Long64_t detIdx = 0; // pointer in detector tree
    Long64_t nHits = hits.size();
    bool foundHit;
    for (int i =0; i< tracks.size(); i++)
    {
        auto track = tracks[i];
        Vec3 vertex = {track.x, track.y, track.z};
        // First we set up the sliding window
        while (detIdx < nHits && hits[detIdx].time < track.t) // take first MALTA hit that is at least the vertexTime
        {
            detIdx++;
        }
        
        // now check all hits in [t, t+Δt]
        Long64_t j = detIdx;
        foundHit = false;
        int DUTnHits = 0;
        while (j < nHits && hits[j].time < track.t + cfg.timeCut) // check whether inside window
        {

            DUTPixX = hits[j].x;
            DUTPixY = hits[j].y;
            filePlane = hits[j].planeID; // possible source of errors 
            DUTLocalTime = hits[j].time - track.t;
            Vec3 pixelPosition = PixelPositionReconstruction(DUTPixX, DUTPixY, detCfg);
            Vec3 trackPlaneIntercept = IntersectTrackPlane(vertex, detCfg, geoMaps[filePlane]);
            auto rotTransPixelPositions = ApplyGeometry3D(pixelPosition, geoMaps[filePlane]);
            double rx = rotTransPixelPositions.x - trackPlaneIntercept.x;
            double ry = rotTransPixelPositions.y - trackPlaneIntercept.y;
            
            ResOut.push_back({rx, ry});

            if(rx*rx + ry*ry <= (cfg.distCut/1000)*(cfg.distCut/1000))
            {
                DUTnHits++;
                TrackOut.push_back({filePlane, i, vertex.x, vertex.y, track.t, DUTPixX, DUTPixY, DUTnHits, DUTLocalTime});
                foundHit = true;
            }
            j++;
        }    
        if (!foundHit) 
        {
            // no hit matched: fill with sentinel values
            DUTPixX = -1;
            DUTPixY = -1;
            DUTLocalTime = -1;
            TrackOut.push_back({filePlane, i, vertex.x, vertex.y, track.t, DUTPixX, DUTPixY, DUTnHits, DUTLocalTime});
        }
    }
    return {TrackOut, ResOut};
}



TFile* CreateTrackedTree(double inputThreshold, int runNumber, std::string saveName, AnaFlags cfg)
{
    std::string inputSubPath = cfg.inputPath+Form("_%04d/", runNumber) + saveName + "/";
    TFile *outfile = new TFile((inputSubPath + "LocalTrackedHitsThr" + std::to_string(int(inputThreshold)) + ".root").c_str(), "RECREATE");

    return outfile;

}


void FillTrackedTree(std::vector<FullTrackInfo> trackedHits, TFile* outfile, int planeZ)
{
    outfile->cd();

    // Create a TTree for each planeZ
    TTree *trackedTree = new TTree(Form("TrackedHits_planeZ%d",planeZ), Form("Tracked Hits in PlaneZ %d",planeZ));
    // Variables for branches
    double vertexX, vertexY, vertexZ, vertexTime, DUTLocalTime;
    int DUTPixX, DUTPixY, trackID, DUTnHits, planeData;

    // Create branches
    trackedTree->Branch("planeID", &planeData, "planeID/I");
    trackedTree->Branch("trackID", &trackID, "trackID/I");
    trackedTree->Branch("vertexX", &vertexX, "vertexX/D");
    trackedTree->Branch("vertexY", &vertexY, "vertexY/D");
    trackedTree->Branch("vertexTime", &vertexTime, "vertexTime/D");
    trackedTree->Branch("DUTPixX", &DUTPixX, "DUTPixX/I");
    trackedTree->Branch("DUTPixY", &DUTPixY, "DUTPixY/I");
    trackedTree->Branch("DUTnHits", &DUTnHits, "DUTnHits/I");
    trackedTree->Branch("DUTLocalTime", &DUTLocalTime, "DUTLocalTime/D");

    for (const auto& el : trackedHits)
    {
        planeData = el.planeID;
        trackID = el.trackID;
        vertexX = el.vertexX;
        vertexY = el.vertexY;
        vertexTime = el.vertexTime;
        DUTPixX = el.dutX;
        DUTPixY = el.dutY;
        DUTnHits = el.dutNHits;
        DUTLocalTime = el.dutTime;
        trackedTree->Fill();
    }
    outfile->cd();  
    trackedTree->Write();
}

void CloseFile(TFile* outfile)
{
    outfile->Close();
}

void SaveResidualHisto(std::vector<Residual> residuals, int planeZ, double threshold, int runNumber, std::string saveName, AnaFlags cfg)
{
    std::string directoryPath = cfg.localPath + "Plots/";
    std::string runPath = Form("local_%04d/", runNumber);
    TH1D *h1ResidualX = new TH1D(Form("h1ResidualX_planeZ%d",planeZ), Form("h1ResidualX_planeZ%d",planeZ), 100, -2, 2);
    TH1D *h1ResidualY = new TH1D(Form("h1ResidualY_planeZ%d",planeZ), Form("h1ResidualY_planeZ%d",planeZ), 100, -2, 2);

    for (const auto& el : residuals)
    {
        h1ResidualX->Fill(el.rx);
        h1ResidualY->Fill(el.ry);
    }


    savePlot(directoryPath, runPath, threshold, saveName, h1ResidualX, Form("h1ResidualX_planeZ%d",planeZ));
    savePlot(directoryPath, runPath, threshold, saveName, h1ResidualY, Form("h1ResidualY_planeZ%d",planeZ));
}


bool hasHitAt(const std::vector<Hit>& cluster, int x, int y)
{
    return std::any_of(cluster.begin(), cluster.end(), [&](const Hit& h)
        {
            return h.x == x && h.y == y;
        });
}

std::vector<FullTrackInfo> GetMatchedHits(AnaFlags cfg, double threshold, int runNumber, std::string saveName, int planeZ)
{
    std::string inputPath = cfg.inputPath+Form("_%04d/", runNumber) + saveName + "/";

    TFile *trackedFile = TFile::Open((inputPath + "LocalTrackedHitsThr" + std::to_string(int(threshold)) + ".root").c_str(), "READ");
    TTree *trackedTree = (TTree*) trackedFile->Get(Form("TrackedHits_planeZ%d",planeZ));
    std::vector<FullTrackInfo> output;
    double vertexX, vertexY, globalTrigger;
    double reconstructedTime;
    int trackID, pixX, pixY, nHits, planeID;
    trackedTree->SetBranchAddress("planeID", &planeID);
    trackedTree->SetBranchAddress("trackID", &trackID);
    trackedTree->SetBranchAddress("vertexX", &vertexX);
    trackedTree->SetBranchAddress("vertexY", &vertexY);
    trackedTree->SetBranchAddress("vertexTime", &globalTrigger);
    trackedTree->SetBranchAddress("trackID", &trackID);
    trackedTree->SetBranchAddress("DUTPixX", &pixX);  
    trackedTree->SetBranchAddress("DUTPixY", &pixY);  
    trackedTree->SetBranchAddress("DUTnHits", &nHits);
    trackedTree->SetBranchAddress("DUTLocalTime", &reconstructedTime);
    Long64_t nTrackedEntries = trackedTree->GetEntries();

    for (int i = 0; i <= nTrackedEntries; i++)
    {
        trackedTree->GetEntry(i);
        output.push_back({planeID, trackID, vertexX, vertexY, globalTrigger, pixX, pixY, nHits, reconstructedTime});
    }

    return output;
}

Cluster ValidateCluster(std::vector<Hit>& cluster, const DetectorConfig& cfg)
{
    Cluster result{};
    const std::vector<std::pair<int,int>> diagonals = { {1,1}, {1,-1}, {-1,1}, {-1,-1} };

    for (auto it = cluster.begin(); it != cluster.end(); )
    {
        const int xPos = it->x;
        const int yPos = it->y;
        bool erase = (xPos == -1 || yPos == -1);

        for (const auto& [dx, dy] : diagonals)
        {
            if (!erase &&
                hasHitAt(cluster, xPos + dx, yPos + dy) &&
                !(hasHitAt(cluster, xPos + dx, yPos) ||
                  hasHitAt(cluster, xPos, yPos + dy)))
            {
                erase = true;
            }
        }

        if (erase)
        {
            it = cluster.erase(it);
        }
        else
        {
            ++result.clSize;
            Vec3 pix = PixelPositionReconstruction(xPos, yPos, cfg);
            result.x += pix.x;
            result.y += pix.y;
            ++it;
        }
    }
    return result;
}

std::pair<double,double> GetClusterPosition(const Cluster& cl, ClusterState& state, const std::string&   clPosMode)
{
    if (clPosMode == "COM" && cl.x > 0 && cl.y > 0)
        return { cl.x / cl.clSize, cl.y / cl.clSize};

    return { state.currentX, state.currentY }; // fallback: MC truth
}

ClusteredHit GetValidCluster(const Cluster& cl, ClusterState& state, const std::pair<double,double> vertex)
{
    double vertexX = vertex.first;
    double vertexY = vertex.second;
    int clSize  = cl.clSize;
    int clPlaneID = state.currentPlaneID;

    double timing = std::min_element(state.cluster.begin(), state.cluster.end(),
                 [](const Hit& a, const Hit& b){ return a.t < b.t; })->t;

    double correctedTiming = timing - state.currentPixY * 0.0125;

    return {clPlaneID, vertexX, vertexY, clSize, timing, correctedTiming};

}

void ResetClusterState(ClusterState& state, FullTrackInfo track)
{
    state.cluster.clear();
    state.cluster.push_back({track.dutX, track.dutY, track.dutTime});
    state.currentPlaneID  = track.planeID;
    state.currentTrackID  = track.trackID;
    state.currentPixY     = track.dutY;
    state.currentX        = track.vertexX;
    state.currentY        = track.vertexY;
}

TFile* CreateClusteredTree(AnaFlags cfg, double threshold, int runNumber, std::string saveName)
{
    std::string inputPath = cfg.inputPath+Form("_%04d/", runNumber) + saveName + "/";
    TFile *outfile = new TFile((inputPath + "analysisThr" + std::to_string(int(threshold)) + ".root").c_str(), "RECREATE");

    return outfile;
}

void FillTrackedTree(std::vector<ClusteredHit> allClusters, TFile* outfile, int planeZ)
{
    outfile->cd();
    TTree *analysisTree = new TTree(Form("analyzedHits_planeZ%d",planeZ), Form("analyzedHits_planeZ%d",planeZ));
    // Variables for branches
    double analysisVertexX, analysisVertexY, timing, correctedTiming;
    int analysisClSize, clPlaneID;
    // Create branches
    analysisTree->Branch("planeID", &clPlaneID, "planeID/I");
    analysisTree->Branch("analysisVertexX", &analysisVertexX, "analysisVertexX/D");
    analysisTree->Branch("analysisVertexY", &analysisVertexY, "analysisVertexY/D");
    analysisTree->Branch("clSize", &analysisClSize, "clSize/I");
    analysisTree->Branch("timing", &timing, "timing/D");
    analysisTree->Branch("correctedTiming", &correctedTiming, "correctedTiming/D");

    for (const auto& el : allClusters)
    {
        clPlaneID = el.planeID;
        analysisVertexX = el.x;
        analysisVertexY = el.y;
        analysisClSize = el.clSize;
        timing = el.timing;
        correctedTiming = el.corrTiming;
        analysisTree->Fill();
    }
    outfile->cd();  
    analysisTree->Write();
}

std::vector<std::vector<FullTrackInfo>> GroupHitsByTrack(const std::vector<FullTrackInfo>& hits)
{
    std::vector<std::vector<FullTrackInfo>> tracks;
    for (const auto& hit : hits)
    {
        if (tracks.empty() || hit.trackID != tracks.back().front().trackID)
            tracks.push_back({hit});
        else
            tracks.back().push_back(hit);
    }
    return tracks;
}

ClusterState BuildClusterState(const std::vector<FullTrackInfo>& track)
{
    ClusterState state;
    state.currentPlaneID = track.front().planeID;
    state.currentTrackID = track.front().trackID;
    state.currentPixY    = track.front().dutY;
    state.currentX       = track.front().vertexX;
    state.currentY       = track.front().vertexY;
    for (const auto& hit : track)
        state.cluster.push_back({hit.dutX, hit.dutY, hit.dutTime});
    return state;
}

// Error in percent, assuming binomial distribution
double getEffErr(int Npassed, int Nall) 
{
    if (Nall == 0) return 0.0;  // avoid division by zero
    double ratio = (double)Npassed / (double)Nall;
    return 100.0 * sqrt(ratio * (1.0 - ratio) / (double)Nall);
}

double getEff(int Npassed, int Nall) 
{
    if (Nall == 0) return 0.0;  // avoid division by zero
    return ((double)Npassed / (double)Nall) * 100.0;
}

std::vector<AnalysisHits> GetAnalysisHits(AnaFlags cfg, double threshold, int runNumber, std::string saveName, int planeZ)
{
    std::vector<AnalysisHits> output;
    std::string inputPath = cfg.inputPath+Form("_%04d/", runNumber)+ saveName + "/";
    TFile *analysisFile = TFile::Open((inputPath + "analysisThr" + std::to_string(int(threshold)) + ".root").c_str(), "READ");

    TTree *analysisTree = (TTree*) analysisFile->Get(Form("analyzedHits_planeZ%d",planeZ));

    double fX, fY, timing, correctedTiming;
    int clSize, planeID;
    analysisTree->SetBranchAddress("planeID", &planeID);
    analysisTree->SetBranchAddress("analysisVertexX", &fX);
    analysisTree->SetBranchAddress("analysisVertexY", &fY);
    analysisTree->SetBranchAddress("clSize", &clSize);  
    analysisTree->SetBranchAddress("timing", &timing); 
    analysisTree->SetBranchAddress("correctedTiming", &correctedTiming);
    Long64_t nAnalyzedEntries = analysisTree->GetEntries();
    for (int i =0; i< nAnalyzedEntries; i++)
    {
        analysisTree->GetEntry(i);
        output.push_back({planeID, fX, fY, clSize, timing, correctedTiming});
    }

    return output;
} 

std::vector<TH2D*> Create2DHistograms(AnaFlags cfg, int planeZ)
{
    std::vector<TH2D*> histograms;
    // TODO: These I think are not actual settable
    double Xcent = cfg.Analysis_XCenter;
    double Ycent = cfg.Analysis_YCenter;
    double Xwidth = cfg.Analysis_XWidth;
    double Ywidth = cfg.Analysis_YWidth;
    double lowX  = Xcent - Xwidth/2.;
    double highX = Xcent + Xwidth/2.; 
    double lowY  = Ycent - Ywidth/2.; 
    double highY = Ycent + Ywidth/2.; 
    //TODO: Generalize
    int numPixelsX = 2, numPixelsY = 2;
    int nX = numPixelsX*16, nY = numPixelsY*16, nZ = 100; // number of bins for in-pixel maps
    double pixelSizeX = 0.0364 , pixelSizeY = 0.0364; // in mm

    histograms.push_back(new TH2D(Form("h2ALL_planeZ%d",    planeZ), Form("h2ALL_planeZ%d",    planeZ), 100, lowX, highX, 100, lowY, highY));
    histograms.push_back(new TH2D(Form("h2PASS_planeZ%d",   planeZ), Form("h2PASS_planeZ%d",   planeZ), 100, lowX, highX, 100, lowY, highY));
    histograms.push_back(new TH2D(Form("h2ClSize_planeZ%d", planeZ), Form("h2ClSize_planeZ%d", planeZ), 100, lowX, highX, 100, lowY, highY));
    histograms.push_back(new TH2D(Form("h2Timing_planeZ%d", planeZ), Form("h2Timing_planeZ%d", planeZ), 100, lowX, highX, 100, lowY, highY));

    double inPixelX = numPixelsX * pixelSizeX * 1000;
    double inPixelY = numPixelsY * pixelSizeY * 1000;
    histograms.push_back(new TH2D(Form("h2ALLInPixel_planeZ%d",        planeZ), Form("h2ALLInPixel_planeZ%d",        planeZ), nX, 0, inPixelX, nY, 0, inPixelY));
    histograms.push_back(new TH2D(Form("h2PASSInPixel_planeZ%d",       planeZ), Form("h2PASSInPixel_planeZ%d",       planeZ), nX, 0, inPixelX, nY, 0, inPixelY));
    histograms.push_back(new TH2D(Form("h2ClSizeInPixel_planeZ%d",     planeZ), Form("h2ClSizeInPixel_planeZ%d",     planeZ), nX, 0, inPixelX, nY, 0, inPixelY));
    histograms.push_back(new TH2D(Form("h2TimingInPixel_planeZ%d",     planeZ), Form("h2TimingInPixel_planeZ%d",     planeZ), nX, 0, inPixelX, nY, 0, inPixelY));
    histograms.push_back(new TH2D(Form("h2MissMergedInPixel_planeZ%d", planeZ), Form("h2MissMergedInPixel_planeZ%d", planeZ), nX, 0, 0,        nY, numPixelsX, numPixelsY));
    
    return histograms;
}

std::vector<TH1D*> Create1DHistograms(int planeZ)
{
    std::vector<TH1D*> histograms;
    // Projections
    int numPixelsX = 2, numPixelsY = 2;
    int nX = numPixelsX*16, nY = numPixelsY*16, nZ = 100; // number of bins for in-pixel maps
    double pixelSizeX = 0.0364 , pixelSizeY = 0.0364; // in mm
    double inPixelX = numPixelsX * pixelSizeX * 1000;
    double inPixelY = numPixelsY * pixelSizeY * 1000;
    histograms.push_back(new TH1D(Form("h1Timing_planeZ%d",             planeZ), Form("h1Timing_planeZ%d",             planeZ), 200, 0, 60));
    histograms.push_back(new TH1D(Form("h1CorrectedTiming_planeZ%d",    planeZ), Form("h1CorrectedTiming_planeZ%d",    planeZ), 200, 0, 60));
    histograms.push_back(new TH1D(Form("h1PASSInPixelXProj_planeZ%d",   planeZ), Form("h1PASSInPixelXProj_planeZ%d",   planeZ), nX, 0, inPixelX));
    histograms.push_back(new TH1D(Form("h1PASSInPixelYProj_planeZ%d",   planeZ), Form("h1PASSInPixelYProj_planeZ%d",   planeZ), nY, 0, inPixelY));

    return histograms;
}

TH2D* FillHistograms(std::vector<AnalysisHits> analysisHits, std::vector<TH2D*> histograms2D, std::vector<TH1D*> histograms1D, AnaFlags cfg, DetectorConfig detCfg, int planeZ)
{
    auto geoMaps = LoadGeometry(cfg.geoFile, detCfg);
    //TODO: Generalize
    double trackunc_X = 4.6/1000.; // tracking uncertainty in X in unit mm
    double trackunc_Y = 4.6/1000.; // tracking uncertainty in X in unit mm
    int numPixelsX = 2, numPixelsY = 2;
    int nX = numPixelsX*16, nY = numPixelsY*16, nZ = 100; // number of bins for in-pixel maps
    double pixelSizeX = 0.0364 , pixelSizeY = 0.0364; // in mm

    TRandom3 rng(0);
    for (int i =0; i< analysisHits.size(); i++)
    {
        //analysisHits[i];
        double trackOffsetX = cfg.trackOffsetX + geoMaps[analysisHits[i].planeID].x *10;
        double trackOffsetY = cfg.trackOffsetY + geoMaps[analysisHits[i].planeID].y *10;

        histograms2D[kALL]->Fill(analysisHits[i].x, analysisHits[i].y, 1);
        histograms2D[kPASS]->Fill(analysisHits[i].x, analysisHits[i].y, analysisHits[i].clSize > 0 ? 1 : 0);
        histograms2D[kClSize]->Fill(analysisHits[i].x, analysisHits[i].y, analysisHits[i].clSize);
        histograms2D[kTiming]->Fill(analysisHits[i].x, analysisHits[i].y, analysisHits[i].timing);

        double foldedX, foldedY;

        if(cfg.trkUnc == true)
        {
            foldedX = fmod(analysisHits[i].x + trackOffsetX + rng.Gaus(0., trackunc_X), numPixelsX*pixelSizeX) * 1000;
            foldedY = fmod(analysisHits[i].y + trackOffsetY + rng.Gaus(0., trackunc_Y), numPixelsY*pixelSizeY) * 1000;
        } 
        else
        {        
            foldedX = fmod(analysisHits[i].x + trackOffsetX, numPixelsX*pixelSizeX) * 1000;
            foldedY = fmod(analysisHits[i].y + trackOffsetY, numPixelsY*pixelSizeY) * 1000;
        }

        histograms2D[kALLInPixel]->Fill(foldedX, foldedY, 1);
        histograms2D[kPASSInPixel]->Fill(foldedX, foldedY, analysisHits[i].clSize > 0 ? 1 : 0);
        histograms2D[kClSizeInPixel]->Fill(foldedX, foldedY, analysisHits[i].clSize);
        histograms2D[kTimingInPixel]->Fill(foldedX, foldedY, analysisHits[i].timing);
        histograms1D[kTiming1D]->Fill(analysisHits[i].timing);
        histograms1D[kCorrectedTiming]->Fill(analysisHits[i].correctedTiming);        
    }
    histograms2D[kPASS]->Divide(histograms2D[kALL]);
    histograms2D[kPASS]->Scale(100.);
    histograms2D[kClSize]->Divide(histograms2D[kALL]);
    histograms2D[kTiming]->Divide(histograms2D[kALL]);

    TH2D *h2PASSInPixelAux = (TH2D*)histograms2D[kPASSInPixel]->Clone(Form("h2PASSInPixelAux_planeZ%d",planeZ));

    return h2PASSInPixelAux;
}

AnalyzedHit GetStatistics(std::vector<TH2D*> histograms2D, TH2D* auxiliaryHisto)
{
    double avgEff = getEff(histograms2D[kPASSInPixel]->Integral(), histograms2D[kALLInPixel]->Integral());// in percent
    double errEff = getEffErr(histograms2D[kPASSInPixel]->Integral(), histograms2D[kALLInPixel]->Integral());// in percent
    double avgTiming = histograms2D[kTimingInPixel]->Integral() / histograms2D[kPASSInPixel]->Integral();
    double avgClSize = histograms2D[kClSizeInPixel]->Integral() / histograms2D[kPASSInPixel]->Integral();
    double errClSize = getEffErr(histograms2D[kClSizeInPixel]->Integral(), auxiliaryHisto->Integral());

    return {avgEff, errEff, avgTiming, avgClSize, errClSize};
}

void ScaleHistograms(std::vector<TH2D*> histograms2D, std::vector<TH1D*> histograms1D, TH2D* auxiliaryHisto)
{
    histograms2D[kClSizeInPixel]->Divide(auxiliaryHisto);
    histograms2D[kPASSInPixel]->Divide(histograms2D[kALLInPixel]);
    histograms2D[kPASSInPixel]->Scale(100.);
    histograms2D[kTimingInPixel]->Divide(auxiliaryHisto);
    histograms1D[kPASSInPixelXProj] = histograms2D[kALLInPixel]->ProjectionX();
    histograms1D[kPASSInPixelYProj]  = histograms2D[kALLInPixel]->ProjectionY();
}




void SetHistogramStyle(std::vector<TH2D*> histograms2D, std::vector<TH1D*> histograms1D, AnalyzedHit statistics)
{
    histograms2D[kClSizeInPixel]->SetTitle( Form("#bf{MALTA2 Sim.}, 30#mum EPI, <cl. size> =%.2f;Track X pos [#mum];Track Y pos [#mum];Cluster size", statistics.avgClSize) );
    histograms2D[kPASSInPixel]->SetTitle( Form("In-pixel eff. = %.2f %% pm %.2f %% ;Track X pos [#mum];Track Y pos [#mum]; Eff. [%%] ", statistics.avgEff, statistics.errEff) );
    histograms2D[kTimingInPixel]->SetTitle( Form("In-pixel timing. = %.2f ns ;Track X pos [#mum];Track Y pos [#mum]; Timing [ns] ", statistics.avgTiming) );
    histograms1D[kPASSInPixelXProj]->SetTitle("In-pixel eff.;Track X pos [#mum];Eff.[%]");
    histograms1D[kPASSInPixelYProj] ->SetTitle("In-pixel eff.;Track Y pos [#mum];Eff.[%]");
}

void SaveHistograms(std::vector<TH2D*> histograms2D, std::vector<TH1D*> histograms1D, TH2D* auxiliaryHisto, AnaFlags cfg, double threshold, int runNumber, std::string saveName)
{

    // Plot save path
    std::string directoryPath = cfg.localPath +"/Plots/";
    std::string runPath = Form("local_%04d/", runNumber);

    // Write histograms into the directory
    savePlot(directoryPath, runPath, threshold, saveName, histograms2D[kPASS], histograms2D[kPASS]->GetName());
    savePlot(directoryPath, runPath, threshold, saveName, histograms2D[kClSize], histograms2D[kClSize]->GetName());
    savePlot(directoryPath, runPath, threshold, saveName, histograms2D[kTiming], histograms2D[kTiming]->GetName());
    savePlot(directoryPath, runPath, threshold, saveName, histograms1D[kTiming1D], "h1Timing");
    savePlot(directoryPath, runPath, threshold, saveName, histograms1D[kCorrectedTiming], "h1CorrectedTiming");
    savePlot(directoryPath, runPath, threshold, saveName, histograms2D[kPASSInPixel], histograms2D[kPASSInPixel]->GetName());
    savePlot(directoryPath, runPath, threshold, saveName, histograms2D[kClSizeInPixel], histograms2D[kClSizeInPixel]->GetName());
    savePlot(directoryPath, runPath, threshold, saveName, histograms2D[kTimingInPixel], histograms2D[kTimingInPixel]->GetName());
    savePlot(directoryPath, runPath, threshold, saveName, auxiliaryHisto, auxiliaryHisto->GetName());
    savePlot(directoryPath, runPath, threshold, saveName, histograms2D[kALLInPixel], histograms2D[kALLInPixel]->GetName());
    savePlot(directoryPath, runPath, threshold, saveName, histograms1D[kPASSInPixelXProj], histograms1D[kPASSInPixelXProj]->GetName());
    savePlot(directoryPath, runPath, threshold, saveName, histograms1D[kPASSInPixelYProj], histograms1D[kPASSInPixelYProj]->GetName());
}

void SaveSummaryRoot(AnaFlags cfg, int runNumber, std::string saveName, double threshold, AnalyzedHit statistics)
{
    std::string directoryPath = cfg.localPath +"/Plots/";
    std::string runPath = Form("local_%04d/", runNumber);

    // Lastly populate a root tree with the average values for later summary plotting
    // Try opening the file in UPDATE mode (read + write)
    std::string summaryPath = (directoryPath + runPath + saveName + "/summary.root").c_str();
    double summaryThreshold, summaryEff, summaryEffErr, summaryClSize, summaryClSizeErr, summaryTiming;

    TFile *f = TFile::Open(summaryPath.c_str(), "UPDATE");
    if (!f || f->IsZombie()) {
        std::cout << "Creating new file: " << summaryPath << std::endl;
        f = new TFile(summaryPath.c_str(), "RECREATE");
    }
    // Check if the tree exists
    TTree *summaryTree = (TTree*) f->Get("summaryTree");
    if (!summaryTree) 
    {
        std::cout << "Creating new summary tree" << std::endl;
        summaryTree = new TTree("summaryTree", "summary Tree");
        summaryTree->Branch("threshold", &summaryThreshold, "threshold/D");
        summaryTree->Branch("efficiency", &summaryEff, "efficiency/D");
        summaryTree->Branch("effError", &summaryEffErr, "effError/D");
        summaryTree->Branch("clSize", &summaryClSize, "clSize/D");
        summaryTree->Branch("clSizeError", &summaryClSizeErr, "clSizeError/D");
        summaryTree->Branch("timing", &summaryTiming, "timing/D");
    } 
    else 
    {
        std::cout << "Appending to existing tree" << std::endl;
    }

    summaryTree->SetBranchAddress("threshold", &summaryThreshold);
    summaryTree->SetBranchAddress("efficiency",  &summaryEff);
    summaryTree->SetBranchAddress("effError",  &summaryEffErr);
    summaryTree->SetBranchAddress("clSize", &summaryClSize);
    summaryTree->SetBranchAddress("clSizeError", &summaryClSizeErr);
    summaryTree->SetBranchAddress("timing", &summaryTiming);

    summaryThreshold = threshold;
    summaryEff = statistics.avgEff;
    summaryEffErr = statistics.errEff;
    summaryClSize = statistics.avgClSize;
    summaryClSizeErr = statistics.errClSize;
    summaryTiming = statistics.avgTiming;
    summaryTree->Fill();

    std::cout << "Saving values: " << "Threshold: " << threshold << "; avgEff: " << summaryEff << "; summaryEffErr: " 
              << summaryEffErr << "; avgClSize: " << summaryClSize << ";avgTiming: " << summaryTiming << std::endl;

    f->cd();
    summaryTree->Write("", TObject::kOverwrite); // overwrite tree object in directory
    f->Close();
    delete f;
}

std::vector<CaloHits> GetCalorimetryHits(AnaFlags cfg, int runNumber, std::string saveName, double threshold)
{
    std::vector<CaloHits> output;

    std::string inputPath = cfg.inputPath+Form("_%04d/", runNumber);
    std::string inputSubPath = inputPath + saveName + "/";
    TFile *reconstructedFile = TFile::Open((inputSubPath + "Plane0ReconstructedHitsThr" + std::to_string(int(threshold)) + ".root").c_str(), "READ");


    // Get Calo tree
    TTree *reconstructedTree = (TTree*) reconstructedFile->Get("ReconstructedHits");

    int reconstructedPixX, reconstructedPixY, NHits, planeID;
    double reconstructedTiming_float;
    reconstructedTree->SetBranchAddress("planeID", &planeID);
    reconstructedTree->SetBranchAddress("PixX", &reconstructedPixX);
    reconstructedTree->SetBranchAddress("PixY", &reconstructedPixY);
    reconstructedTree->SetBranchAddress("timing", &reconstructedTiming_float);
    reconstructedTree->SetBranchAddress("NHits", &NHits);
    Long64_t nReconstructedEntries = reconstructedTree->GetEntries();

    for (Long64_t i = 0; i<nReconstructedEntries; i++)
    {
        reconstructedTree->GetEntry(i);
        output.push_back({planeID, reconstructedPixX, reconstructedPixY, static_cast<float>(reconstructedTiming_float), NHits});
    }

    return output;
}

std::vector<PositionHits> GetPositionHits(AnaFlags cfg, int runNumber, std::string saveName, double threshold)
{
    std::vector<PositionHits> output;

    std::string inputPath = cfg.inputPath+Form("_%04d/", runNumber);
    std::string inputSubPath = inputPath + saveName + "/";
    TFile *reconstructedFile = TFile::Open((inputSubPath + "Plane0ReconstructedHitsThr" + std::to_string(int(threshold)) + ".root").c_str(), "READ");

    // Get Pos tree
    TTree *positionTree = (TTree*) reconstructedFile->Get("PositionHits");

    int planeID, stripX, stripY;
    double posTiming;
    positionTree->SetBranchAddress("planeID", &planeID);
    positionTree->SetBranchAddress("stripX", &stripX);
    positionTree->SetBranchAddress("stripY", &stripY);
    positionTree->SetBranchAddress("timing", &posTiming);
    Long64_t nPositionEntries = positionTree->GetEntries();

    for (Long64_t i = 0; i<nPositionEntries; i++)
    {
        positionTree->GetEntry(i);
        output.push_back({planeID, stripX, stripY, static_cast<float>(posTiming)});
    }
    return output;
}

void SetClusterCalorimetry(PlaneState& state)
{
    const int dx[4] = { 1, -1,  0,  0 };
    const int dy[4] = { 0,  0,  1, -1 };

    while (!state.pixels.empty())
    {
        state.numClusters++;
        std::stack<Pixel> toVisit;
        Pixel start = *state.pixels.begin();
        toVisit.push(start);
        state.pixels.erase(start);

        while (!toVisit.empty())
        {
            Pixel p = toVisit.top();
            toVisit.pop();

            for (int d = 0; d < 4; ++d)
            {
                Pixel neighbor{p.x + dx[d], p.y + dy[d]};
                auto it = state.pixels.find(neighbor);
                if (it != state.pixels.end())
                {
                    toVisit.push(neighbor);
                    state.pixels.erase(it);
                }
            }
        }
    }
}

void ResetCalorimetry(PlaneState& state, AnaFlags cfg)
{
    state.numSecondaries = 0;
    state.numClusters = 0;
    state.pixels.clear();
    state.timeWindow += cfg.veto;
}


std::pair<std::vector<float>, std::vector<float>> GetPositionCalorimetry(int currentEvent, std::unordered_map<int, Long64_t>& posIndexMap, const std::vector<CaloHits>& caloHits, const std::vector<PositionHits>& positions, PlaneState& state)
{
    // compute X/Y stats using positionTree
    std::vector<float> xVals;
    std::vector<float> yVals;

    auto& posIndex = posIndexMap[caloHits[currentEvent].planeID];

    while (posIndex < positions.size())
    {
        
        //std::cout << posTiming << std::endl;
        if (positions[posIndex].planeID != caloHits[currentEvent].planeID) 
        {
            posIndex++;
            continue;
        }
        if (positions[posIndex].time < state.timeWindow)
        {
            xVals.push_back(positions[posIndex].stripX);
            yVals.push_back(positions[posIndex].stripY);
            posIndex++;
        }
        else break;
    }
    return {xVals, yVals};
}


std::vector<FullCalorimetryInfo> ProcessCalorimetry(std::vector<CaloHits> calorimetryHits, std::vector<PositionHits> positionHits, AnaFlags cfg)
{
    std::unordered_set<Pixel, PixelHash> pixelsToCluster{};
    std::unordered_map<int, PlaneState> planeStates;
    std::unordered_map<int, Long64_t> posIndexMap;
    std::unordered_map<int, Long64_t> eventIDMap;
    std::vector<FullCalorimetryInfo> output;
    float meanX, meanY, sigmaX, sigmaY;

    for (Long64_t i = 0; i < calorimetryHits.size(); i++)
    {

        //std::cout << " Event: " << i << " out of total: " << calorimetryHits.size() << std::endl;
        int planeID = calorimetryHits[i].planeID;
        int pixX = calorimetryHits[i].x;
        int pixY = calorimetryHits[i].y;
        float timing = static_cast<float>(calorimetryHits[i].time);
        auto& state = planeStates[calorimetryHits[i].planeID];
        //std::cout << "Index: " << i << "; timing: " << timing << "; window: " << state.timeWindow << "; planeID: " << planeID << "; pixX: " << pixX << "; pixY: " << pixY << " ; sec: " << state.numSecondaries << std::endl;

        // Initialize time window on first hit
        if (state.timeWindow == 0) state.timeWindow = cfg.veto;

        if (timing < state.timeWindow)
        {
            if (cfg.boolHWC) state.numSecondaries += calorimetryHits[i].nHit;
            else state.numSecondaries++;
            state.pixels.insert({pixX, pixY});
        }
        else
        {
            SetClusterCalorimetry(state);
            // Output
            auto [xVals, yVals] = GetPositionCalorimetry(i, posIndexMap, calorimetryHits, positionHits, state);
            computeStats(xVals, meanX, sigmaX);
            computeStats(yVals, meanY, sigmaY);
            auto& eventID = eventIDMap[planeID];            
            output.push_back({meanX, meanY, sigmaX, sigmaY, planeID, (int)eventID, state.numSecondaries, state.numClusters});
            eventID++;
            // Reset
            ResetCalorimetry(state, cfg);
        }
    }
    output = ProcessLastEventCalorimetry(output, planeStates, posIndexMap, eventIDMap, calorimetryHits, positionHits);

    return output;
}


std::vector<FullCalorimetryInfo> ProcessLastEventCalorimetry(std::vector<FullCalorimetryInfo> output, std::unordered_map<int, PlaneState>& planeStates, std::unordered_map<int, Long64_t>& posIndexMap, std::unordered_map<int, Long64_t>& eventIDMap, std::vector<CaloHits> caloHits, std::vector<PositionHits> positions)
{
    float meanX, meanY, sigmaX, sigmaY;
    // Go through also the last remaining window for each plane
    for (auto& [pid, state] : planeStates)
    {
        if (state.pixels.empty()) continue;
        SetClusterCalorimetry(state);

        auto& eventID = eventIDMap[pid];
        auto [xVals, yVals] = GetPositionCalorimetry((int)eventID, posIndexMap, caloHits, positions, state);

        computeStats(xVals, meanX, sigmaX);
        computeStats(yVals, meanY, sigmaY);
        output.push_back({meanX, meanY, sigmaX, sigmaY, pid, (int)eventID, state.numSecondaries, state.numClusters});
    }
    return output;
}

void FillClusterTree(AnaFlags cfg, int runNumber, std::string saveName, double threshold, std::vector<FullCalorimetryInfo> caloHits)
{
    std::string inputPath = cfg.inputPath+Form("_%04d/", runNumber);
    std::string inputSubPath = inputPath + saveName + "/";
    TFile *outfile = new TFile((inputSubPath + "CalorimetryThr" + std::to_string(int(threshold)) + ".root").c_str(), "RECREATE");

    // Create a TTree
    TTree *caloTree = new TTree("CaloHits", "Calorimetry Hits");

    // Variables for branches
    int numSecondaries, numClusters, eventNum, planeNum;
    float meanX, meanY, sigmaX, sigmaY;
    // Create branches
    caloTree->Branch("meanX", &meanX, "meanX/F");
    caloTree->Branch("meanY", &meanY, "meanY/F");
    caloTree->Branch("sigmaX", &sigmaX, "sigmaX/F");
    caloTree->Branch("sigmaY", &sigmaY, "sigmaY/F");
    caloTree->Branch("planeID", &planeNum, "planeID/I");
    caloTree->Branch("eventID", &eventNum, "eventID/I");
    caloTree->Branch("numSecondaries", &numSecondaries, "numSecondaries/I");
    caloTree->Branch("numClusters", &numClusters, "numClusters/I");

    for (const auto& el : caloHits)
    {
        meanX = el.meanX;
        meanY = el.meanY;
        sigmaX = el.sigmaX;
        sigmaY = el.sigmaY;
        planeNum = el.planeID;
        eventNum = el.eventID;
        numSecondaries = el.numSec;
        numClusters = el.numCl;
        caloTree->Fill();
    }

    caloTree->Write();
    outfile->Close();
}

RawCalorimetryPerMap GetCalorimetryAnalyzedHits( int runNumber, std::string saveName, double threshold)
{
    RawCalorimetryPerMap output;
    std::string summaryPath = Form("Results/local_%04d/%s/CalorimetryThr%i.root", runNumber, saveName.c_str(), int(threshold));
    TFile *summaryFile = TFile::Open(summaryPath.c_str(), "READ");
    TTree *summaryTree = (TTree*) summaryFile->Get("CaloHits");
    Long64_t nSummaryEntries = summaryTree->GetEntries();
    int numSecondaries, numClusters, planeID, eventID;
    float meanX, meanY;
    std::vector<int> histoSecondaries;
    histoSecondaries.reserve(nSummaryEntries);
    std::vector<int> histoClusters;
    histoClusters.reserve(nSummaryEntries);

    std::unordered_map<int, std::vector<int>> secondariesPerPlane;
    std::unordered_map<int, int> secondariesPerEvent;
    std::unordered_map<int, std::vector<int>> secondariesVecPerEvent;
    std::unordered_map<int, std::vector<int>> clustersPerPlane;
    std::unordered_map<int, std::vector<int>> xPosPerPlane;
    std::unordered_map<int, std::vector<int>> yPosPerPlane;

    summaryTree->SetBranchAddress("planeID", &planeID);
    summaryTree->SetBranchAddress("eventID", &eventID);
    summaryTree->SetBranchAddress("meanX", &meanX);
    summaryTree->SetBranchAddress("meanY", &meanY);
    summaryTree->SetBranchAddress("numSecondaries", &numSecondaries);
    summaryTree->SetBranchAddress("numClusters", &numClusters);

    for (Long64_t i = 0; i < nSummaryEntries; i++)
    {
        summaryTree->GetEntry(i);
        secondariesPerEvent[eventID] += numSecondaries;
        secondariesVecPerEvent[eventID].push_back(numSecondaries);
        secondariesPerPlane[planeID].push_back(numSecondaries);
        clustersPerPlane[planeID].push_back(numClusters);
        xPosPerPlane[planeID].push_back(meanX);
        yPosPerPlane[planeID].push_back(meanY);
    }
    return {secondariesPerPlane, secondariesPerEvent, secondariesVecPerEvent, clustersPerPlane, xPosPerPlane, yPosPerPlane};
}

std::vector<FitCalorimetryInfo> GetCalorimetryMultiLayerFitInformation(RawCalorimetryPerMap rawCaloMap)
{
    auto secondariesVecPerEvent = rawCaloMap.secVecPerEvent;
    std::vector<FitCalorimetryInfo> output;
    double radLength = 0.86;
    for (Long64_t i = 0; i< secondariesVecPerEvent.size(); i++)
    {
        std::vector<double> vsecondaries{};
        std::vector<int> vlayer{};
        std::vector<double> vradLength{};
        for (int j = 0 ; j< secondariesVecPerEvent[i].size() ; j+=2)
        {
            // layer index (each pair corresponds to one layer)
            vlayer.push_back(j / 2);
            vradLength.push_back((j/2 + 1)* radLength);

            // make sure we don't go out of bounds
            int sum = secondariesVecPerEvent[i][j];

            if (j + 1 < secondariesVecPerEvent[i].size())
                sum += secondariesVecPerEvent[i][j + 1];

            vsecondaries.push_back(sum);
        }
        TGraph *gSecondaries = new TGraph(vradLength.size(), vradLength.data(), vsecondaries.data());

        TF1 *f = new TF1("f", "[0]*pow([1]*x,[2]-1)*exp(-[1]*x)", 0, vradLength.back());
        const double A_init = 500;
        const double beta_init = 0.5;
        const double alpha_init = 1.7;
        f->SetParameters(A_init, beta_init, alpha_init);
        TFitResultPtr r = gSecondaries->Fit(f,"QS");
        double chi2 = f->GetChisquare();
        double ndf  = f->GetNDF();
        double chi2ndf = chi2 / ndf;

        if (r->Status() != 0) 
        {
            std::cout << "Fit for event " << i << " failed with status " << r->Status() << std::endl;
            delete gSecondaries;
            delete f;
            continue;  // skip pushing bad fits
        }

        double alpha = f->GetParameter(2);
        double beta  = f->GetParameter(1);
        double A = f->GetParameter(0);
        // Shower max position
        double tmax{};
        if (beta!=0) tmax = (alpha - 1)/beta;
        // Total energy deposition
        double totEnergy = f->Integral(0, vradLength.back());
        output.push_back({tmax, totEnergy, chi2ndf, A, beta, alpha});
    }
    return output;
}

void SaveCalorimetryHistograms(RawCalorimetryPerMap rawCaloMap, std::vector<FitCalorimetryInfo> fitCaloMap, int runNumber, std::string saveName, double threshold)
{
    gROOT->SetBatch(kTRUE);
    TFile *outHistFile = new TFile(Form("Plots/local_%04d/%s/CaloHistos.root",runNumber, saveName.c_str()), "RECREATE");
    // Secondaries Histo
    auto secondariesPerEvent = rawCaloMap.secPerEvent;
    auto secondariesPerPlane = rawCaloMap.secPerPlane;
    auto clustersPerPlane = rawCaloMap.clPerPlane;
    auto xPosPerPlane = rawCaloMap.xPosPerPlane;
    auto yPosPerPlane = rawCaloMap.yPosPerPlane;


    auto [minIt, maxIt] = minMaxByProjection(secondariesPerEvent, [](const auto& v) { return v; });
    double minSecSum = minIt->second;
    double maxSecSum = maxIt->second;
    
    TH1D* hSecSum = new TH1D(Form("hSecSum_run%d_file%s_thr%d", runNumber, saveName.c_str(), (int)threshold), Form(";Secondaries;Counts"), 50, minSecSum, maxSecSum);

    for (const auto& [pid, secSum] : secondariesPerEvent)
    {
        hSecSum->Fill(secSum);
    }

    auto [minTmax, maxTmax]     = fieldRange(fitCaloMap, [](const auto& e) { return e.tmax; });
    auto [minEnergy, maxEnergy] = fieldRange(fitCaloMap, [](const auto& e) { return (float)e.energy; });
    auto [minChi2ndf, maxChi2ndf] = fieldRange(fitCaloMap, [](const auto& e) { return (float)e.chi2ndf; });
    auto [minP0, maxP0] = fieldRange(fitCaloMap, [](const auto& e) { return (float)e.p0; });
    auto [minP1, maxP1] = fieldRange(fitCaloMap, [](const auto& e) { return (float)e.p1; });
    auto [minP2, maxP2] = fieldRange(fitCaloMap, [](const auto& e) { return (float)e.p2; });
    TH1D* hTmax = new TH1D("hTmax", "tmax", 100, minTmax, maxTmax);
    TH1D* hEnergyInt = new TH1D("hEnergyInt", "energy integral", 100, minEnergy, maxEnergy);
    TH1D* hChi2ndf = new TH1D("hChi2ndf", "chi2ndf", 100, minChi2ndf, maxChi2ndf);
    TH1D* hP0 = new TH1D("hP0", "p0", 100, minP0, maxP0);
    TH1D* hP1 = new TH1D("hP1", "p1", 100, minP1, maxP1);
    TH1D* hP2 = new TH1D("hP2", "p2", 100, minP2, maxP2);

    for (int i = 0; i < fitCaloMap.size(); i++)
    {
        auto fitCaloInfo = fitCaloMap[i]; 
        hTmax->Fill(fitCaloInfo.tmax);
        hEnergyInt->Fill(fitCaloInfo.energy);
        hChi2ndf->Fill(fitCaloInfo.chi2ndf);
        hP0->Fill(fitCaloInfo.p0);
        hP1->Fill(fitCaloInfo.p1);
        hP2->Fill(fitCaloInfo.p2);
    }
    outHistFile->cd();
    hSecSum->Write();
    hTmax->Write();
    hEnergyInt->Write();
    hChi2ndf->Write();
    hP0->Write();
    hP1->Write();
    hP2->Write();

    for (const auto& [pid, secVec] : secondariesPerPlane)
    {
        const auto& cluVec = clustersPerPlane[pid];
        const auto& xPosVec = xPosPerPlane[pid];
        const auto& yPosVec = yPosPerPlane[pid];
        if (secVec.empty()) continue;
        // Secondaries Histo
        double minSec = *std::min_element(secVec.begin(), secVec.end());
        double maxSec = *std::max_element(secVec.begin(), secVec.end());
        TH1D* hSec = new TH1D(
            Form("hSec_plane%d_run%d_file%s_thr%d", pid, runNumber, saveName.c_str(), (int)threshold),
            Form("Plane %d;Secondaries;Counts", pid),
            50, minSec, maxSec
        );

        for (auto v : secVec) hSec->Fill(v);
        if (cluVec.empty()) continue;

        // Cluster Histo
        double minClu = *std::min_element(cluVec.begin(), cluVec.end());
        double maxClu = *std::max_element(cluVec.begin(), cluVec.end());

        TH1D* hClu = new TH1D(
            Form("hClu_plane%d_run%d_file%s_thr%d", pid, runNumber, saveName.c_str(), (int)threshold),
            Form("Plane %d;Clusters;Counts", pid),
            50, minClu, maxClu
        );

        for (auto v : cluVec) hClu->Fill(v);

        if (xPosVec.empty()) continue;

        // X position Histo
        double minX = *std::min_element(xPosVec.begin(), xPosVec.end());
        double maxX = *std::max_element(xPosVec.begin(), xPosVec.end());
        if (minX == maxX) { minX -= 0.5; maxX += 0.5; }

        TH1D* hX = new TH1D(
            Form("hX_plane%d_run%d_file%s_thr%d", pid, runNumber, saveName.c_str(), (int)threshold),
            Form("Plane %d;X position;Counts", pid),
            50, minX, maxX
        );

        for (auto v : xPosVec) hX->Fill(v);

        if (yPosVec.empty()) continue;

        // Y position Histo
        double minY = *std::min_element(yPosVec.begin(), yPosVec.end());
        double maxY = *std::max_element(yPosVec.begin(), yPosVec.end());
        if (minY == maxY) { minY -= 0.5; maxY += 0.5; }

        TH1D* hY = new TH1D(
            Form("hY_plane%d_run%d_file%s_thr%d", pid, runNumber, saveName.c_str(), (int)threshold),
            Form("Plane %d;Y position;Counts", pid),
            50, minY, maxY
        );

        for (auto v : yPosVec) hY->Fill(v);

        // Save all to root file instead
        outHistFile->cd();
        hSec->Write();
        hClu->Write();
        hX->Write();
        hY->Write();
    }
    gDirectory->Delete("*");
    outHistFile->Write();
    outHistFile->Close();
}

std::vector<MALTA3Word> EncodeMALTA3ReducedWord(EnergyMap enMap, SortedTimeVector sortedTimings, ThresholdMap thresholdMap, AnaFlags cfg)
{
    std::vector<MALTA3Word> words{};
    double prevTiming{};
    __uint128_t groupMerger{};
    int count{};
    for (const auto& entry : sortedTimings) 
    {
        const HitKey& key = entry.first;
        int eventID = entry.first.plane;
        int pixX   = entry.first.x;
        int pixY   = entry.first.y;
        double timing = entry.second; 
        count++;
        auto itThr = thresholdMap.find({pixX, pixY});
        
        double threshold = itThr->second;
        auto it = enMap.find(key);
        double cenergy = it->second;
    
        if (cenergy < threshold) continue;
        
        __uint128_t word = encodeWord(pixX, pixY, cfg.groupSizeX, cfg.groupSizeY, cfg.groupLeng, cfg.parityLeng, cfg.dColLeng, false);
        if (count == 1) 
        {
            prevTiming = timing;
            groupMerger = word;
        }
        words.push_back({word, timing});
    }
    return words;
}

std::unordered_map<int, std::vector<MALTA3Word>> SortWordsByBus(std::vector<MALTA3Word> words)
{
    std::unordered_map<int, std::vector<MALTA3Word>> busWords;
    for (auto&[word, timing] : words)
    {
        // This is not generalized for any bit size
        int doubleColumn = word& 0xFF;
        int parity = (word & ((__uint128_t)1 << 8)) != 0;
        //int bus = ( parity + 1) * doubleColumn;
        int bus = 2 * doubleColumn + parity;
        busWords[bus].push_back({word,timing});
    }
    return busWords;
}

std::map<MatrixSection, std::vector<MALTA3Word>> SortWordsByGroup(std::unordered_map<int, std::vector<MALTA3Word>> busWords)
{
    std::map<MatrixSection, std::vector<MALTA3Word>> groupWords;
    for (auto& [bus, words] : busWords)
    {
        for (auto& word : words)
        {
            __uint128_t group = (word.word >> 9) & ((__uint128_t)0x1F); 
            groupWords[{group, bus}].push_back(word);
        }
    }
    return groupWords;
}

std::vector<MALTA3Word> GroupMerge(std::map<MatrixSection, std::vector<MALTA3Word>> groupWords, AnaFlags cfg)
{
    std::vector<MALTA3Word> wordsAfterGroup{};
    for (auto& [ID, words]: groupWords)
    {
        std::sort(words.begin(), words.end(), [](auto &a, auto &b) {return a.time < b.time;});
        std::vector<MALTA3Word> groupMerged;
        for (size_t i = 0; i < words.size(); i++)
        {
            
            auto [word, timing] = words[i];
            if (groupMerged.empty())
            {
                groupMerged.push_back({word, timing});
                continue;
            }
            auto &[lastWord, lastTime] = groupMerged.back();

            if(timing - lastTime <= cfg.slowcontrolDelay)
            {
                lastWord|=word;
            }
            else
            {
                groupMerged.push_back({word, timing});
            }
            
        }
        wordsAfterGroup.insert(wordsAfterGroup.end(), groupMerged.begin(), groupMerged.end());
    }
    return wordsAfterGroup;
}

std::vector<MALTA3Word> BusMerge(std::unordered_map<int, std::vector<MALTA3Word>> busWords, AnaFlags cfg)
{
    std::vector<MALTA3Word> wordsAfterBus{};
    for (auto& [busID,words]: busWords)
    {
        // Sort by timing
        std::sort(words.begin(), words.end(), [](auto &a, auto &b) {return a.time < b.time;});
        std::vector<MALTA3Word> busMerged;
        for (size_t i = 0; i < words.size(); i++)
        {
            auto [word, timing] = words[i];
            if (busMerged.empty())
            {
                busMerged.push_back({word, timing});
                continue;
            }
            auto &[lastWord, lastTime] = busMerged.back();

            if(timing - lastTime <= cfg.busMergingThreshold)
            {
                lastWord|=word;
            }
            else
            {
                busMerged.push_back({word, timing});
            }
            
        }

        wordsAfterBus.insert(wordsAfterBus.end(), busMerged.begin(), busMerged.end());
    }
    std::sort(wordsAfterBus.begin(), wordsAfterBus.end(), [](auto &a, auto &b){return a.time < b.time;});

    return wordsAfterBus;
}

int SelectSector(SRAMState& state, AnaFlags cfg)
{
    int nSectors = (512 + (2 * cfg.sectorSize) - 1) / (2 * cfg.sectorSize);
    if (cfg.prioAlgo == "RoundRobin")
    {
        int sector = state.occupiedSectors.front();
        state.occupiedSectors.pop();
        return sector;
    }

    std::vector<int> filledBuses(nSectors, 0);
    for (int i = 0; i < 512; i++)
    {
        int currentSector = static_cast<int>(i / (2 * cfg.sectorSize));
        if (cfg.prioAlgo == "MostFilled" && state.memoryModule[i] > 0) filledBuses[currentSector]++;
        if (cfg.prioAlgo == "MostFull"   && state.memoryModule[i] > 0) filledBuses[currentSector] += state.memoryModule[i];
    }
    state.occupiedSectors.pop();
    auto it = std::max_element(filledBuses.begin(), filledBuses.end());
    return std::distance(filledBuses.begin(), it);
}


DrainResult DrainSector(int sector, SRAMState& state, AnaFlags cfg)
{
    int start = sector * 2 * cfg.sectorSize;
    int end   = std::min(start + 2 * cfg.sectorSize, 512);

    DrainResult result{};
    for (int i = start; i < end; i++)
    {
        if (state.memoryModule[i] > 0 && !state.memoryWordStore[i].empty())
        {
            uint32_t fourBit = state.memoryWordStore[i].front().word & 0xF;
            result.reducedWords.push_back(fourBit);
            result.improvTiming = std::max(result.improvTiming, state.memoryWordStore[i].front().time);
            state.memoryWordStore[i].erase(state.memoryWordStore[i].begin());
            state.memoryModule[i]--;
            state.sectorOccupancy[sector]--;
            result.numValids++;
        }
        else result.reducedWords.push_back(0);
    }
    return result;
}


MALTA3Word BuildHWCWord(const DrainResult& drain, int sector, AnaFlags cfg)
{
    __uint128_t HWCWord{};
    auto vcompressedWords = CompressWords(drain.reducedWords, cfg.wordSize);
    for (const auto& compWord : vcompressedWords)
        HWCWord = (HWCWord << cfg.wordSize) | (compWord & ((1u << cfg.wordSize) - 1));

    uint8_t sixBit = sector & 0x3F;
    __uint128_t fullHWCWord = (HWCWord << 6) | sixBit;
    return {fullHWCWord, drain.improvTiming};
}


void ProcessReadCycle(SRAMState& state, std::vector<MALTA3Word>& wordsAfterSRAM, AnaFlags cfg)
{
    if (state.occupiedSectors.empty()) return;

    int sector  = SelectSector(state, cfg);
    auto drain  = DrainSector(sector, state, cfg);

    if (state.sectorOccupancy[sector] > 0)
        state.occupiedSectors.push(sector);

    auto hwcWord = BuildHWCWord(drain, sector, cfg);
    if (hwcWord.word != 0)
        wordsAfterSRAM.push_back(hwcWord);
}


void WriteToMemory(int bus, __uint128_t word, double timing, SRAMState& state, AnaFlags cfg)
{
    if (state.memoryModule[bus] >= cfg.sramDepth) { state.missedHitCount++; return; }

    int sector = bus / (2 * cfg.sectorSize);
    if (state.sectorOccupancy[sector] == 0) state.occupiedSectors.push(sector);

    int pixAddr = (word >> 14) & 0xFFFF;
    __uint128_t HWCWord = __builtin_popcount(pixAddr);
    state.memoryWordStore[bus].push_back({HWCWord, timing});
    state.memoryModule[bus]++;
    state.sectorOccupancy[sector]++;
}

std::vector<MALTA3Word> MemorySynchronize(std::vector<MALTA3Word> wordsAfterBus, AnaFlags cfg)
{
    int nSectors = (512 + (2 * cfg.sectorSize) - 1) / (2 * cfg.sectorSize);
    SRAMState  state{nSectors};
    std::vector<MALTA3Word> wordsAfterSRAM{};

    for (auto& [word, timing] : wordsAfterBus)
    {
        int doubleColumn = word & 0xFF;
        int parity       = (word & ((__uint128_t)1 << 8)) != 0;
        //int bus          = ( parity + 1) * doubleColumn;
        int bus          = 2 * doubleColumn + parity;
        state.timeMEMSYNC     += timing - state.prevGlobalTiming;
        state.prevGlobalTiming = timing;

        int nRead = floor(state.timeMEMSYNC / cfg.SRAMFrequency);
        for (int k = 0; k < nRead; k++)
        {
            ProcessReadCycle(state, wordsAfterSRAM, cfg);
            state.timeMEMSYNC -= cfg.SRAMFrequency;
        }
        WriteToMemory(bus, word, timing, state, cfg);
    }

    std::sort(wordsAfterSRAM.begin(), wordsAfterSRAM.end(), [](auto& a, auto& b) { return a.time < b.time; });
    return wordsAfterSRAM;
}

std::vector<MALTA3Word> FIFOPass(std::vector<MALTA3Word> wordsAfterSRAM, AnaFlags cfg)
{
    double prevFIFOTiming{};
    double timeFIFO{};
    int fifoFill{};
    std::vector<MALTA3Word> wordsAfterFIFO{};  

    for (auto& [word, timing]:wordsAfterSRAM)
    {
        double prevTiming = prevFIFOTiming ;
        double timeDiff = timing - prevTiming;
        timeFIFO += timeDiff;
        prevFIFOTiming = timing;
        int nRead = floor(timeFIFO / cfg.FIFOFrequency);
        if(nRead >= 1)
        {
            for(int k = 0; k < nRead; k++) 
            {
                if (fifoFill) fifoFill--;
            }
            timeFIFO -= nRead * cfg.FIFOFrequency;
        }
        if(fifoFill < cfg.fifoSize)
        {
            wordsAfterFIFO.push_back({word, timing});
            fifoFill++;
        }
    }
    return wordsAfterFIFO;
 }

std::pair<std::vector<ProcessedHit>, std::vector<ProcessedHit>> DecodeMALTA3HWCHits( std::vector<MALTA3Word> wordsAfterFIFO, int planeID, Offset offset, AnaFlags cfg)
{
    std::vector<ProcessedHit> hitTree, positionTree;
    double dutAxisRotation = offset.zrot;

    for (const auto &word: wordsAfterFIFO)
    {
        int nHits = 0;
        double reconstructedTiming = word.time;
        int reconstructedPixX, reconstructedPixY;
        int stripSaveX, stripSaveY;
        double timingSave;
        // Sector position
        if (dutAxisRotation != 90)
        {
            reconstructedPixX =  (word.word & 0x3F) * cfg.sectorSize;
            reconstructedPixY = -1; // Only strip X info
        }
        else
        {
            reconstructedPixX =  -1;
            reconstructedPixY = (word.word & 0x3F) * cfg.sectorSize; // Only strip Y info
        }
        __uint128_t x = word.word >> 6; 
        int wrongnHits = __builtin_popcountll(x);
        uint32_t mask = (1u << cfg.wordSize) - 1;
        int count = 0;
        while (x) 
        {
            nHits += (x & mask);
            x >>= cfg.wordSize;

            if (dutAxisRotation != 90)
            {
                stripSaveX = reconstructedPixX + count;
                stripSaveY = -1;
            }
            else
            {
                stripSaveX = -1;
                stripSaveY = reconstructedPixY + count;
            }
            timingSave = reconstructedTiming;
            count++;

            positionTree.push_back({planeID, stripSaveX, stripSaveY, timingSave});
        }
        hitTree.push_back({planeID, reconstructedPixX, reconstructedPixY, timingSave, nHits});
    }
    return {hitTree, positionTree};
}

void FillMALTA3HWCTree(std::pair<std::vector<ProcessedHit>, std::vector<ProcessedHit>> malta3Hits, AnaFlags cfg, double threshold, int runNumber, std::string saveName)
{
    auto hitTree = malta3Hits.first;
    auto positionTree = malta3Hits.second;

    std::string inputPath = cfg.inputPath + Form("_%04d/", runNumber);
    mkdir((inputPath + saveName).c_str(), 0777);
    TFile *outfile = new TFile((inputPath + saveName + "/Plane0ReconstructedHitsThr" + std::to_string(int(threshold)) + ".root").c_str(), "RECREATE");
    outfile->cd();
    // Create a TTree
    TTree *reconstructedTree = new TTree("ReconstructedHits", "Reconstructed Hits");
    // Variables for branches
    int reconstructedPixX, reconstructedPixY, nHits, planeVal;
    double reconstructedTiming;
    // Create branches
    reconstructedTree->Branch("planeID", &planeVal, "planeID/I");
    reconstructedTree->Branch("PixX", &reconstructedPixX, "PixX/I");
    reconstructedTree->Branch("PixY", &reconstructedPixY, "PixY/I");
    reconstructedTree->Branch("timing", &reconstructedTiming, "timing/D");
    reconstructedTree->Branch("NHits", &nHits, "NHits/I");
    // Create a TTree
    TTree *posTree = new TTree("PositionHits", "Position Hits");
    int stripSaveX, stripSaveY;
    double timingSave;
    // Create branches
    posTree->Branch("planeID", &planeVal, "planeID/I");
    posTree->Branch("stripX",   &stripSaveX, "stripX/I");
    posTree->Branch("stripY",   &stripSaveY, "stripY/I");
    posTree->Branch("timing",  &timingSave, "timing/D");

    for(const auto& el : hitTree)
    {
        planeVal = el.planeID;
        reconstructedPixX = el.x;
        reconstructedPixY = el.y;
        reconstructedTiming = el.time;
        nHits = el.nHit;
        reconstructedTree->Fill();
    }

    for(const auto& el: positionTree)
    {
        planeVal = el.planeID;
        stripSaveX = el.x;
        stripSaveY = el.y;
        timingSave = el.time;
        posTree->Fill();
    }
    outfile->cd();
    reconstructedTree->Write();
    posTree->Write();
    outfile->Close();
}
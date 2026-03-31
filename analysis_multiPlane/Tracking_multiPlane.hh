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
    double momX, momY, momZ;
};

struct Module 
{
    int x, y, z;
    double xoff, yoff, zoff, xrot, yrot, zrot;
    int modID;
};

struct Offset
{
    double x, y, z;
    double xrot, yrot, zrot;
    double R[3][3];
};

struct Vec3
{
    double x, y, z;
};


inline double dot(const Vec3& a, const Vec3& b)
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

inline Vec3 ApplyGeometry3D(const Vec3& p, Offset& g)
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
inline Vec3 PixelPositionReconstruction(int pixelX, int pixelY, const DetectorConfig& cfg)
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


// Main tracking function
void Tracking_multiPlane(int runNumber, std::string saveName);
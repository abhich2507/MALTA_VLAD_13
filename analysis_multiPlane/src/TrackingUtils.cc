#include "TrackingUtils.hh"
#include <cmath>
#include <string>
#include "Utils.hh"
#include <TString.h>
#include <RtypesCore.h>

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
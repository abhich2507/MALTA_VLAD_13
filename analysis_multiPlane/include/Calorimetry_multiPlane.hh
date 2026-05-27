
#pragma once
#include <unordered_set>
#include <vector>
#include <string>
#include <cstddef>  
#include <math.h>
#include <unordered_map>
#include <algorithm>
#include <numeric>

struct Pixel
{
    int x;
    int y;
    // Overload == operator
    bool operator==(const Pixel& other) const 
    {
        return x == other.x && y == other.y;
    }
};
// Hash value creation for XOR cluster ranking if a ==b hash(a) == hash(b)
struct PixelHash 
{
    std::size_t operator()(const Pixel& p) const 
    {
        return std::hash<int>()(p.x) ^ (std::hash<int>()(p.y) << 1);
    }
};
struct PlaneState 
{
    float timeWindow = 0;
    int numSecondaries = 0;
    int numClusters = 0;
    std::unordered_set<Pixel, PixelHash> pixels;
};
struct CaloHits
{
    int planeID;
    int x;
    int y;
    float time;
    int nHit;
};
struct PositionHits
{
    int planeID;
    int stripX;
    int stripY;
    float time;
};
struct FullCalorimetryInfo
{
    double meanX;
    double meanY;
    double sigmaX;
    double sigmaY;
    int64_t planeID;
    int eventID;
    int numSec;
    int numCl;

};
struct RawCalorimetryPerMap
{
    std::unordered_map<int, std::vector<int>> secPerPlane;
    std::unordered_map<int, int> secPerEvent;
    std::unordered_map<int, std::vector<int>> secVecPerEvent;
    std::unordered_map<int, std::vector<int>> clPerPlane;
    std::unordered_map<int, std::vector<int>> xPosPerPlane;
    std::unordered_map<int, std::vector<int>> yPosPerPlane;
};
struct FitCalorimetryInfo
{
    double tmax;
    double energy;
    double chi2ndf;
    double p0;
    double p1;
    double p2;
};
template<typename Map, typename Proj>
auto minMaxByProjection(const Map& m, Proj proj)
{
    auto cmp = [&](const auto& a, const auto& b) {
        return proj(a.second) < proj(b.second);
    };
    auto minIt = std::min_element(m.begin(), m.end(), cmp);
    auto maxIt = std::max_element(m.begin(), m.end(), cmp);
    return std::make_pair(minIt, maxIt);
}
template<typename Vec, typename Proj>
std::pair<float, float> fieldRange(const Vec& v, Proj proj, float guardSigma = 3.0f)
{
    std::vector<float> vals;
    vals.reserve(v.size());
    for (const auto& e : v) vals.push_back(proj(e));

    float mean = std::accumulate(vals.begin(), vals.end(), 0.f) / vals.size();
    float var  = 0;
    for (auto x : vals) var += (x - mean) * (x - mean);
    float sigma = std::sqrt(var / vals.size());

    float lo =  std::numeric_limits<float>::max();
    float hi = -std::numeric_limits<float>::max();
    for (auto x : vals)
    {
        if (std::abs(x - mean) > guardSigma * sigma) continue;
        lo = std::min(lo, x);
        hi = std::max(hi, x);
    }
    return {lo, hi};
}
auto computeStats = [](const std::vector<float>& v, float& mean, float& sigma)
{
    
    //Mean and Sigma computation helper function
    
    if (v.empty()) {
        mean = 0;
        sigma = 0;
        return;
    }

    float sum = 0;
    for (auto val : v) sum += val;
    mean = sum / v.size();

    float var = 0;
    for (auto val : v) var += (val - mean) * (val - mean);
    sigma = std::sqrt(var / v.size());
};

void Calorimetry_multiPlane(float threshold, int runNumber, std::string saveName);
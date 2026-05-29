#include "RootIO.hh"
#include "Utils.hh"
#include "CalorimetryUtils.hh"
#include "TrackingUtils.hh"
#include "Debug_multiPlane.hh"
#include <cassert>
#include <iostream>
#include <cmath>
#include <sstream>
#include "TError.h"

double sigma(const std::vector<double>& v)
{
    if (v.size() < 2) return 0.0;
    double mean = std::accumulate(v.begin(), v.end(), 0.0) / v.size();
    double sq_sum = std::accumulate(v.begin(), v.end(), 0.0,
        [mean](double acc, double x) { return acc + (x - mean)*(x - mean); });
    return std::sqrt(sq_sum / (v.size() - 1)); // sample std dev
}

std::pair<double,double> residualSigma(std::string configPath)
{
    std::stringstream buffer;
    std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());
    setenv("ANALYSIS_CONFIG", configPath.c_str(), 1);
    DigitalProcessing_multiPlane(0, 0, "Test", false);
    auto config = GetDigitalConfig();
    auto hits = GetTrackHits(config, 0, 0, "Test", 0);
    auto tracks = GetVertex(config, 0);
    auto [fullTrackInfo, residualInfo] = MatchHits(tracks, hits, config, 0);
    std::cout.rdbuf(old);
    std::vector<double> rx, ry;
    for (const auto& res : residualInfo)
    {
        rx.push_back(res.rx);
        ry.push_back(res.ry);
    }
    return {sigma(rx), sigma(ry)};
}


void test_idealResidual()
{
    std::string configPath = "analysis_multiPlane/tests/integration/test_fullDigitalProcessing/config.cfg";
    std::stringstream buffer;
    std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());
    setenv("ANALYSIS_CONFIG", configPath.c_str(), 1);
    DigitalProcessing_multiPlane(0, 0, "Test", false);
    auto config = GetDigitalConfig();
    auto hits = GetTrackHits(config, 0, 0, "Test", 0);
    auto tracks = GetVertex(config, 0);
    auto [fullTrackInfo, residualInfo] = MatchHits(tracks, hits, config, 0);
    std::cout.rdbuf(old);
    for (const auto& res : residualInfo)
    {
        assert(std::abs(res.rx) < 0.1);
        assert(std::abs(res.ry) < 0.1);    
    }
}

void test_idealTrackingCount()
{
    std::string configPath = "analysis_multiPlane/tests/integration/test_fullDigitalProcessing/config.cfg";
    std::stringstream buffer;
    std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());
    setenv("ANALYSIS_CONFIG", configPath.c_str(), 1);
    DigitalProcessing_multiPlane(0, 0, "Test", false);
    auto config = GetDigitalConfig();
    Tracking_multiPlane(0, 0, "Test");
    auto matchedHits = GetMatchedHits(config, 0, 0, "Test", 0);
    auto procHits = GetTrackHits(config, 0, 0, "Test", 0);
    std::cout.rdbuf(old);

    assert(matchedHits.size() == procHits.size());
}

void test_residualSigmaIncreaseMergingWindow()
{
    std::string configPath = "analysis_multiPlane/tests/integration/test_fullDigitalProcessing/config.cfg";
    auto resSigma0 = residualSigma(configPath);
    configPath = "analysis_multiPlane/tests/integration/test_fullDigitalProcessing/config_wS1.6.cfg";
    auto resSigma16 = residualSigma(configPath);
    configPath = "analysis_multiPlane/tests/integration/test_fullDigitalProcessing/config_wS3.cfg";
    auto resSigma3 = residualSigma(configPath);

    assert(resSigma16.first > resSigma0.first);
    assert(resSigma16.second > resSigma0.second);
}

void test_idealSentinelCount()
{
    std::string configPath = "analysis_multiPlane/tests/integration/test_fullDigitalProcessing/config.cfg";
    std::stringstream buffer;
    std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());
    setenv("ANALYSIS_CONFIG", configPath.c_str(), 1);
    auto config = GetDigitalConfig();
    DigitalProcessing_multiPlane(0, 0, "Test", false);
    Tracking_multiPlane(0, 0, "Test");
    std::cout.rdbuf(old);
    auto matchedHits = GetMatchedHits(config, 0, 0, "Test", 0);

    for (const auto& hit : matchedHits)
    {
        assert(hit.dutTime != -1);
    }
}

void test_sentinelCountAtMaximumDistanceCut()
{
    std::string configPath = "analysis_multiPlane/tests/integration/test_fullDigitalProcessing/config_dCutMax.cfg";
    std::stringstream buffer;
    std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());
    setenv("ANALYSIS_CONFIG", configPath.c_str(), 1);
    auto config = GetDigitalConfig();
    DigitalProcessing_multiPlane(0, 0, "Test", false);
    Tracking_multiPlane(0, 0, "Test");
    auto matchedHits = GetMatchedHits(config, 0, 0, "Test", 0);
    std::cout.rdbuf(old);

    for (const auto& hit : matchedHits)
    {
        assert(hit.dutTime != -1);
    }

}



int main()
{
    gErrorIgnoreLevel = kError;

    std::cout << "Running ideal residual test...\n";
    test_idealResidual();
    std::cout << "Running ideal tracking count test... \n";
    test_idealTrackingCount();
    std::cout << "Running residual sigma increase with merging window test ...\n";
    test_residualSigmaIncreaseMergingWindow();
    std::cout << "Running ideal sentinel count test ...\n";
    test_idealSentinelCount();
    std::cout << "Running 0 sentinel count at maximum distance cut test ...\n";
    test_sentinelCountAtMaximumDistanceCut();

    std::cout << "\nAll tests passed.\n";

    return 0;
}
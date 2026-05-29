#include "RootIO.hh"
#include "DigitalUtils.hh"
#include "Utils.hh"
#include "CalorimetryUtils.hh"
#include "Debug_multiPlane.hh"
#include <cassert>
#include <iostream>
#include <cmath>
#include <sstream>
#include "TError.h"


std::string configPath = "analysis_multiPlane/tests/integration/test_fullDigitalProcessing/config.cfg";

int getHitCount(double threshold)
{
    setenv("ANALYSIS_CONFIG", configPath.c_str(), 1);
    std::stringstream buffer;
    std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());
    DigitalProcessing_multiPlane(threshold, 0, "Test", false);
    auto cfg = GetDigitalConfig();
    auto procHits = GetTrackHits(cfg, threshold, 0, "Test", 0);
    std::cout.rdbuf(old);
    return procHits.size();
}
void test_digitalCompile()
{
    setenv("ANALYSIS_CONFIG", configPath.c_str(), 1);
    std::stringstream buffer;
    std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());
    int status = DigitalProcessing_multiPlane(0, 0, "Test", false);
    std::cout.rdbuf(old);

    assert(status == 0);
}
void test_idealWordCount()
{
    // Test that when there are no restrictive bounds done on the GEANT4 output, the digital output has the same size as the GEANT4 input
    setenv("ANALYSIS_CONFIG", configPath.c_str(), 1);
    std::stringstream buffer;
    std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());
    DigitalProcessing_multiPlane(0, 0, "Test", false);
    auto cfg = GetDigitalConfig();
    auto multiPlanes = CaloPreProcessing(0, 0, "Test", cfg);
    auto rawHits = GetRawHits(multiPlanes[0]);
    auto procHits = GetTrackHits(cfg, 0, 0, "Test", 0);
    std::cout.rdbuf(old);

    assert(rawHits.size() == procHits.size());
}
void test_thresholdCountDependence()
{
    const std::vector<double> thresholds = {0,100,200,300,400,500,600,700,800,1000};
    auto previousCount = getHitCount(thresholds.front());

    for (size_t i = 1; i < thresholds.size(); ++i)
    {
        auto currentCount = getHitCount(thresholds[i]);

        assert(currentCount <= previousCount);

        previousCount = currentCount;
    }
}
void test_mergingWindowCountDependence()
{
    auto currentCount_wS0 = getHitCount(0);
    configPath = "analysis_multiPlane/tests/integration/test_fullDigitalProcessing/config_wS1.6.cfg";
    auto currentCount_wS16 = getHitCount(0);
    configPath = "analysis_multiPlane/tests/integration/test_fullDigitalProcessing/config_wS3.cfg";
    auto currentCount_wS3 = getHitCount(0);

    assert (currentCount_wS0 <= currentCount_wS16 <= currentCount_wS3);
}
void test_idealPixelPositionRetention()
{
    configPath = "analysis_multiPlane/tests/integration/test_fullDigitalProcessing/config.cfg";
    setenv("ANALYSIS_CONFIG", configPath.c_str(), 1);
    std::stringstream buffer;
    std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());
    DigitalProcessing_multiPlane(0, 0, "Test", false);
    auto cfg = GetDigitalConfig();
    auto multiPlanes = CaloPreProcessing(0, 0, "Test", cfg);
    auto rawHits = GetRawHits(multiPlanes[0]);
    auto procHits = GetTrackHits(cfg, 0, 0, "Test", 0);
    std::cout.rdbuf(old);

    std::sort(rawHits.begin(), rawHits.end(),[](const RawHit& a, const RawHit& b) {return a.time < b.time;});
    std::sort(procHits.begin(), procHits.end(),[](const ProcessedHit& a, const ProcessedHit& b) {return a.time < b.time;});

    assert(rawHits.size() == procHits.size());

    for (size_t i = 0; i < rawHits.size(); ++i)
    {
        assert(std::abs(rawHits[i].key.x - procHits[i].x) < 2);
        assert(std::abs(rawHits[i].key.y - procHits[i].y) < 2);
    }
}

int main()
{
    gErrorIgnoreLevel = kError;

    std::cout << "Running digital compile test...\n";
    test_digitalCompile();
    std::cout << "Running ideal word count test...\n";
    test_idealWordCount();    
    std::cout << "Running hit count dependence on threshold test...\n";
    test_thresholdCountDependence();
    std::cout << "Running hit count dependence on merging window size test...\n";
    test_mergingWindowCountDependence();
    std::cout << "Runnig ideal pixel position retention test... \n";
    test_idealPixelPositionRetention();

    std::cout << "\nAll tests passed.\n";

    return 0;
}
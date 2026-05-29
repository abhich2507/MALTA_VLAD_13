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

double mean(const std::vector<double>& v)
{
    if (v.empty()) return 0.0;
    return std::accumulate(v.begin(), v.end(), 0.0) / v.size();
}
double getClSizeMean(double threshold)
{
    std::string configPath = "analysis_multiPlane/tests/integration/test_fullDigitalProcessing/config.cfg";
    setenv("ANALYSIS_CONFIG", configPath.c_str(), 1);
    std::stringstream buffer;
    std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());
    auto config = GetDigitalConfig();
    DigitalProcessing_multiPlane(threshold, 0, "Test", false);
    Tracking_multiPlane(threshold, 0, "Test");
    Clustering_multiPlane(threshold, 0, "Test");
    std::cout.rdbuf(old);
    std::vector<double> vclSize;
    auto analysisHits = GetAnalysisHits(config, threshold, 0, "Test", 0);
    for (const auto& hit: analysisHits)
    {
        vclSize.push_back(hit.clSize);
    }

    return mean(vclSize);
}

void test_idealClusterCountTrackCountConsistency()
{
    std::string configPath = "analysis_multiPlane/tests/integration/test_fullDigitalProcessing/config.cfg";
    std::stringstream buffer;
    std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());
    setenv("ANALYSIS_CONFIG", configPath.c_str(), 1);
    auto config = GetDigitalConfig();
    DigitalProcessing_multiPlane(0, 0, "Test", false);
    Tracking_multiPlane(0, 0, "Test");
    Clustering_multiPlane(0, 0, "Test");
    std::cout.rdbuf(old);

    auto analysisHits = GetAnalysisHits(config, 0, 0, "Test", 0);
    auto vertices = GetVertex(config, 0);
    auto procHits = GetTrackHits(config, 0, 0, "Test", 0);

    assert (analysisHits.size() == vertices.size());
}
void test_idealClusterCountsHitCountConsistency()
{
    std::string configPath = "analysis_multiPlane/tests/integration/test_fullDigitalProcessing/config.cfg";
    std::stringstream buffer;
    std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());
    setenv("ANALYSIS_CONFIG", configPath.c_str(), 1);
    auto config = GetDigitalConfig();
    DigitalProcessing_multiPlane(0, 0, "Test", false);
    Tracking_multiPlane(0, 0, "Test");
    Clustering_multiPlane(0, 0, "Test");
    std::cout.rdbuf(old);

    auto analysisHits = GetAnalysisHits(config, 0, 0, "Test", 0);
    auto vertices = GetVertex(config, 0);
    auto procHits = GetTrackHits(config, 0, 0, "Test", 0);

    std::vector<double> vclSize;
    for (const auto& hit: analysisHits)
    {
        vclSize.push_back(hit.clSize);
    }

    assert (procHits.size() == vertices.size() * mean(vclSize));
}
void test_decreasingClusterSizeThreshold()
{
    const std::vector<double> thresholds = {0,100,200,300,400,500,600,700,800,1000};
    auto previousClSize = getClSizeMean(thresholds.front());

    for (size_t i = 1; i < thresholds.size(); ++i)
    {
        auto currentClSize = getClSizeMean(thresholds[i]);

        assert(currentClSize <= previousClSize);

        previousClSize = currentClSize;
    }
}

int main()
{
    gErrorIgnoreLevel = kError;

    std::cout << "Running ideal cluster, track count consistency test ...\n";
    test_idealClusterCountTrackCountConsistency();
    std::cout << "Running ideal clustered hits count, hit count consistency test ...\n";
    test_idealClusterCountsHitCountConsistency();
    std::cout << "Running decreasing cluster size with increasing threshold test ...\n";
    test_decreasingClusterSizeThreshold();

    std::cout << "\nAll tests passed.\n";

    return 0;
}
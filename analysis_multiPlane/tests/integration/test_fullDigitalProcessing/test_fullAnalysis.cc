#include "RootIO.hh"
#include "Utils.hh"
#include "CalorimetryUtils.hh"
#include "AnalysisUtils.hh"
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

void test_idealEfficiency()
{
    std::string configPath = "analysis_multiPlane/tests/integration/test_fullDigitalProcessing/config.cfg";
    setenv("ANALYSIS_CONFIG", configPath.c_str(), 1);
    std::stringstream buffer;
    std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());
    auto config = GetDigitalConfig();
    DigitalProcessing_multiPlane(0, 0, "Test", false);
    Tracking_multiPlane(0, 0, "Test");
    Clustering_multiPlane(0, 0, "Test");
    Analysis_multiPlane(0, 0, "Test");
    std::cout.rdbuf(old);

    std::string summaryPath = "analysis_multiPlane/tests/integration/test_fullDigitalProcessing/GEANT4Input_0000/Plots/local_0000/Test/summary.root";
    TFile *summaryFile = TFile::Open(summaryPath.c_str(), "READ");
    TTree *summaryTree = (TTree*) summaryFile->Get("summaryTree");
    double efficiency;
    summaryTree->SetBranchAddress("efficiency", &efficiency);
    summaryTree->GetEntry(0);

    assert( efficiency == 100);

}
void test_idealClusterSize()
{
    std::string configPath = "analysis_multiPlane/tests/integration/test_fullDigitalProcessing/config.cfg";
    setenv("ANALYSIS_CONFIG", configPath.c_str(), 1);
    std::stringstream buffer;
    std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());
    std::system("rm -rf analysis_multiPlane/tests/integration/test_fullDigitalProcessing/GEANT4Input_0000/Plots");
    auto config = GetDigitalConfig();
    DigitalProcessing_multiPlane(0, 0, "Test", false);
    Tracking_multiPlane(0, 0, "Test");
    Clustering_multiPlane(0, 0, "Test");
    Analysis_multiPlane(0, 0, "Test");
    std::cout.rdbuf(old);

    std::string summaryPath = "analysis_multiPlane/tests/integration/test_fullDigitalProcessing/GEANT4Input_0000/Plots/local_0000/Test/summary.root";
    TFile *summaryFile = TFile::Open(summaryPath.c_str(), "READ");
    TTree *summaryTree = (TTree*) summaryFile->Get("summaryTree");
    double clSize;
    summaryTree->SetBranchAddress("clSize", &clSize);
    summaryTree->GetEntry(0);

    auto analysisHits = GetAnalysisHits(config, 0, 0, "Test", 0);
    std::vector<double> vclSize;
    for (const auto& hit: analysisHits)
    {
        vclSize.push_back(hit.clSize);
    }
    double idealClSize = mean(vclSize);


    assert( clSize == idealClSize);
}
void test_idealAnalysisCount()
{
    std::string configPath = "analysis_multiPlane/tests/integration/test_fullDigitalProcessing/config.cfg";
    setenv("ANALYSIS_CONFIG", configPath.c_str(), 1);
    std::stringstream buffer;
    std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());
    auto config = GetDigitalConfig();
    DigitalProcessing_multiPlane(0, 0, "Test", false);
    Tracking_multiPlane(0, 0, "Test");
    Clustering_multiPlane(0, 0, "Test");
    Analysis_multiPlane(0, 0, "Test");
    std::cout.rdbuf(old);
    std::string summaryPath = "analysis_multiPlane/tests/integration/test_fullDigitalProcessing/GEANT4Input_0000/Plots/local_0000/Test/histos.root";
    TFile *summaryFile = TFile::Open(summaryPath.c_str(), "READ");

    TDirectory *dir = (TDirectory*)summaryFile->Get("Thr0");
    TH2D *h2Eff = (TH2D*) dir->Get("h2PASSInPixelAux_planeZ0");
    auto vertices = GetVertex(config, 0);
    assert(h2Eff->GetEntries() == vertices.size());
}

int main()
{
    gErrorIgnoreLevel = kError;

    std::cout << "Running ideal Efficiency test ...\n";
    test_idealEfficiency();
    std::cout << "Running ideal Cluster Size test ...\n";
    test_idealClusterSize();
    std::cout << "Running ideal analysis count test ...\n";
    test_idealAnalysisCount();

    std::cout << "\nAll tests passed.\n";

    return 0;
}
#pragma once
#include <vector>
#include "Analysis_multiPlane.hh"
#include "ConfigAnalysis.hh"
#include "TH1.h"
#include "TH2.h"

double getEffErr(int Npassed, int Nall);
double getEff(int Npassed, int Nall);
std::vector<TH2D*> Create2DHistograms(AnaFlags cfg, int planeZ);
std::vector<TH1D*> Create1DHistograms(int planeZ);
TH2D* FillHistograms(std::vector<AnalysisHits> analysisHits, std::vector<TH2D*> histograms2D, std::vector<TH1D*> histograms1D, AnaFlags cfg, DetectorConfig detCfg, int planeZ);
AnalyzedHit GetStatistics(std::vector<TH2D*> histograms2D, TH2D* auxiliaryHisto);
void ScaleHistograms(std::vector<TH2D*> histograms2D, std::vector<TH1D*> histograms1D, TH2D* auxiliaryHisto);
void SetHistogramStyle(std::vector<TH2D*> histograms2D, std::vector<TH1D*> histograms1D, AnalyzedHit statistics);
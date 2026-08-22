#include "AnalysisUtils.hh"
#include "Utils.hh"
#include <cmath>
#include <vector>
#include <iostream>
#include "TH1.h"
#include "TH2.h"
#include "TRandom3.h"

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
         if (analysisHits[i].mcFlag != 0) continue;
         
        //analysisHits[i];
        double trackOffsetX = cfg.trackOffsetX + geoMaps[analysisHits[i].planeID].x *10;
        double trackOffsetY = cfg.trackOffsetY + geoMaps[analysisHits[i].planeID].y *10;

        histograms2D[kALL]->Fill(analysisHits[i].x, analysisHits[i].y, 1);
        histograms2D[kPASS]->Fill(analysisHits[i].x, analysisHits[i].y, analysisHits[i].clSize > 0 ? 1 : 0);
        histograms2D[kClSize]->Fill(analysisHits[i].x, analysisHits[i].y, analysisHits[i].clSize);
        histograms2D[kTiming]->Fill(analysisHits[i].x, analysisHits[i].y, analysisHits[i].timing);

        double foldedX, foldedY;
        double period_X = numPixelsX * pixelSizeX;
        double period_Y = numPixelsY * pixelSizeY;


        if(cfg.trkUnc == true)
        {
            foldedX = fmod(fmod(analysisHits[i].x + trackOffsetX + rng.Gaus(0., trackunc_X), period_X) + period_X, period_X) * 1000;
            foldedY = fmod(fmod(analysisHits[i].y + trackOffsetY + rng.Gaus(0., trackunc_Y), period_Y) + period_Y, period_Y) * 1000;
            //foldedX = fmod(analysisHits[i].x + trackOffsetX + rng.Gaus(0., trackunc_X), numPixelsX*pixelSizeX) * 1000;
            //foldedY = fmod(analysisHits[i].y + trackOffsetY + rng.Gaus(0., trackunc_Y), numPixelsY*pixelSizeY) * 1000;
        } 
        else
        { 
            foldedX = fmod(fmod(analysisHits[i].x + trackOffsetX, period_X) + period_X, period_X) * 1000;
            foldedY = fmod(fmod(analysisHits[i].y + trackOffsetY, period_Y) + period_Y, period_Y) * 1000;       
            //foldedX = fmod(analysisHits[i].x + trackOffsetX, numPixelsX*pixelSizeX) * 1000;
            //foldedY = fmod(analysisHits[i].y + trackOffsetY, numPixelsY*pixelSizeY) * 1000;
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
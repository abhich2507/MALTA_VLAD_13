// plotting_scripts/EffVsEnergy.C
// Plot efficiency vs signal energy with error bars, in the style of BkgEff.c.
//
// Scans every Results_* directory for pion runs in 1-5 GeV, reads:
//   - <dir>/bkg_eff.csv   : plane,bkgRateMean,avgEff,effError
//                           -> per-plane signal efficiency at RATE (1.584)
//   - <dir>/coin_eff.csv  : run,window_ns,nGen,coinCount,eff_percent
//                           -> coincidence efficiency for the run at RATE
// and draws one graph per plane plus the coincidence efficiency.
//
// Usage (from malta_simulation/):
//   root -l -b -q plotting_scripts/EffVsEnergy.C
//   root -l -b -q 'plotting_scripts/EffVsEnergy.C("Plots")'

#include <fstream>
#include <sstream>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <regex>
#include <cmath>
#include <algorithm>

#include "TGraphErrors.h"
#include "TCanvas.h"
#include "TAxis.h"
#include "TMarker.h"
#include "TLine.h"
#include "TStyle.h"
#include "TString.h"
#include "TROOT.h"
#include "TSystem.h"
#include "TLatex.h"

static std::string Trim(const std::string& s)
{
    size_t b = s.find_first_not_of(" \t\r\n");
    if (b == std::string::npos) return "";
    size_t e = s.find_last_not_of(" \t\r\n");
    return s.substr(b, e - b + 1);
}

struct LegendEntry
{
    bool isLine;
    int markerStyle;
    int markerColor;
    double markerSize;
    int lineStyle;
    int lineColor;
    int lineWidth;
    std::string label;
};

// Draw a borderless legend using TMarker/TLine symbols + TLatex text.
static void DrawLatexLegend(const std::vector<LegendEntry>& entries,
                            double x0, double y0, double yStep, double textSize)
{
    for (size_t i = 0; i < entries.size(); ++i)
    {
        const LegendEntry& e = entries[i];
        double y = y0 - i * yStep;

        if (e.isLine)
        {
            TLine *ln = new TLine(x0, y, x0 + 0.05, y);
            ln->SetLineColor(e.lineColor);
            ln->SetLineStyle(e.lineStyle);
            ln->SetLineWidth(e.lineWidth);
            ln->SetNDC(true);
            ln->Draw();
        }
        else if (e.markerStyle > 0)
        {
            TMarker *mk = new TMarker(x0 + 0.025, y, 1);
            mk->SetNDC(true);
            mk->SetMarkerStyle(e.markerStyle);
            mk->SetMarkerColor(e.markerColor);
            mk->SetMarkerSize(e.markerSize);
            mk->Draw();
        }

        TLatex *txt = new TLatex(x0 + 0.06, y, e.label.c_str());
        txt->SetNDC();
        txt->SetTextFont(42);
        txt->SetTextSize(textSize);
        txt->SetTextAlign(12);  // left, vertically centered
        txt->Draw();
    }
}

struct EffPoint { double energy, eff, err; };

void EffVsEnergy(const char* outDir = "./")
{
    gStyle->SetOptStat(0);
    gROOT->SetStyle("ATLAS");

    const double rate = 1.584;   // background rate (particles/event) used to select rows

    // run -> rate map from configs/bkg_rate.csv, find the run at `rate`
    int targetRun = -1;
    {
        std::ifstream rc("configs/bkg_rate.csv");
        if (rc)
        {
            std::string line;
            std::getline(rc, line);  // skip header
            while (std::getline(rc, line))
            {
                if (line.empty()) continue;
                std::stringstream ss(line);
                int run; double r; char comma;
                ss >> run >> comma >> r;
                if (std::fabs(r - rate) < 1e-9) { targetRun = run; break; }
            }
        }
    }

    // ---- scan Results_* directories ----------------------------------------
    std::vector<EffPoint> p0, p1, coin;
    double coinWindowNs = 0.0;

    TString list = gSystem->GetFromPipe("ls -d Results_* 2>/dev/null");
    std::stringstream ls(list.Data());
    std::string dirName;
    while (std::getline(ls, dirName))
    {
        dirName = Trim(dirName);
        if (dirName.empty()) continue;

        // Results_<particle><energy>GeV  (particle must start with "pi")
        std::smatch m;
        std::regex re("^Results_(pi[a-z]*)([0-9]+(?:\\.[0-9]+)?)GeV$");
        if (!std::regex_match(dirName, m, re)) continue;

        double energy = std::stod(m[2].str());
        if (energy < 1.0 || energy > 5.0) continue;

        // per-plane efficiency from bkg_eff.csv
        std::map<int, std::pair<double,double>> planeEff;  // plane -> (eff, err)
        {
            std::ifstream in((dirName + "/bkg_eff.csv").c_str());
            if (in)
            {
                std::string line;
                std::getline(in, line);  // skip header
                while (std::getline(in, line))
                {
                    if (line.empty()) continue;
                    std::stringstream ss(line);
                    std::string cell;
                    std::vector<std::string> cols;
                    while (std::getline(ss, cell, ',')) cols.push_back(cell);
                    if (cols.size() < 4) continue;
                    if (std::fabs(std::stod(cols[1]) - rate) < 1e-9)
                        planeEff[std::stoi(cols[0])] = {std::stod(cols[2]), std::stod(cols[3])};
                }
            }
        }

        // coincidence efficiency from coin_eff.csv
        double coinEff = -1., coinErr = 0.;
        {
            std::ifstream in((dirName + "/coin_eff.csv").c_str());
            if (in)
            {
                std::string line;
                std::getline(in, line);  // skip header
                while (std::getline(in, line))
                {
                    if (line.empty()) continue;
                    std::stringstream ss(line);
                    std::string cell;
                    std::vector<std::string> cols;
                    while (std::getline(ss, cell, ',')) cols.push_back(cell);
                    if (cols.size() < 5) continue;
                    if (std::stoi(cols[0]) == targetRun)
                    {
                        coinWindowNs = std::stod(cols[1]);
                        coinEff = std::stod(cols[4]);
                        double nGen = std::stod(cols[2]);
                        coinErr = (nGen > 0) ? 100.0 * std::sqrt((coinEff / 100.0) * (1.0 - coinEff / 100.0) / nGen) : 0.0;
                        break;
                    }
                }
            }
        }

        std::cout << dirName << "  E=" << energy << " GeV"
                  << "  p0=" << (planeEff.count(0) ? planeEff[0].first : -1.)
                  << "  p1=" << (planeEff.count(1) ? planeEff[1].first : -1.)
                  << "  coin=" << coinEff << std::endl;

        if (planeEff.count(0)) p0.push_back({energy, planeEff[0].first, planeEff[0].second});
        if (planeEff.count(1)) p1.push_back({energy, planeEff[1].first, planeEff[1].second});
        if (coinEff >= 0.)     coin.push_back({energy, coinEff, coinErr});
    }

    auto byEnergy = [](const EffPoint& a, const EffPoint& b){ return a.energy < b.energy; };
    std::sort(p0.begin(), p0.end(), byEnergy);
    std::sort(p1.begin(), p1.end(), byEnergy);
    std::sort(coin.begin(), coin.end(), byEnergy);

    if (p0.empty() && p1.empty() && coin.empty())
    {
        std::cerr << "No data found in Results_* directories (pi 1-5 GeV)" << std::endl;
        return;
    }

    // ---- build graphs ------------------------------------------------------
    auto makeGraph = [](const std::vector<EffPoint>& v)
    {
        TGraphErrors* g = new TGraphErrors(v.size());
        for (size_t i = 0; i < v.size(); ++i)
        {
            g->SetPoint(i, v[i].energy, v[i].eff);
            g->SetPointError(i, 0., v[i].err);
        }
        return g;
    };

    TGraphErrors* g0 = makeGraph(p0);
    TGraphErrors* g1 = makeGraph(p1);
    TGraphErrors* gc = makeGraph(coin);

    // ---- draw --------------------------------------------------------------
    TCanvas* c = new TCanvas("c", "eff vs energy", 800, 600);
    c->SetLeftMargin(0.15);
    c->SetBottomMargin(0.15);
    c->SetTopMargin(0.17);

    double yMin = 90., yMax = 99.;

    if (p0.empty()) { std::cerr << "No plane-0 data" << std::endl; return; }

    g0->SetTitle("Hit Efficiency vs Signal Energy;Signal Momentum [GeV/c];Signal efficiency [%]");
    g0->SetMarkerStyle(45);
    g0->SetMarkerSize(1.6);
    g0->SetMarkerColor(kBlue-4);
    g0->SetLineColor(kBlue-4);
    g0->SetLineWidth(10);
    g0->Draw("AP");
    g0->GetXaxis()->SetRangeUser(0.5, 5.5);
    g0->GetYaxis()->SetRangeUser(yMin - 1.5, yMax + 1.5);
    g0->GetYaxis()->SetTitleOffset(1.0);
    g0->GetXaxis()->SetTitleOffset(1.0);

    g1->SetMarkerStyle(25);
    g1->SetMarkerSize(1.6);
    g1->SetMarkerColor(kRed-4);
    g1->SetLineColor(kRed-4);
    g1->SetLineWidth(10);
    g1->Draw("P SAME");

    std::vector<LegendEntry> legendEntries;
    legendEntries.push_back({false, 45, kBlue-4, 1.5, 1, kBlue-4, 2, "plane 0 (z=0 mm)"});
    legendEntries.push_back({false, 25, kRed-4, 1.5, 1, kRed-4, 2, "plane 1 (z=100 mm)"});

    if (!coin.empty())
    {
        gc->SetMarkerStyle(30);
        gc->SetMarkerSize(1.7);
        gc->SetMarkerColor(kBlack);
        gc->SetLineColor(kBlack);
        gc->SetLineWidth(10);
        gc->Draw("P SAME");
        legendEntries.push_back({false, 30, kBlack, 1.6, 1, kBlack, 2,
                                 Form("coincidence (%.0f ns window)", coinWindowNs)});
    }

    //legendEntries.push_back({false, 0, kBlack, 0, 1, kBlack, 1,
                            // Form("#lambda = %.3g particles/event", rate)});

    c->Modified();
    c->Update();

    DrawLatexLegend(legendEntries, 0.37, 0.34, 0.035, 0.04);

    // Beam label on top of the canvas.
    TLatex* titleLatex = new TLatex();
    titleLatex->SetNDC();
    titleLatex->SetTextFont(42);
    titleLatex->SetTextSize(0.05);
    titleLatex->SetTextAlign(22);
    titleLatex->DrawLatex(0.5, 0.90, "#bf{#pi^{+}} beam, 1-5 GeV/c #lambda = 1.584 particles/event");

    gSystem->mkdir(outDir, kTRUE);
    std::string outPng = TString::Format("%s/eff_vs_energy.png", outDir).Data();
    std::string outPdf = TString::Format("%s/eff_vs_energy.pdf", outDir).Data();
    c->Modified();
    c->Update();
    c->SaveAs(outPng.c_str());
    c->SaveAs(outPdf.c_str());
    std::cout << "Saved " << outPng << " and " << outPdf << std::endl;
}

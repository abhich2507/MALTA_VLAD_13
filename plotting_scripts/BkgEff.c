// plotting_scripts/BkgEff.c
// Plot efficiency vs background rate with error bars from Results/bkg_eff.csv
// CSV columns: plane, bkgRateMean, avgEff, effError  (one graph per plane)
// Usage (from malta_simulation/):
//   root -l -b -q plotting_scripts/BkgEff.c           # plane efficiencies only
//   root -l -b -q 'plotting_scripts/BkgEff.c(1)'      # also overlay coincidence efficiency
// Replot a renamed results directory (e.g. Results_p120GeV_eEIC_mp_coin):
//   root -l -b -q 'plotting_scripts/BkgEff.c(1, "Results_p120GeV_eEIC_mp_coin", "Plots_p120GeV_eEIC_mp_coin")'
// Coincidence data: <resDir>/coin_eff.csv (run, window_ns, nGen, coinCount, eff_percent)
// mapped to background rates via configs/bkg_rate.csv.

#include <fstream>
#include <sstream>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include "TGraphErrors.h"
#include "TCanvas.h"
#include "TAxis.h"
#include "TLine.h"
#include "TMarker.h"
#include "TMath.h"
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

// Read particleType and particleEnergy from <resDir>/local_0001/flags.cfg and return
// a ROOT display string for the canvas title, e.g. "#pi^{+} 1 GeV".
static std::string GetSignalTitle(const char* resDir)
{
    std::string particle, energy;
    std::ifstream in(TString::Format("%s/local_0001/flags.cfg", resDir).Data());
    std::string line;
    while (std::getline(in, line))
    {
        if (particle.empty() && line.compare(0, 12, "particleType") == 0)
        {
            size_t eq = line.find('=');
            if (eq != std::string::npos) particle = Trim(line.substr(eq + 1));
        }
        else if (energy.empty() && line.compare(0, 14, "particleEnergy") == 0)
        {
            size_t eq = line.find('=');
            if (eq != std::string::npos) energy = Trim(line.substr(eq + 1));
        }
        if (!particle.empty() && !energy.empty()) break;
    }

    std::string name;
    if (particle == "pi+") name = "#pi^{+}";
    else if (particle == "pi-") name = "#pi^{-}";
    else if (particle == "proton") name = "p";
    else name = particle.empty() ? "signal" : particle;

    std::string out = name;
    if (!energy.empty()) out += " " + energy + " GeV";
    return out;
}

// Map planeZ index -> z position [cm] from the geometry file referenced in the run flags.
static std::map<int, double> GetPlaneZPositions(const char* resDir)
{
    std::map<int, double> pos;

    std::string geoFile;
    std::ifstream flags(TString::Format("%s/local_0001/flags.cfg", resDir).Data());
    std::string line;
    while (std::getline(flags, line))
    {
        if (line.compare(0, 7, "geoFile") == 0)
        {
            size_t eq = line.find('=');
            if (eq != std::string::npos) geoFile = Trim(line.substr(eq + 1));
            break;
        }
    }
    if (geoFile.empty()) return pos;

    std::ifstream geo(TString::Format("configs/geometry/%s", geoFile.c_str()).Data());
    if (!geo) return pos;

    std::getline(geo, line);  // skip header
    while (std::getline(geo, line))
    {
        if (line.empty()) continue;
        std::stringstream ss(line);
        std::string cell;
        std::vector<std::string> cols;
        while (std::getline(ss, cell, ',')) cols.push_back(Trim(cell));
        if (cols.size() < 6) continue;
        int x = std::stoi(cols[0]);
        int y = std::stoi(cols[1]);
        int z = std::stoi(cols[2]);
        double zoff = std::stod(cols[5]);
        if (x == 0 && y == 0) pos[z] = zoff;
    }
    return pos;
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
        else
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

void BkgEff(int withCoin = 0, const char* resDir = "Results", const char* outDir = "Plots")
{
    gStyle->SetOptStat(0);
    gROOT->SetStyle("ATLAS");

    std::string bkgCsv = TString::Format("%s/bkg_eff.csv", resDir).Data();
    std::ifstream in(bkgCsv.c_str());
    if (!in) { std::cerr << "Cannot open " << bkgCsv << std::endl; return; }

    std::map<int, std::vector<double>> mCount, mEff, mEffErr;
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
        int plane = std::stoi(cols[0]);
        mCount[plane].push_back(std::stod(cols[1]));
        mEff[plane].push_back(std::stod(cols[2]));
        mEffErr[plane].push_back(std::stod(cols[3]));
    }
    in.close();

    if (mEff.empty()) { std::cerr << "No data read from " << bkgCsv << std::endl; return; }

    // Global y range across all planes
    double yMin = 90., yMax = 99.;
    // for (std::map<int, std::vector<double>>::iterator it = mEff.begin(); it != mEff.end(); ++it)
    // {
    //     for (size_t i = 0; i < it->second.size(); ++i)
    //     {
    //         if (it->second[i] < yMin) yMin = it->second[i];
    //         if (it->second[i] > yMax) yMax = it->second[i];
    //     }
    // }

    // Optional: coincidence efficiency from Results/coin_eff.csv
    std::vector<double> coinX, coinY, coinYErr;
    double coinWindowNs = 0.0;
    if (withCoin)
    {
        // run -> bkgRateMean from configs/bkg_rate.csv
        std::map<int,double> runRate;
        std::ifstream rc("configs/bkg_rate.csv");
        if (rc)
        {
            std::string rl;
            std::getline(rc, rl);  // skip header
            while (std::getline(rc, rl))
            {
                if (rl.empty()) continue;
                std::stringstream ss(rl);
                int run; double rate; char comma;
                ss >> run >> comma >> rate;
                runRate[run] = rate;
            }
        }

        std::string coinCsv = TString::Format("%s/coin_eff.csv", resDir).Data();
        std::ifstream coin(coinCsv.c_str());
        if (coin)
        {
            std::string cl;
            std::getline(coin, cl);  // skip header
            while (std::getline(coin, cl))
            {
                if (cl.empty()) continue;
                std::stringstream ss(cl);
                std::string cell;
                std::vector<std::string> cols;
                while (std::getline(ss, cell, ',')) cols.push_back(cell);
                if (cols.size() < 5) continue;
                int run = std::stoi(cols[0]);
                coinWindowNs = std::stod(cols[1]);
                double nGen = std::stod(cols[2]);
                double eff  = std::stod(cols[4]);
                if (!runRate.count(run))
                {
                    std::cerr << "BkgEff: run " << run << " not in configs/bkg_rate.csv, skipping" << std::endl;
                    continue;
                }
                double x   = runRate[run];  // background rate, consistent with plane graphs
                double err = (nGen > 0) ? 100.0 * std::sqrt((eff/100.0)*(1.0-eff/100.0)/nGen) : 0.0;
                coinX.push_back(x);
                coinY.push_back(eff);
                coinYErr.push_back(err);
                if (eff - err < yMin) yMin = eff - err;
                if (eff + err > yMax) yMax = eff + err;
            }
            coin.close();
        }
        else
        {
            std::cerr << "Cannot open " << coinCsv << std::endl;
        }
    }

    TCanvas *c = new TCanvas("c", "bkg efficiency", 800, 600);
    c->SetLeftMargin(0.15);
    c->SetBottomMargin(0.15);
    c->SetTopMargin(0.17);

    std::vector<LegendEntry> legendEntries;
    int colors[6] = {kBlue-4, kRed-4, kGreen-4, kMagenta-4, kOrange-4, kCyan-4};
    int styles[6] = {45, 25, 26, 27, 24, 30};  // hollow markers: circle, square, triangle, diamond, cross, star
    int colIdx = 0;
    bool first = true;

    std::string signalTitle = GetSignalTitle(resDir);
    std::map<int, double> planeZcm = GetPlaneZPositions(resDir);

    for (std::map<int, std::vector<double>>::iterator it = mEff.begin(); it != mEff.end(); ++it)
    {
        int plane = it->first;
        std::vector<double>& eff = it->second;
        TGraphErrors *gr = new TGraphErrors(eff.size(), mCount[plane].data(), eff.data(), nullptr, mEffErr[plane].data());
        gr->SetTitle("Hit Efficiency vs Background Rate;Background rate #lambda [particles/event];Signal-only efficiency [%]");
        gr->SetMarkerStyle(styles[colIdx % 6]);
        gr->SetMarkerSize(1.6);
        gr->SetMarkerColor(colors[colIdx % 6]);
        gr->SetLineColor(colors[colIdx % 6]);
        gr->SetLineWidth(10);
        if (first)
        {
            gr->Draw("AP");
            gr->GetYaxis()->SetRangeUser(yMin - 1.5, yMax + 1.5);
            gr->GetYaxis()->SetTitleOffset(1.0);  // smaller = y-axis title closer to the axis
            gr->GetXaxis()->SetTitleOffset(1.0);
            first = false;
        }
        else
        {
            gr->Draw("P SAME");
        }
        std::string label = planeZcm.count(plane) ? Form("planeZ%d (z = %.0f cm)", plane, planeZcm[plane])
                                                  : Form("planeZ%d", plane);
        legendEntries.push_back({false, styles[colIdx % 6], colors[colIdx % 6], 1.5, 1, colors[colIdx % 6], 2, label});
        ++colIdx;
    }

    if (withCoin && !coinX.empty())
    {
        TGraphErrors *grCoin = new TGraphErrors(coinX.size(), coinX.data(), coinY.data(), nullptr, coinYErr.data());
        grCoin->SetMarkerStyle(30);
        grCoin->SetMarkerSize(1.7);
        grCoin->SetMarkerColor(kBlack);
        grCoin->SetLineColor(kBlack);
        grCoin->SetLineWidth(10);
        grCoin->Draw("P SAME");
        legendEntries.push_back({false, 30, kBlack, 1.6, 1, kBlack, 2, Form("coincidence (%.0f ns window)", coinWindowNs)});
    }

    // Paint the pad first so gPad->GetUxmin()/GetUymax() return user coordinates.
    c->Modified();
    c->Update();

    // Vertical line marking the original EIC background rate
    // (0.198 hits/sensor/event x 8 sensors = 1.584 particles/event).
    const double eicRate = 1.584;
    TLine *eicLine = new TLine(eicRate, yMin - 1.5, eicRate, yMax + 1.5);
    eicLine->SetLineColor(kRed);
    eicLine->SetLineStyle(kDashed);
    eicLine->SetLineWidth(2);
    eicLine->Draw();

    legendEntries.push_back({true, 1, kRed, 0, kDashed, kRed, 2, "Original EIC bkg rate (#lambda= 1.584)"});

    DrawLatexLegend(legendEntries, 0.37, 0.34, 0.035, 0.03);

    // Signal particle + energy as an explicit label on top of the canvas.
    TLatex *titleLatex = new TLatex();
    titleLatex->SetNDC();
    titleLatex->SetTextFont(42);
    titleLatex->SetTextSize(0.05);
    titleLatex->SetTextAlign(22);
    titleLatex->DrawLatex(0.5, 0.90, Form("#bf{Signal: %s}", signalTitle.c_str()));

    gSystem->mkdir(outDir, kTRUE);
    std::string outPng = TString::Format("%s/bkg_eff.png", outDir).Data();
    std::string outPdf = TString::Format("%s/bkg_eff.pdf", outDir).Data();
    c->Modified();
    c->Update();
    c->SaveAs(outPng.c_str());
    c->SaveAs(outPdf.c_str());
    std::cout << "Saved " << outPng << " and " << outPdf << std::endl;
}

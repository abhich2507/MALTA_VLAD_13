// plotting_scripts/BkgEff.c
// Plot efficiency vs background count with error bars from configs/bkg_eff.csv
// CSV columns: plane, count-1, avgEff, effError  (one graph per plane)
// Usage (from malta_simulation/):
//   root -l -b -q plotting_scripts/BkgEff.c           # plane efficiencies only
//   root -l -b -q 'plotting_scripts/BkgEff.c(1)'      # also overlay coincidence efficiency
// Coincidence data: Results/coin_eff.csv (run, window_ns, nGen, coinCount, eff_percent)
// mapped to background counts via configs/bkg_count.csv.

#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <map>
#include "TGraphErrors.h"
#include "TCanvas.h"
#include "TLegend.h"
#include "TAxis.h"
#include "TMath.h"
#include "TStyle.h"
#include "TROOT.h"

void BkgEff(int withCoin = 0)
{
    gStyle->SetOptStat(0);
    gROOT->SetStyle("ATLAS");

    std::ifstream in("configs/bkg_eff.csv");
    if (!in) { std::cerr << "Cannot open configs/bkg_eff.csv" << std::endl; return; }

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

    if (mEff.empty()) { std::cerr << "No data read from configs/bkg_eff.csv" << std::endl; return; }

    // Global y range across all planes
    double yMin = 70., yMax = 99.;
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
        // run -> particleCount from configs/bkg_count.csv
        std::map<int,int> runCount;
        std::ifstream rc("configs/bkg_count.csv");
        if (rc)
        {
            std::string rl;
            std::getline(rc, rl);  // skip header
            while (std::getline(rc, rl))
            {
                if (rl.empty()) continue;
                std::stringstream ss(rl);
                int run, count; char comma;
                ss >> run >> comma >> count;
                runCount[run] = count;
            }
        }

        std::ifstream coin("Results/coin_eff.csv");
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
                if (!runCount.count(run))
                {
                    std::cerr << "BkgEff: run " << run << " not in configs/bkg_count.csv, skipping" << std::endl;
                    continue;
                }
                double x   = runCount[run] - 1.0;  // background count, consistent with plane graphs
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
            std::cerr << "Cannot open Results/coin_eff.csv" << std::endl;
        }
    }

    TCanvas *c = new TCanvas("c", "bkg efficiency", 800, 600);
    c->SetLeftMargin(0.15);
    c->SetBottomMargin(0.15);

    TLegend *leg = new TLegend(0.55, 0.75, 0.88, 0.88);
    int colors[6] = {kBlue+2, kRed+2, kGreen+2, kMagenta+2, kOrange+7, kCyan+2};
    int colIdx = 0;
    bool first = true;

    for (std::map<int, std::vector<double>>::iterator it = mEff.begin(); it != mEff.end(); ++it)
    {
        int plane = it->first;
        std::vector<double>& eff = it->second;
        TGraphErrors *gr = new TGraphErrors(eff.size(), mCount[plane].data(), eff.data(), nullptr, mEffErr[plane].data());
        gr->SetTitle("Hit Efficiency vs Background Count;Background(e-) count;Signal-only efficiency [%]");
        gr->SetMarkerStyle(20 + (colIdx % 4));
        gr->SetMarkerSize(0.9);
        gr->SetMarkerColor(colors[colIdx % 6]);
        gr->SetLineColor(colors[colIdx % 6]);
        gr->SetLineWidth(2);
        if (first)
        {
            gr->Draw("AP");
            gr->GetYaxis()->SetRangeUser(yMin - 1.5, yMax + 1.5);
            first = false;
        }
        else
        {
            gr->Draw("P SAME");
        }
        leg->AddEntry(gr, Form("planeZ%d", plane), "p");
        ++colIdx;
    }

    if (withCoin && !coinX.empty())
    {
        TGraphErrors *grCoin = new TGraphErrors(coinX.size(), coinX.data(), coinY.data(), nullptr, coinYErr.data());
        grCoin->SetMarkerStyle(23);
        grCoin->SetMarkerSize(1.1);
        grCoin->SetMarkerColor(kBlack);
        grCoin->SetLineColor(kBlack);
        grCoin->SetLineWidth(2);
        grCoin->Draw("P SAME");
        leg->AddEntry(grCoin, Form("coincidence (%.0f ns window)", coinWindowNs), "p");
    }

    leg->Draw();

    c->SaveAs("Plots/bkg_eff.png");
    c->SaveAs("Plots/bkg_eff.pdf");
    std::cout << "Saved Plots/bkg_eff.png and Plots/bkg_eff.pdf" << std::endl;
}

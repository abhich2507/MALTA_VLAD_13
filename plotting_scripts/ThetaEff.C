// plotting_scripts/ThetaEff.c
// Plot per-plane signal efficiency vs incidence angle theta_deg from <resDir>/theta_eff.csv.
// Optional: overlay two-plane coincidence efficiency from <resDir>/coin_eff.csv
// (run numbers mapped back to theta via the theta-scan CSV).
// Usage:
//   root -l -b -q 'plotting_scripts/ThetaEff.c(1, "Results_thetaX", "Plots_thetaX", "configs/theta_scan_X.csv")'
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <vector>
#include "TGraphErrors.h"
#include "TCanvas.h"
#include "TStyle.h"
#include "TROOT.h"
#include "TString.h"
#include "TSystem.h"
#include "TLegend.h"

void ThetaEff(int withCoin = 1, const char* resDir = "Results_thetaX",
              const char* outDir = "Plots_thetaX", const char* csvPath = "configs/theta_scan_X.csv")
{
    gStyle->SetOptStat(0);
    gROOT->SetStyle("ATLAS");

    std::map<int, std::vector<double>> x, y, ey;
    {
        std::ifstream in(TString::Format("%s/theta_eff.csv", resDir).Data());
        if (!in) { std::cerr << "Cannot open theta_eff.csv" << std::endl; return; }
        std::string line; std::getline(in, line);
        while (std::getline(in, line)) {
            if (line.empty()) continue;
            std::stringstream ss(line); std::string cell; std::vector<std::string> c;
            while (std::getline(ss, cell, ',')) c.push_back(cell);
            if (c.size() < 4) continue;
            int p = std::stoi(c[0]);
            x[p].push_back(std::stod(c[1]));
            y[p].push_back(std::stod(c[2]));
            ey[p].push_back(std::stod(c[3]));
        }
    }
    if (y.empty()) { std::cerr << "no data in theta_eff.csv" << std::endl; return; }

    // run -> theta map for the coincidence overlay
    std::map<int,double> runTheta;
    {
        std::ifstream rc(csvPath);
        std::string line; std::getline(rc, line);
        while (std::getline(rc, line)) {
            if (line.empty()) continue;
            std::stringstream ss(line);
            int run; double th; char comma;
            ss >> run >> comma >> th;
            runTheta[run] = th;
        }
    }

    TGraphErrors *gCoin = nullptr;
    if (withCoin) {
        std::vector<double> cx, cy, ce;
        std::ifstream coin(TString::Format("%s/coin_eff.csv", resDir).Data());
        if (coin) {
            std::string line; std::getline(coin, line);
            while (std::getline(coin, line)) {
                if (line.empty()) continue;
                std::stringstream ss(line); std::string cell; std::vector<std::string> c;
                while (std::getline(ss, cell, ',')) c.push_back(cell);
                if (c.size() < 5) continue;
                int run = std::stoi(c[0]);
                if (!runTheta.count(run)) continue;
                double nGen = std::stod(c[2]);
                double eff  = std::stod(c[4]);
                double err  = (nGen > 0) ? 100.0 * std::sqrt((eff/100.0)*(1.0-eff/100.0)/nGen) : 0.0;
                cx.push_back(runTheta[run]); cy.push_back(eff); ce.push_back(err);
            }
            if (!cx.empty()) gCoin = new TGraphErrors(cx.size(), cx.data(), cy.data(), nullptr, ce.data());
        }
    }

    gSystem->mkdir(outDir, kTRUE);
    TCanvas *c = new TCanvas("c", "theta efficiency", 800, 600);
    c->SetLeftMargin(0.14); c->SetBottomMargin(0.13);

    int colors[4] = {kBlue-4, kRed-4, kGreen-4, kMagenta-4};
    int styles[4] = {45, 25, 26, 27};
    TLegend *leg = new TLegend(0.20, 0.16, 0.48, 0.40);
    bool first = true; int i = 0;
    for (auto &kv : y) {
        TGraphErrors *g = new TGraphErrors(kv.second.size(), x[kv.first].data(),
                                           kv.second.data(), nullptr, ey[kv.first].data());
        g->SetMarkerStyle(styles[i % 4]);
        g->SetMarkerColor(colors[i % 4]);
        g->SetLineColor(colors[i % 4]);
        g->SetMarkerSize(1.5);
        g->SetTitle("Hit efficiency vs incidence angle;#theta_{x} [deg];Signal Efficiency [%]");
        g->GetYaxis()->SetRangeUser(90.0, 100.0);
        if (first) { g->Draw("AP"); first = false; }
        else g->Draw("P SAME");
        leg->AddEntry(g, Form("plane %d", kv.first), "p");
        i++;
    }
    if (gCoin) {
        gCoin->SetMarkerStyle(45); gCoin->SetMarkerColor(kBlack);
        gCoin->SetMarkerSize(1.6); gCoin->SetLineColor(kBlack);
        gCoin->Draw("P SAME");
        leg->AddEntry(gCoin, "two-plane coincidence", "p");
    }
    leg->Draw();
    gPad->Update();
    c->SaveAs(TString::Format("%s/theta_eff.png", outDir).Data());
    c->SaveAs(TString::Format("%s/theta_eff.pdf", outDir).Data());
    std::cout << "saved theta_eff.png/pdf in " << outDir << std::endl;
}
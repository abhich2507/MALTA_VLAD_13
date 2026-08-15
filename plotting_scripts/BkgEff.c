// plotting_scripts/BkgEff.c
// Plot efficiency vs background count with error bars from configs/bkg_eff.csv
// CSV columns: count-1, avgEff, effError
// Usage (from malta_simulation/): root -l -b -q plotting_scripts/BkgEff.c

#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include "TGraphErrors.h"
#include "TCanvas.h"
#include "TAxis.h"
#include "TMath.h"
#include "TStyle.h"
#include "TROOT.h"

void BkgEff()
{
    gStyle->SetOptStat(0);
    gROOT->SetStyle("ATLAS");

    std::ifstream in("configs/bkg_eff.csv");
    if (!in) { std::cerr << "Cannot open configs/bkg_eff.csv" << std::endl; return; }

    std::vector<double> vCount, vEff, vEffErr;
    std::string line;
    std::getline(in, line);  // skip header
    while (std::getline(in, line))
    {
        if (line.empty()) continue;
        std::stringstream ss(line);
        std::string cell;
        std::vector<std::string> cols;
        while (std::getline(ss, cell, ',')) cols.push_back(cell);
        if (cols.size() < 3) continue;
        vCount.push_back(std::stod(cols[0]));
        vEff.push_back(std::stod(cols[1]));
        vEffErr.push_back(std::stod(cols[2]));
    }
    in.close();

    auto *gr = new TGraphErrors(vCount.size(), vCount.data(), vEff.data(), nullptr, vEffErr.data());
    gr->SetTitle("");
    gr->SetMarkerStyle(20);
    gr->SetMarkerSize(0.9);
    gr->SetMarkerColor(kBlue + 2);
    gr->SetLineColor(kBlue + 2);

    TCanvas *c = new TCanvas("c", "bkg efficiency", 800, 600);
    c->SetLeftMargin(0.15);
    c->SetBottomMargin(0.13);
    gr->Draw("AP");
    gr->GetXaxis()->SetTitle("Background count (count-1)");
    gr->GetYaxis()->SetTitle("Efficiency [%]");
    gr->GetYaxis()->SetRangeUser(TMath::MinElement(vEff.size(), vEff.data()) - 1.5,
                                 TMath::MaxElement(vEff.size(), vEff.data()) + 1.5);

    c->SaveAs("Plots/bkg_eff.png");
    c->SaveAs("Plots/bkg_eff.pdf");
    std::cout << "Saved Plots/bkg_eff.png and Plots/bkg_eff.pdf" << std::endl;
}

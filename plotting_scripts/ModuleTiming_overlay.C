// plotting_scripts/ModuleTiming_overlay.C
// Overlay the 4 sensor (quad-module) timing distributions of plane 0 in ONE
// canvas, each taken from its own perpendicular-beam run (configs/module_scan.csv).
//   run i (CSV row order) -> sensor i (S0..S3) -> planeID == i in TrackedHits_planeZ0
// Each histogram is normalized, its peak is located automatically, and a
// Gaussian is fitted around it so the per-sensor time resolution (sigma) is read
// off directly.
//
// Usage:
//   root -l -b -q 'plotting_scripts/ModuleTiming_overlay.C'                                  # header defaults
//   root -l -b -q 'plotting_scripts/ModuleTiming_overlay.C("Results_module","analysis_results_224x512_module",100,"Plots_module","configs/module_scan.csv")'

#include <iostream>
#include "TFile.h"
#include "TTree.h"
#include "TH1D.h"
#include "TCanvas.h"
#include "TF1.h"
#include "TLegend.h"
#include "TLatex.h"
#include "TStyle.h"
#include "TString.h"
#include "TSystem.h"
#include "TROOT.h"
#include <fstream>
#include <sstream>
#include <vector>
#include <string>

// ==================== USER CONFIG (edit here) ====================
const char* RES_DIR   = "Results_module";
const char* SAVE      = "analysis_results_224x512_module";
const int   THRESHOLD = 100;
const char* OUT_DIR   = "Plots_module";
const char* CSV_PATH  = "configs/module_scan.csv";

const double T_MIN = 0.0,  T_MAX = 60.0;   // timing histogram range [ns]
const int    NBINS = 120;
// fit half-width around the located peak [ns]
const double FIT_HALF = 8.0;
// ================================================================

void ModuleTiming_overlay(const char* resDir = "", const char* save = "",
                          int thr = -1, const char* outDir = "",
                          const char* csvPath = "")
{
    if (resDir[0] == '\0')  resDir  = RES_DIR;
    if (save[0]   == '\0')  save    = SAVE;
    if (thr    <  0)        thr     = THRESHOLD;
    if (outDir[0] == '\0')  outDir  = OUT_DIR;
    if (csvPath[0] == '\0') csvPath = CSV_PATH;

    gStyle->SetOptStat(0);
    gROOT->cd();

    // Read run list from CSV (run i -> sensor i)
    std::vector<int> runs;
    std::vector<std::string> sensors;
    {
        std::ifstream in(csvPath);
        if (!in) { std::cerr << "Cannot open " << csvPath << std::endl; return; }
        std::string line; std::getline(in, line);  // header
        while (std::getline(in, line)) {
            if (line.empty()) continue;
            std::stringstream ss(line);
            std::string cell; std::vector<std::string> c;
            while (std::getline(ss, cell, ',')) c.push_back(cell);
            if (c.size() < 2) continue;
            runs.push_back(std::stoi(c[0]));
            sensors.push_back(c[1]);
        }
    }
    if (runs.size() > 4) runs.resize(4), sensors.resize(4);
    if (runs.empty()) { std::cerr << "No runs in " << csvPath << std::endl; return; }

    int colors[4]  = {kBlue, kRed, kGreen+1, kMagenta};
    int markers[4] = {20, 21, 22, 23};

    TH1D *h[4] = {nullptr, nullptr, nullptr, nullptr};

    for (size_t i = 0; i < runs.size(); i++) {
        int run = runs[i];
        TString live = TString::Format("Results/local_%04d/%s/LocalTrackedHitsThr%d.root", run, save, thr);
        TString arch = TString::Format("%s/local_%04d/%s/LocalTrackedHitsThr%d.root", resDir, run, save, thr);
        TString path = (!gSystem->AccessPathName(live.Data())) ? live : arch;

        TFile *f = TFile::Open(path.Data(), "READ");
        if (!f || f->IsZombie()) { std::cerr << "Cannot open " << path << " (skip " << sensors[i] << ")" << std::endl; continue; }
        TTree *t = (TTree*) f->Get("TrackedHits_planeZ0");
        if (!t) { std::cerr << "No TrackedHits_planeZ0 in " << path << std::endl; f->Close(); continue; }

        gROOT->cd();
        double timing; int planeID;
        t->SetBranchAddress("DUTLocalTime", &timing);
        t->SetBranchAddress("planeID",      &planeID);

        h[i] = new TH1D(Form("h%s", sensors[i].c_str()), Form("%s;t [ns];a.u.", sensors[i].c_str()), NBINS, T_MIN, T_MAX);
        Long64_t n = t->GetEntries();
        for (Long64_t e = 0; e < n; e++) {
            t->GetEntry(e);
            if (planeID == (int)i && timing >= T_MIN && timing <= T_MAX) h[i]->Fill(timing);
        }
        f->Close();
        std::cout << sensors[i] << " (run " << run << "): in-range entries = " << h[i]->GetEntries() << std::endl;
    }

    TCanvas *c = new TCanvas("c", "Plane0 module timing (4 sensors)", 1200, 800);
    TLegend *leg = new TLegend(0.58, 0.42, 0.88, 0.88);
    leg->SetBorderSize(0);
    leg->SetFillStyle(0);
    leg->SetTextSize(0.032);

    bool first = true;
    for (int i = 0; i < 4; i++) {
        if (!h[i] || h[i]->Integral() <= 0) { std::cout << sensors[i] << " empty, skipping\n"; continue; }

        h[i]->Scale(1.0 / h[i]->Integral());
        h[i]->SetMarkerStyle(markers[i]);
        h[i]->SetMarkerColor(colors[i]);
        h[i]->SetMarkerSize(1.1);
        h[i]->SetLineColor(colors[i]);
        h[i]->SetLineWidth(3);
        h[i]->SetFillStyle(0);

        if (first) { h[i]->Draw("P"); first = false; }
        else        h[i]->Draw("P SAME");

        // locate peak = max bin in range, then fit +/- FIT_HALF around it
        int peakBin = h[i]->GetMaximumBin();
        double peak  = h[i]->GetBinCenter(peakBin);

        TF1 *g = new TF1(Form("g%d", i), "gaus", peak - FIT_HALF, peak + FIT_HALF);
        g->SetLineColor(colors[i]);
        g->SetLineWidth(3);
        h[i]->Fit(g, "QR");
        g->Draw("SAME");

        double mu  = g->GetParameter(1);
        double sig = g->GetParameter(2);
        leg->AddEntry(h[i], Form("#bf{%s,  peak = %.1f ns}", sensors[i].c_str(), mu), "pl");
        leg->AddEntry((TObject*)0, Form("#bf{#sigma = %.2f ns}", sig), "");
    }
    leg->Draw();

    TLatex ttl;
    ttl.SetNDC();
    ttl.SetTextSize(0.045);
    ttl.DrawLatex(0.26, 0.93, "MALTA2 224#times512  plane-0  quad-module timing");

    c->Update();
    gSystem->mkdir(outDir, kTRUE);
    c->SaveAs(Form("%s/ModuleTiming_all4.png", outDir));
    c->SaveAs(Form("%s/ModuleTiming_all4.pdf", outDir));
    std::cout << "Saved " << outDir << "/ModuleTiming_all4.png/pdf" << std::endl;
}

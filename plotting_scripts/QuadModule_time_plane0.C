// plotting_scripts/ModuleTiming_plane0.C
// Extract per-sensor (quad-module) timing for plane 0 from
// <resDir>/local_NNNN/<save>/LocalTrackedHitsThr<THR>.root
// and overlay the 4 sensors (S0..S3) in one canvas, each Gaussian-fitted.
//
// Usage:
//   root -l -b -q 'plotting_scripts/ModuleTiming_plane0.C'   // uses the header config below
//   root -l -b -q 'plotting_scripts/ModuleTiming_plane0.C(402, "Results_thetaX", "analysis_results_224x512_thetaX", 100, "Plots_thetaX")'

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

// ==================== USER CONFIG (edit here) ====================
const int    RUN       = 401;
const char*  RES_DIR   = "Results_thetaX";
const char*  SAVE      = "analysis_results_224x512_thetaX";
const int    THRESHOLD = 100;
const char*  OUT_DIR   = "Plots_thetaX";

// timing histogram range [ns] and binning
const double T_MIN = 0.0,  T_MAX = 60.0;
const int    NBINS = 120;

// Gaussian fit window per sensor S0..S3: {peak guess [ns], half-width [ns]}
// The 4 modules are staggered by 8 ns (ComputeModuleDelay), so guesses are
// roughly  S0~38, S1~30, S2~22, S3~14 ns. Adjust to your data.
const double PEAK[4]  = {38.0, 30.0, 22.0, 14.0};
const double WIDTH[4] = { 3.0,  3.0,  3.0,  3.0};
// ================================================================

void QuadModule_time_plane0(int run = 0, const char* resDir = "", const char* save = "",
                         int thr = -1, const char* outDir = "")
{
    if (run    <= 0)        run    = RUN;
    if (resDir[0] == '\0')  resDir = RES_DIR;
    if (save[0]   == '\0')  save   = SAVE;
    if (thr    <  0)        thr    = THRESHOLD;
    if (outDir[0] == '\0')  outDir = OUT_DIR;

    gStyle->SetOptStat(0);

    // prefer the live Results/local_NNNN location, else the archived resDir
    TString live = TString::Format("Results/local_%04d/%s/LocalTrackedHitsThr%d.root", run, save, thr);
    TString arch = TString::Format("%s/local_%04d/%s/LocalTrackedHitsThr%d.root", resDir, run, save, thr);
    TString path = (!gSystem->AccessPathName(live.Data())) ? live : arch;
    std::cout << "Opening: " << path << std::endl;

    TFile *f = TFile::Open(path.Data(), "READ");
    if (!f || f->IsZombie()) { std::cerr << "Cannot open " << path << std::endl; return; }
    TTree *t = (TTree*) f->Get("TrackedHits_planeZ0");
    if (!t) { std::cerr << "No tree TrackedHits_planeZ0 in " << path << std::endl; f->Close(); return; }

    // Create histograms in memory, NOT inside the open TFile — otherwise
    // f->Close() below deletes them and the next access segfaults.
    gROOT->cd();

    double timing;
    int    planeID;
    t->SetBranchAddress("DUTLocalTime", &timing);
    t->SetBranchAddress("planeID",      &planeID);

    TH1D *h[4];
    for (int i = 0; i < 4; i++)
        h[i] = new TH1D(Form("hS%d", i), Form("S%d;t [ns];a.u.", i), NBINS, T_MIN, T_MAX);

    Long64_t n = t->GetEntries();
    for (Long64_t i = 0; i < n; i++) {
        t->GetEntry(i);
        if (planeID >= 0 && planeID < 4) h[planeID]->Fill(timing);   // planeID 0..3 == S0..S3
    }
    f->Close();

    for (int i = 0; i < 4; i++)
        if (h[i]->Integral() > 0) h[i]->Scale(1.0 / h[i]->Integral());

    int colors[4]  = {kBlue, kRed, kGreen+1, kMagenta};
    int markers[4] = {20, 21, 22, 23};

    TCanvas *c = new TCanvas("c", "Plane0 module timing", 1200, 800);
    for (int i = 0; i < 4; i++) {
        h[i]->SetMarkerStyle(markers[i]);
        h[i]->SetMarkerColor(colors[i]);
        h[i]->SetMarkerSize(1.2);
        h[i]->SetLineColor(colors[i]);
        h[i]->SetLineWidth(4);
        h[i]->SetFillStyle(0);
    }
    h[0]->GetXaxis()->SetTitle("Hit time (local) [ns]");
    h[0]->GetYaxis()->SetTitle("Normalized entries [a.u.]");
    h[0]->GetXaxis()->SetTitleSize(0.045);
    h[0]->GetYaxis()->SetTitleSize(0.045);
    h[0]->GetXaxis()->SetLabelSize(0.04);
    h[0]->GetYaxis()->SetLabelSize(0.04);

    TLegend *leg = new TLegend(0.58, 0.40, 0.88, 0.88);
    leg->SetBorderSize(0);
    leg->SetFillStyle(0);
    leg->SetTextSize(0.032);

    bool first = true;
    for (int i = 0; i < 4; i++) {
        if (h[i]->Integral() <= 0) { std::cout << "S" << i << " empty, skipping\n"; continue; }

        if (first) { h[i]->Draw("P"); first = false; }
        else        h[i]->Draw("P SAME");

        TF1 *g = new TF1(Form("g%d", i), "gaus", PEAK[i] - 1.6*WIDTH[i], PEAK[i] + WIDTH[i]);
        g->SetLineColor(colors[i]);
        g->SetLineWidth(3);
        h[i]->Fit(g, "QR");

        double mu  = g->GetParameter(1);
        double sig = g->GetParameter(2);
        g->Draw("SAME");

        leg->AddEntry(h[i], Form("#bf{S%d,  peak = %.1f ns}", i, mu), "pl");
        leg->AddEntry((TObject*)0, Form("#bf{#sigma = %.2f ns}", sig), "");
    }
    leg->Draw();

    TLatex ttl;
    ttl.SetNDC();
    ttl.SetTextSize(0.045);
    ttl.DrawLatex(0.28, 0.93, Form("MALTA2 224#times512  plane-0  run %d", run));

    c->Update();
    gSystem->mkdir(outDir, kTRUE);
    c->SaveAs(Form("%s/ModuleTiming_plane0_run%d.pdf", outDir, run));
    c->SaveAs(Form("%s/ModuleTiming_plane0_run%d.png", outDir, run));
    std::cout << "Saved " << outDir << "/ModuleTiming_plane0_run" << run << ".pdf/.png" << std::endl;
}
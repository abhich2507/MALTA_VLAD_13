// plotting_scripts/ModuleTiming_plane0.C
// Extract per-sensor (quad-module) timing for plane 0 from
// <resDir>/local_NNNN/<save>/LocalTrackedHitsThr<THR>.root
// and overlay the 4 sensors (S0..S3) in one canvas, each Gaussian-fitted.
//
// Usage:
//   root -l -b -q 'plotting_scripts/ModuleTiming_plane0.C'
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
#include <iomanip>

// ==================== USER CONFIG (edit here) ====================
const int    RUN       = 504;
const char*  RES_DIR   = "Results_module";
const char*  SAVE      = "analysis_results_224x512_module";
const int    THRESHOLD = 100;
const char*  OUT_DIR   = "Plots_module";

// timing histogram range [ns] and binning
const double T_MIN = 0.0,  T_MAX = 120.0;
const int    NBINS = 1200;

// Custom Gaussian fit windows per sensor S0..S3: [min, max]
const double FIT_MIN[4] = {37.650, 29.50, 21.5,  13.55};
const double FIT_MAX[4] = {40.35, 32.750, 24.4, 16.35};
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
    std::cout << "\nOpening: " << path << std::endl;

    TFile *f = TFile::Open(path.Data(), "READ");
    if (!f || f->IsZombie()) { std::cerr << "Cannot open " << path << std::endl; return; }
    TTree *t = (TTree*) f->Get("TrackedHits_planeZ0");
    if (!t) { std::cerr << "No tree TrackedHits_planeZ0 in " << path << std::endl; f->Close(); return; }

    // Create histograms in memory, NOT inside the open TFile
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
        if (planeID >= 0 && planeID < 4) h[planeID]->Fill(timing); 
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

        // Fit curve using the custom min and max bounds for this sensor
        TF1 *g = new TF1(Form("g%d", i), "gaus", FIT_MIN[i], FIT_MAX[i]);
        g->SetLineColor(colors[i]);
        g->SetLineWidth(3);
        
        // "Q" suppresses standard ROOT fit output, "R" uses the specified range
        h[i]->Fit(g, "QR");

        double mu      = g->GetParameter(1);
        double mu_err  = g->GetParError(1);
        double sig     = g->GetParameter(2);
        double sig_err = g->GetParError(2);
        double chi2    = g->GetChisquare();
        int    ndf     = g->GetNDF();
        
        g->Draw("SAME");

        // Print custom fit results to terminal
        std::cout << "--- Sensor S" << i << " Fit Results ---" << std::endl;
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "  Chi2 / NDF : " << chi2 << " / " << ndf 
                  << " = " << (ndf > 0 ? chi2/ndf : 0.0) << std::endl;
        std::cout << std::setprecision(3);
        std::cout << "  Mean       : " << mu  << " +/- " << mu_err  << " ns" << std::endl;
        std::cout << "  Sigma      : " << sig << " +/- " << sig_err << " ns" << std::endl;
        std::cout << "---------------------------------\n" << std::endl;

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
// plotting_scripts/CorrectedTiming_plane0.C
// Per-sensor ROW-CORRECTED timing from the analysis output, in the same style
// as QuadModule_time_plane0.C: overlay S0..S3 of plane 0, each Gaussian-fitted.
//
// Reads:
//     <resDir>/local_<RUN>/<save>/analysisThr<THR>.root
//     tree: analyzedHits_planeZ0   (branches: correctedTiming, planeID)
//
// correctedTiming = timing - pixY * 0.0125  (row-geometry delay removed).
//
// Usage (from malta_simulation_224x512/):
//   root -l -b -q 'plotting_scripts/CorrectedTiming_plane0.C'
//   root -l -b -q 'plotting_scripts/CorrectedTiming_plane0.C(501, "Results_module_widegaus_thr200", "analysis_results_224x512_module", 200, "Plots_module_widegaus_thr200")'

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
const int    RUN       = 501;
const char*  RES_DIR   = "Results_module_widegaus_thr200";
const char*  SAVE      = "analysis_results_224x512_module";
const int    THRESHOLD = 200;
const char*  OUT_DIR   = "Plots_module_widegaus_thr200";

// timing histogram range [ns] and binning
const double T_MIN = 0.0,  T_MAX = 60.0;
const int    NBINS = 600;    // 0.1 ns/bin

// Custom Gaussian fit windows per sensor S0..S3: [min, max]
// corrected peaks are ~1.4 ns earlier than the DUTLocalTime peaks
// (row delay of the central rows is removed).
const double FIT_MIN[4] = {36.0, 29.0, 21.0, 13.0};
const double FIT_MAX[4] = {43.0, 35.0, 27.0, 19.0};
// ================================================================

void CorrectedTiming_plane0(int run = 0, const char* resDir = "",
                            const char* save = "", int thr = -1,
                            const char* outDir = "")
{
    if (run    <= 0)        run    = RUN;
    if (resDir[0] == '\0')  resDir = RES_DIR;
    if (save[0]   == '\0')  save   = SAVE;
    if (thr    <  0)        thr    = THRESHOLD;
    if (outDir[0] == '\0')  outDir = OUT_DIR;

    gStyle->SetOptStat(0);

    TString path = TString::Format("%s/local_%04d/%s/analysisThr%d.root",
                                   resDir, run, save, thr);
    std::cout << "\nOpening: " << path << std::endl;

    TFile *f = TFile::Open(path.Data(), "READ");
    if (!f || f->IsZombie()) { std::cerr << "Cannot open " << path << std::endl; return; }
    TTree *t = (TTree*) f->Get("analyzedHits_planeZ0");
    if (!t) { std::cerr << "No tree analyzedHits_planeZ0 in " << path << std::endl; f->Close(); return; }

    // Create histograms in memory, NOT inside the open TFile
    gROOT->cd();

    double correctedTiming;
    int    planeID;
    t->SetBranchAddress("correctedTiming", &correctedTiming);
    t->SetBranchAddress("planeID",         &planeID);

    TH1D *h[4];
    for (int i = 0; i < 4; i++)
        h[i] = new TH1D(Form("hS%d", i), Form("S%d;t [ns];a.u.", i), NBINS, T_MIN, T_MAX);

    Long64_t n = t->GetEntries();
    for (Long64_t i = 0; i < n; i++) {
        t->GetEntry(i);
        if (correctedTiming <= 0) continue;             // skip unmatched sentinels
        if (planeID >= 0 && planeID < 4) h[planeID]->Fill(correctedTiming);
    }
    f->Close();

    for (int i = 0; i < 4; i++)
        if (h[i]->Integral() > 0) h[i]->Scale(1.0 / h[i]->Integral());

    int colors[4]  = {kBlue, kRed, kGreen+1, kMagenta};
    int markers[4] = {20, 21, 22, 23};

    TCanvas *c = new TCanvas("c", "Plane0 corrected timing", 1200, 800);
    for (int i = 0; i < 4; i++) {
        h[i]->SetMarkerStyle(markers[i]);
        h[i]->SetMarkerColor(colors[i]);
        h[i]->SetMarkerSize(1.2);
        h[i]->SetLineColor(colors[i]);
        h[i]->SetLineWidth(4);
        h[i]->SetFillStyle(0);
    }
    h[0]->GetXaxis()->SetTitle("Corrected hit time [ns]");
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
    ttl.DrawLatex(0.26, 0.93, Form("MALTA2 224#times512  plane-0  run %d  thr %d e^{-}", run, thr));

    c->Update();
    gSystem->mkdir(outDir, kTRUE);
    c->SaveAs(Form("%s/CorrectedTiming_plane0_run%d.pdf", outDir, run));
    c->SaveAs(Form("%s/CorrectedTiming_plane0_run%d.png", outDir, run));
    std::cout << "Saved " << outDir << "/CorrectedTiming_plane0_run" << run << ".pdf/.png" << std::endl;
}

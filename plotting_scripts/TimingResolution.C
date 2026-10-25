// plotting_scripts/TimingResolution.C
// Intrinsic time resolution of the SECOND sensor (planeID==1) of plane Z=0,
// signal only. Single Gaussian fit in a user-supplied range.
//
//   DUTLocalTime = reconstructed hit time - truth vertex time [ns]
//   read from LocalTrackedHitsThr100.root -> TrackedHits_planeZ0
//   selection: mcFlag==0 && planeID==1 && DUTPixX!=-1 && DUTnHits==1 (first hit)
//
// Usage (from malta_simulation/):
//   root -l -b -q 'plotting_scripts/TimingResolution.C(1,"Results_p120GeV_eEIC_mp_coin_test")'
// Fit range: edit fitLo / fitHi below.

#include "TFile.h"
#include "TTree.h"
#include "TH1.h"
#include "TF1.h"
#include "TCanvas.h"
#include "TLatex.h"
#include "TStyle.h"
#include "TROOT.h"
#include "TSystem.h"
#include "TString.h"
#include <iostream>

void TimingResolution(int run = 1,
                      const char* resDir = "Results_p120GeV",
                      const char* outDir = "",
                      int threshold = 100)
{
    // Fit range [ns] — edit here//
    //run =1
    // const double fitLo = 30.25;
    // const double fitHi = 34.55;
    //run =2
    const double fitLo = 30.95;
    const double fitHi = 33.95;

    gROOT->SetStyle("ATLAS");
    gStyle->SetOptStat(0);

    TString od = outDir;
    if (od == "")
    {
        od = resDir;
        if (od.BeginsWith("Results"))
            od = TString("Plots") + od(7, od.Length() - 7);
    }
    gSystem->mkdir(od, kTRUE);
    TString runDir = Form("%s/local_%04d", od.Data(), run);
    gSystem->mkdir(runDir, kTRUE);

    TString fname = Form("%s/local_%04d/analysis_results_MP/LocalTrackedHitsThr%d.root",
                         resDir, run, threshold);
    TFile* f = TFile::Open(fname, "READ");
    if (!f || f->IsZombie()) { std::cerr << "Cannot open " << fname << std::endl; return; }

    TTree* t = (TTree*)f->Get("TrackedHits_planeZ0");

    int    mcFlag, planeID, pixX, DUTnHits;
    double tlocal;
    t->SetBranchAddress("mcFlag",       &mcFlag);
    t->SetBranchAddress("planeID",      &planeID);
    t->SetBranchAddress("DUTPixX",      &pixX);
    t->SetBranchAddress("DUTnHits",     &DUTnHits);
    t->SetBranchAddress("DUTLocalTime", &tlocal);

    TH1D* h = new TH1D("hT0", "Second sensor (planeID=1, Z=0), signal only;DUTLocalTime [ns];entries",
                       160, 20, 100);
    h->SetDirectory(0);   // detach so f->Close() does not delete it

    for (Long64_t i = 0; i < t->GetEntries(); ++i)
    {
        t->GetEntry(i);
        if (mcFlag != 0 || planeID != 1 || pixX == -1 || DUTnHits != 1) continue;
        h->Fill(tlocal);
    }
    f->Close();

    // single Gaussian fit, restricted to the requested range
    TF1* g = new TF1("g", "gaus", fitLo, fitHi);
    h->Fit(g, "RQ");

    double amp   = g->GetParameter(0);
    double mean  = g->GetParameter(1);
    double sigma = g->GetParameter(2);
    double meanErr = g->GetParError(1);
    double sigErr  = g->GetParError(2);
    double chi2    = g->GetChisquare();
    int    ndf     = g->GetNDF();
    double r2      = (ndf > 0) ? 1.0 - chi2 / h->GetEntries() : 0.0;   // rough R^2 proxy

    std::cout << "Second sensor (planeID=1, Z=0), signal only, fit [" << fitLo << ", " << fitHi << "] ns:" << std::endl;
    std::cout << "  N      = " << h->GetEntries() << std::endl;
    std::cout << "  mean   = " << mean << " +/- " << meanErr << " ns" << std::endl;
    std::cout << "  sigma  = " << sigma << " +/- " << sigErr << " ns" << std::endl;
    std::cout << "  chi2/ndf = " << chi2 << "/" << ndf << " = " << (ndf > 0 ? chi2/ndf : 0.0) << std::endl;
    std::cout << "  RMS(all) = " << h->GetRMS() << " ns" << std::endl;

    TCanvas* c = new TCanvas("cT0", "second sensor timing", 800, 600);
    h->Draw("E1");
    TF1* gd = new TF1("gd", "gaus", fitLo, fitHi);
    gd->SetParameters(amp, mean, sigma);
    gd->SetLineColor(kRed);
    gd->SetLineWidth(2);
    gd->Draw("same");

    TLatex l;
    l.SetNDC();
    l.SetTextSize(0.05);
    l.SetTextFont(42);
    l.DrawLatex(0.60, 0.82, Form("#sigma = %.2f #pm %.2f ns", sigma, sigErr));
    l.DrawLatex(0.60, 0.74, Form("mean = %.2f ns", mean));
    l.DrawLatex(0.60, 0.66, Form("#chi^{2}/ndf = %.2f / %d", chi2, ndf));
    l.DrawLatex(0.60, 0.58, Form("fit range = [%.1f, %.1f] ns", fitLo, fitHi));

    c->SaveAs(Form("%s/timing_secondSensor_Z0_run%04d.png", runDir.Data(), run));
    c->SaveAs(Form("%s/timing_secondSensor_Z0_run%04d.pdf", runDir.Data(), run));

    std::cout << "Saved " << runDir << "/timing_secondSensor_Z0_run" << Form("%04d", run) << ".png|pdf" << std::endl;
}
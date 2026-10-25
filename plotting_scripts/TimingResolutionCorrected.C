// plotting_scripts/TimingResolutionCorrected.C
// ---------------------------------------------------------------------------
// Step 2: intrinsic time resolution of the SECOND sensor (planeID==1) of
// plane Z=0, signal only, using the ROW-CORRECTED cluster timing.
//
//   correctedTiming = (min hit time in cluster) - firstPixY * 0.0125   [ns]
//   read from analysisThr100.root -> analyzedHits_planeZ0
//
// correctedTiming is already vertex-relative (it is built from DUTLocalTime
// in the clustering stage), and the -pixY*0.0125 term removes the row-dependent
// geometry delay (0.0125 ns/row). One row per truth track; the timing is the
// earliest hit of the cluster (leading edge).
//
// Selection: mcFlag==0 (signal) && planeID==1 (2nd sensor @ z=0) && clSize>0 (matched)
//
// Output: <outDir>/timingCorrected_secondSensor_Z0.png|pdf
//
// Usage (from malta_simulation/):
//   root -l -b -q 'plotting_scripts/TimingResolutionCorrected.C(1,"Results_p120GeV_eEIC_mp_coin_test")'
// ---------------------------------------------------------------------------

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

void TimingResolutionCorrected(int run = 1,
                               const char* resDir = "Results_p120GeV_eEIC_mp_coin_test",
                               const char* outDir = "",
                               int threshold = 100)
{
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

    TString fname = Form("%s/local_%04d/analysis_results_MP/analysisThr%d.root",
                         resDir, run, threshold);
    TFile* f = TFile::Open(fname, "READ");
    if (!f || f->IsZombie()) { std::cerr << "Cannot open " << fname << std::endl; return; }

    TTree* t = (TTree*)f->Get("analyzedHits_planeZ0");

    int    mcFlag, planeID, clSize;
    double correctedTiming;
    t->SetBranchAddress("mcFlag",          &mcFlag);
    t->SetBranchAddress("planeID",         &planeID);
    t->SetBranchAddress("clSize",          &clSize);
    t->SetBranchAddress("correctedTiming", &correctedTiming);

    TH1D* h = new TH1D("hT0c", "Second sensor (planeID=1, Z=0), signal only, corrected timing;correctedTiming [ns];entries",
                       200, 0, 100);

    for (Long64_t i = 0; i < t->GetEntries(); ++i)
    {
        t->GetEntry(i);
        if (mcFlag != 0 || planeID != 1 || clSize <= 0) continue;
        h->Fill(correctedTiming);
    }

    Long64_t n       = h->GetEntries();
    double mean      = h->GetMean();
    double meanErr   = h->GetMeanError();
    double rms       = h->GetRMS();

    // Gaussian core fit (two-pass)
    double sigma = 0., sigmaErr = 0., amp = 0.;
    if (n >= 20 && rms > 0.)
    {
        double lo = mean - 3.0 * rms, hi = mean + 3.0 * rms;
        if (hi <= lo) { lo = h->GetXaxis()->GetXmin(); hi = h->GetXaxis()->GetXmax(); }
        TF1* f1 = new TF1("f1", "gaus", lo, hi);
        h->Fit(f1, "RQN");
        double s0 = f1->GetParameter(2);
        if (s0 > 0.)
        {
            TF1* f2 = new TF1("f2", "gaus", mean - 2.5 * s0, mean + 2.5 * s0);
            f2->SetParameters(f1->GetParameter(0), f1->GetParameter(1), f1->GetParameter(2));
            h->Fit(f2, "RQ");
            amp      = f2->GetParameter(0);
            sigma    = f2->GetParameter(2);
            sigmaErr = f2->GetParError(2);
        }
    }

    std::cout << "Second sensor (planeID=1, Z=0), signal only, corrected timing:" << std::endl;
    std::cout << "  N       = " << n << std::endl;
    std::cout << "  mean    = " << mean << " +/- " << meanErr << " ns" << std::endl;
    std::cout << "  RMS     = " << rms << " ns" << std::endl;
    std::cout << "  sigma   = " << sigma << " +/- " << sigmaErr << " ns  (Gaussian core)" << std::endl;

    TCanvas* c = new TCanvas("cT0c", "second sensor corrected timing", 800, 600);
    h->Draw("E1");

    if (sigma > 0.)
    {
        TF1* gdraw = new TF1("gdraw", "gaus", mean - 3.5 * sigma, mean + 3.5 * sigma);
        gdraw->SetParameters(amp, mean, sigma);
        gdraw->SetLineColor(kRed);
        gdraw->SetLineWidth(2);
        gdraw->Draw("same");
    }

    TLatex l;
    l.SetNDC();
    l.SetTextSize(0.05);
    l.SetTextFont(42);
    l.DrawLatex(0.60, 0.82, Form("#sigma = %.2f #pm %.2f ns", sigma, sigmaErr));
    l.DrawLatex(0.60, 0.74, Form("RMS = %.2f ns", rms));
    l.DrawLatex(0.60, 0.66, Form("mean = %.2f ns", mean));

    c->SaveAs(Form("%s/timingCorrected_secondSensor_Z0.png", od.Data()));
    c->SaveAs(Form("%s/timingCorrected_secondSensor_Z0.pdf", od.Data()));

    f->Close();
    std::cout << "Saved " << od << "/timingCorrected_secondSensor_Z0.png|pdf" << std::endl;
}

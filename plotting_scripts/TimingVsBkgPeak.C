// plotting_scripts/TimingVsBkgPeak.C
// ---------------------------------------------------------------------------
// Time resolution vs background rate, fitting ONLY the main (core) peak.
//
// The timing distribution has a sharp core + a charge-dependent timewalk tail
// on the late side (verified in VerifyTimewalk.C). Fitting the full histogram
// with a Gaussian is meaningless; instead we fit a Gaussian restricted to the
// main peak and take ITS sigma as the intrinsic time resolution.
//
// Quantity: correctedTiming (row-corrected leading-edge time, one row per
// cluster) from analysisThr100.root -> analyzedHits_planeZ0.
// Selection: mcFlag==0 (signal) && planeID==1 (2nd sensor @ z=0) && clSize>0.
//
// Peak-fit procedure (automatic, per rate):
//   1. fine histogram (0.1 ns bins), find the mode (max bin) m
//   2. estimate sigma from the LEFT half-maximum (tail is on the right, so the
//      left side of the peak is clean):  sigma_est = (m - x_half) / 1.177
//   3. fit gaus in [m - 4*sigma_est, m + 2*sigma_est]  (asymmetric window
//      keeps the timewalk tail out of the fit)
//
// Outputs (<outDir>/):
//   timingVsBkg_peak_sigma.png|pdf   : main-peak sigma vs background rate
//   timingVsBkg_peak_mean.png|pdf    : main-peak mean vs background rate
//   timingVsBkg_peak_example.png|pdf : example fit for the EIC rate (run 4)
//   timing_vs_bkg_peak.csv
//
// Usage (from malta_simulation/):
//   root -l -b -q 'plotting_scripts/TimingVsBkgPeak.C("Results_p120GeV_eEIC_mp_coin_test")'
// ---------------------------------------------------------------------------

#include "TFile.h"
#include "TTree.h"
#include "TH1.h"
#include "TF1.h"
#include "TCanvas.h"
#include "TLatex.h"
#include "TGraphErrors.h"
#include "TLegend.h"
#include "TStyle.h"
#include "TROOT.h"
#include "TSystem.h"
#include "TString.h"
#include "TMath.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>

struct PeakFit
{
    double mode{0};
    double sigma{0};
    double sigmaErr{0};
    double mean{0};
    double meanErr{0};
    Long64_t n{0};
    double rmsFull{0};
    double meanFull{0};
    bool   ok{false};
};

PeakFit FitMainPeak(TH1D* h)
{
    PeakFit p;
    p.n        = h->GetEntries();
    p.meanFull = h->GetMean();
    p.rmsFull  = h->GetRMS();
    if (p.n < 50) return p;

    // mode
    int mx = h->GetMaximumBin();
    p.mode = h->GetBinCenter(mx);
    double hmax = h->GetBinContent(mx);

    // left half-maximum -> sigma estimate (right side is the timewalk tail)
    double xhalf = p.mode;
    for (int b = mx; b >= 1; --b)
    {
        if (h->GetBinContent(b) <= 0.5 * hmax) { xhalf = h->GetBinCenter(b); break; }
    }
    double sigEst = (p.mode - xhalf) / 1.177;   // HWHM / 1.177 = sigma
    if (sigEst <= 0.05) sigEst = 0.5;
    if (sigEst > 20.)  sigEst = 10.;

    double lo = p.mode - 4.0 * sigEst;
    double hi = p.mode + 2.0 * sigEst;          // asymmetric: keep tail out
    if (lo < h->GetXaxis()->GetXmin()) lo = h->GetXaxis()->GetXmin();
    if (hi > h->GetXaxis()->GetXmax()) hi = h->GetXaxis()->GetXmax();

    TF1* f1 = new TF1(Form("f1_%s", h->GetName()), "gaus", lo, hi);
    f1->SetParameters(hmax, p.mode, sigEst);
    h->Fit(f1, "RQN");

    double s0 = f1->GetParameter(2);
    if (s0 <= 0.) return p;
    if (s0 > 5.0 * sigEst) s0 = 2.0 * sigEst;   // sanity clamp

    TF1* f2 = new TF1(Form("f2_%s", h->GetName()), "gaus", lo, p.mode + 1.5 * s0);
    f2->SetParameters(f1->GetParameter(0), f1->GetParameter(1), f1->GetParameter(2));
    h->Fit(f2, "RQ");

    p.sigma    = f2->GetParameter(2);
    p.sigmaErr = f2->GetParError(2);
    p.mean     = f2->GetParameter(1);
    p.meanErr  = f2->GetParError(1);
    p.ok       = true;
    return p;
}

void TimingVsBkgPeak(const char* resDir = "Results_p120GeV_eEIC_mp_coin_test",
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

    // (run, rate)
    struct RP { int run; double rate; };
    std::vector<RP> pts;
    {
        std::ifstream in("configs/bkg_rate.csv");
        std::string line;
        std::getline(in, line);
        while (std::getline(in, line))
        {
            if (line.empty()) continue;
            std::stringstream ss(line);
            int run; double rate; char comma;
            ss >> run >> comma >> rate;
            pts.push_back({run, rate});
        }
    }
    if (pts.empty()) { std::cerr << "configs/bkg_rate.csv empty" << std::endl; return; }

    std::vector<double> rates, sigma, sigmaE, mean, meanE, rmsFull;
    std::ofstream csv(Form("%s/timing_vs_bkg_peak.csv", od.Data()));
    csv << "run,rate,n,mode_ns,sigma_ns,sigmaErr_ns,mean_ns,meanErr_ns,rms_full_ns,mean_full_ns\n";

    TH1D* example = nullptr;

    for (const auto& p : pts)
    {
        TFile* f = TFile::Open(Form("%s/local_%04d/analysis_results_MP/analysisThr%d.root",
                                    resDir, p.run, threshold), "READ");
        if (!f || f->IsZombie())
        {
            if (f) f->Close();
            std::cerr << "skip run " << p.run << std::endl;
            continue;
        }
        TTree* t = (TTree*)f->Get("analyzedHits_planeZ0");

        int    mcFlag, planeID, clSize;
        double correctedTiming;
        t->SetBranchAddress("mcFlag",          &mcFlag);
        t->SetBranchAddress("planeID",         &planeID);
        t->SetBranchAddress("clSize",          &clSize);
        t->SetBranchAddress("correctedTiming", &correctedTiming);

        TH1D* h = new TH1D(Form("hT_%d", p.run), "correctedTiming (second sensor Z0, signal);correctedTiming [ns];entries",
                           600, 0, 60);
        h->SetDirectory(0);
        for (Long64_t i = 0; i < t->GetEntries(); ++i)
        {
            t->GetEntry(i);
            if (mcFlag != 0 || planeID != 1 || clSize <= 0) continue;
            h->Fill(correctedTiming);
        }
        f->Close();

        PeakFit pf = FitMainPeak(h);

        rates.push_back(p.rate);
        sigma.push_back(pf.sigma);
        sigmaE.push_back(pf.sigmaErr);
        mean.push_back(pf.mean);
        meanE.push_back(pf.meanErr);
        rmsFull.push_back(pf.rmsFull);

        csv << p.run << "," << p.rate << "," << pf.n << "," << pf.mode << ","
            << pf.sigma << "," << pf.sigmaErr << "," << pf.mean << "," << pf.meanErr << ","
            << pf.rmsFull << "," << pf.meanFull << "\n";

        std::cout << Form("run %d  rate %.2f : peak sigma = %.3f +/- %.3f ns   (full RMS %.3f ns)",
                          p.run, p.rate, pf.sigma, pf.sigmaErr, pf.rmsFull) << std::endl;

        if (p.rate == 1.584 && example == nullptr) example = h;
    }
    csv.close();

    // --- sigma vs rate ---
    TGraphErrors* gs = new TGraphErrors(rates.size(), rates.data(), sigma.data(), nullptr, sigmaE.data());
    gs->SetTitle("Time resolution (main peak) vs background rate;Background rate #lambda [particles/event];#sigma_{peak} [ns]");
    gs->SetMarkerStyle(20);
    gs->SetMarkerColor(kRed);
    gs->SetLineColor(kRed);
    {
        TCanvas* c = new TCanvas("cSig", "peak sigma vs rate", 800, 600);
        double ymax = 0.; for (int i = 0; i < gs->GetN(); ++i) ymax = TMath::Max(ymax, gs->GetY()[i] + gs->GetErrorY(i));
        gs->GetYaxis()->SetRangeUser(0, ymax * 1.4);
        gs->Draw("AP");
        c->SaveAs(Form("%s/timingVsBkg_peak_sigma.png", od.Data()));
        c->SaveAs(Form("%s/timingVsBkg_peak_sigma.pdf", od.Data()));
    }
    // --- mean vs rate ---
    TGraphErrors* gm = new TGraphErrors(rates.size(), rates.data(), mean.data(), nullptr, meanE.data());
    gm->SetTitle("Main-peak mean vs background rate;Background rate #lambda [particles/event];peak mean [ns]");
    gm->SetMarkerStyle(21);
    gm->SetMarkerColor(kBlue);
    gm->SetLineColor(kBlue);
    {
        TCanvas* c = new TCanvas("cMean", "peak mean vs rate", 800, 600);
        double ymin = 1e9, ymax = -1e9;
        for (int i = 0; i < gm->GetN(); ++i) { ymin = TMath::Min(ymin, gm->GetY()[i]); ymax = TMath::Max(ymax, gm->GetY()[i]); }
        double pad = (ymax - ymin) * 0.3 + 0.5;
        gm->GetYaxis()->SetRangeUser(ymin - pad, ymax + pad);
        gm->Draw("AP");
        c->SaveAs(Form("%s/timingVsBkg_peak_mean.png", od.Data()));
        c->SaveAs(Form("%s/timingVsBkg_peak_mean.pdf", od.Data()));
    }

    // --- example fit at EIC rate (run with rate 1.584) ---
    if (example)
    {
        TCanvas* c = new TCanvas("cEx", "example main-peak fit", 900, 650);
        example->Draw("E1");
        PeakFit pf = FitMainPeak(example);
        TF1* gf = new TF1("gf", "gaus", pf.mode - 4.0 * pf.sigma, pf.mode + 2.0 * pf.sigma);
        gf->SetLineColor(kRed);
        gf->SetLineWidth(2);
        gf->Draw("same");
        TLatex l; l.SetNDC(); l.SetTextSize(0.05); l.SetTextFont(42);
        l.DrawLatex(0.60, 0.82, Form("#sigma_{peak} = %.2f #pm %.2f ns", pf.sigma, pf.sigmaErr));
        l.DrawLatex(0.60, 0.74, Form("full RMS = %.2f ns (tail incl.)", pf.rmsFull));
        l.DrawLatex(0.60, 0.66, "fit window = core only");
        c->SaveAs(Form("%s/timingVsBkg_peak_example.png", od.Data()));
        c->SaveAs(Form("%s/timingVsBkg_peak_example.pdf", od.Data()));
    }

    std::cout << "Wrote " << od << "/timing_vs_bkg_peak.csv and timingVsBkg_peak_*.png|pdf" << std::endl;
}

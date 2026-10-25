// plotting_scripts/VerifyTimewalk.C
// ---------------------------------------------------------------------------
// Definitive check that the timing tails are the charge-dependent timewalk.
//
// The digitizer builds (DigitalUtils.cc, CorrectAndSortTimeMap):
//   t_reco = rawHitTime + timewalk(C) + rowDelay(pixY) + moduleDelay + jitter
// with
//   timewalk(C) = T / pow((C*TrefThr/threshold) - x0, n)   (T=390, TrefThr=150,
//                 threshold=100, x0=149.8, n=0.65)
//   rowDelay    = pixY * 0.0125
//   moduleDelay = 24 ns for module 1 (second sensor)
//
// DUTLocalTime = t_reco - vertexTime, and rawHitTime - vertexTime = TOF.
// Therefore, after removing the measured TOF, row delay and module delay:
//   y = DUTLocalTime - (rawHitTime - vertexTime) - pixY*0.0125 - 24
//     = timewalk(C) + jitter
//
// If the scatter of (raw pixel charge C, y) follows the analytic timewalk
// curve (steep rise at low C), the tails are timewalk. One point per
// first-hit (DUTnHits==1) signal pixel on the second sensor (planeID==1, Z=0).
//
// Output: <outDir>/verify_timewalk.png|pdf
//
// Usage (from malta_simulation/):
//   root -l -b -q 'plotting_scripts/VerifyTimewalk.C(1,"Results_p120GeV_eEIC_mp_coin_test")'
// ---------------------------------------------------------------------------

#include "TFile.h"
#include "TTree.h"
#include "TChain.h"
#include "TH2.h"
#include "TF1.h"
#include "TCanvas.h"
#include "TLatex.h"
#include "TStyle.h"
#include "TROOT.h"
#include "TSystem.h"
#include "TString.h"

#include <map>
#include <vector>
#include <utility>
#include <iostream>
#include <cmath>

void VerifyTimewalk(int run = 1,
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

    // ---- raw hits: (x,y) -> list of (hitTime, hitEnergy) on planeID 1 ----
    TChain* raw = new TChain("RawPixelHits");
    raw->Add(Form("%s/local_%04d/output0_t*.root", resDir, run));
    int    rev, rpl, rx, ry;
    double rt;
    float  ren;
    raw->SetBranchAddress("iEvent",    &rev);
    raw->SetBranchAddress("iPlane",    &rpl);
    raw->SetBranchAddress("PixX",      &rx);
    raw->SetBranchAddress("PixY",      &ry);
    raw->SetBranchAddress("hitTime",   &rt);
    raw->SetBranchAddress("hitEnergy", &ren);

    std::map<std::pair<int,int>, std::vector<std::pair<double,float>>> mm;
    for (Long64_t i = 0; i < raw->GetEntries(); ++i)
    {
        raw->GetEntry(i);
        if (rpl == 1) mm[{rx, ry}].push_back({rt, ren});
    }

    // ---- tracked first hits ----
    TFile* f = TFile::Open(Form("%s/local_%04d/analysis_results_MP/LocalTrackedHitsThr%d.root",
                                resDir, run, threshold), "READ");
    TTree* t = (TTree*)f->Get("TrackedHits_planeZ0");

    int    mc, pid, px, py, nh;
    double tl, vt;
    t->SetBranchAddress("mcFlag",       &mc);
    t->SetBranchAddress("planeID",      &pid);
    t->SetBranchAddress("DUTPixX",      &px);
    t->SetBranchAddress("DUTPixY",      &py);
    t->SetBranchAddress("DUTnHits",     &nh);
    t->SetBranchAddress("DUTLocalTime", &tl);
    t->SetBranchAddress("vertexTime",   &vt);

    TH2D* h2 = new TH2D("h2tw",
                        "y = DUTLocalTime - TOF - rowDelay - moduleDelay;raw pixel charge C [e-];y [ns]",
                        120, 50, 4000, 130, -10, 70);
    h2->SetDirectory(0);   // detach from the TFile so f->Close() does not delete it

    int nmatch = 0;
    for (Long64_t i = 0; i < t->GetEntries(); ++i)
    {
        t->GetEntry(i);
        if (mc != 0 || pid != 1 || px == -1 || nh != 1) continue;

        auto it = mm.find({px, py});
        if (it == mm.end()) continue;

        // pick the raw deposit in this pixel whose hit time matches vertexTime+TOF
        double target = vt + 3.3;
        float bestE = -1.f;
        double bestRT = 0.;
        double bestD = 1e9;
        for (auto& [rtime, renE] : it->second)
        {
            double d = std::fabs(rtime - target);
            if (d < bestD) { bestD = d; bestE = renE; bestRT = rtime; }
        }
        if (bestE < 0.f) continue;

        double TOF = bestRT - vt;               // measured time of flight
        double y   = tl - TOF - py * 0.0125 - 24.0;   // = timewalk(C) + jitter
        h2->Fill(bestE, y);
        ++nmatch;
    }
    f->Close();
    std::cout << "Matched first hits: " << nmatch << std::endl;

    // ---- analytic timewalk curve (same parameters as the digitizer) ----
    TF1* tw = new TF1("tw", "390.0/pow(1.5*x - 149.8, 0.65)", 100.0, 4000.0);

    // ---- numerical comparison: mean measured y vs theoretical timewalk ----
    std::cout << "=== charge [e-] | measured <y> [ns] | timewalk(C) [ns] ===" << std::endl;
    TGraph* g = new TGraph();
    const double edges[10] = {50, 100, 150, 220, 320, 500, 800, 1400, 2500, 4000};
    for (int i = 0; i < 9; ++i)
    {
        int bx1 = h2->GetXaxis()->FindBin(edges[i] + 0.001);
        int bx2 = h2->GetXaxis()->FindBin(edges[i + 1] - 0.001);
        double sum = 0.; long n = 0;
        for (int bx = bx1; bx <= bx2; ++bx)
            for (int by = 1; by <= h2->GetNbinsY(); ++by)
            {
                double c = h2->GetBinContent(bx, by);
                if (c <= 0.) continue;
                sum += h2->GetYaxis()->GetBinCenter(by) * c;
                n   += (long)c;
            }
        double C   = 0.5 * (edges[i] + edges[i + 1]);
        double y   = n ? sum / n : 0.;
        double yth = (C > 100.) ? 390.0 / TMath::Power(1.5 * C - 149.8, 0.65) : 200.;
        std::cout << "  " << (int)edges[i] << "-" << (int)edges[i + 1]
                  << "  |  " << (n ? Form("%.2f", y) : "  -  ")
                  << "  |  " << yth << std::endl;
        if (n > 0) g->SetPoint(g->GetN(), C, y);
    }

    g->SetTitle("Verification: measured residual vs analytic timewalk;raw pixel charge C [e-];y = timewalk + jitter [ns]");
    g->SetMarkerStyle(20);
    g->SetMarkerColor(kBlue);

    TCanvas* c = new TCanvas("cTW", "timewalk verification", 900, 700);
    c->SetLogx(0);
    tw->SetLineColor(kRed);
    tw->SetLineWidth(2);
    tw->Draw();
    g->Draw("P same");

    TLatex l;
    l.SetNDC();
    l.SetTextSize(0.05);
    l.SetTextFont(42);
    l.DrawLatex(0.55, 0.85, "#color[2]{timewalk(C) = 390/(1.5C-149.8)^{0.65}}");
    l.DrawLatex(0.55, 0.80, "#color[4]{points = measured (charge, corrected time)}");

    c->SaveAs(Form("%s/verify_timewalk.png", od.Data()));
    c->SaveAs(Form("%s/verify_timewalk.pdf", od.Data()));

    std::cout << "Saved " << od << "/verify_timewalk.png|pdf" << std::endl;
}

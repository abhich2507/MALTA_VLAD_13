// plotting_scripts/EventTimeDist.C
// Draws the event-time distribution of Geant4 output files in a 2000 ns time window
// for ONE fixed run. Chains only the thread files output0_t*.root of that run.
//   - trueGlobalTime folded modulo 2000 ns  (in-window distribution)
//   - trueGlobalTime over the full range     (beam period / spill structure)
//
// Usage (from malta_simulation/) - pick one run:
//   root -l -b -q 'plotting_scripts/EventTimeDist.C(1, "Results_10mev_e_mp")'   // run 1
//   root -l -b -q 'plotting_scripts/EventTimeDist.C(2, "Results_10mev_e_mp")'   // run 2
// or with defaults (run 4, Results_10mev_e_mp):
//   root -l -b -q plotting_scripts/EventTimeDist.C

#include <iostream>
#include <cmath>
#include "TChain.h"
#include "TCanvas.h"
#include "TH1D.h"
#include "TMath.h"
#include "TStyle.h"
#include "TROOT.h"
#include "TString.h"

void EventTimeDist(int run = 4, const char* dir = "Results_10mev_e_mp")
{
    const double window = 2000.0; // ns (beam veto window)

    gStyle->SetOptStat(1110);
    gROOT->SetStyle("ATLAS");

    TString path = TString::Format("%s/local_%04d/output0_t*.root", dir, run);

    TChain chain("TruthVertex");
    chain.Add(path.Data());
    if (chain.GetEntries() == 0)
    {
        std::cerr << "No entries found for " << path << std::endl;
        return;
    }

    Int_t iEvent = 0;
    Float_t trueGlobalTime = 0;
    chain.SetBranchAddress("iEvent", &iEvent);
    chain.SetBranchAddress("trueGlobalTime", &trueGlobalTime);

    // in-window: fold every event time back into [0, 2000) ns
    TH1D* hInWindow = new TH1D("hInWindow", "Event time distribution in 2000 ns window",
                               200, 0., window);
    hInWindow->GetXaxis()->SetTitle("t mod 2000 [ns]");
    hInWindow->GetYaxis()->SetTitle("Events / 10 ns");

    // full range: spill structure across events
    double tMax = chain.GetMaximum("trueGlobalTime");
    TH1D* hFull = new TH1D("hFull", "Event time distribution (full range)",
                           TMath::Nint(tMax / 10.0) + 1, 0., tMax * 1.02);
    hFull->GetXaxis()->SetTitle("trueGlobalTime [ns]");
    hFull->GetYaxis()->SetTitle("Events / bin");

    //Long64_t nEntries = chain.GetEntries();
    int nEntries = 1;
    for (Long64_t i = 0; i < nEntries; ++i)
    {
        chain.GetEntry(i);
        hInWindow->Fill(fmod(trueGlobalTime, window));
        hFull->Fill(trueGlobalTime);
    }

    TCanvas* c1 = new TCanvas("c1", "in-window", 800, 600);
    c1->SetLeftMargin(0.13);
    c1->SetBottomMargin(0.13);
    hInWindow->SetLineColor(kBlue + 2);
    hInWindow->SetFillColor(kBlue - 9);
    hInWindow->Draw();
    c1->SaveAs(TString::Format("%s/EventTimeDist_%04d_window.png", dir, run));
    c1->SaveAs(TString::Format("%s/EventTimeDist_%04d_window.pdf", dir, run));

    TCanvas* c2 = new TCanvas("c2", "full-range", 800, 600);
    c2->SetLeftMargin(0.13);
    c2->SetBottomMargin(0.13);
    hFull->SetLineColor(kRed + 1);
    hFull->Draw();
    c2->SaveAs(TString::Format("%s/EventTimeDist_%04d_full.png", dir, run));
    c2->SaveAs(TString::Format("%s/EventTimeDist_%04d_full.pdf", dir, run));

    std::cout << "Entries: " << nEntries
              << "  mean t mod 2000: " << hInWindow->GetMean() << " ns"
              << "  RMS: " << hInWindow->GetRMS() << " ns" << std::endl;
    std::cout << "Saved " << dir << "/EventTimeDist_" << run << "_window/full .png/.pdf" << std::endl;
}

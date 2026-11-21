// thetaResiduals.C - hit-position residual RMS vs incidence angle from theta_scan.
// Reads Plots/local_NNNN/<save>/histos.root -> Thr100/h1Residual{X,Y}_planeZ{0,1}.
#include <fstream>
#include <sstream>
#include <vector>
#include <iostream>
#include "TFile.h"
#include "TDirectory.h"
#include "TH1D.h"
#include "TF1.h"
#include "TGraph.h"
#include "TCanvas.h"
#include "TLegend.h"
#include "TSystem.h"
#include "TString.h"
#include "TStyle.h"
#include "TROOT.h"

void thetaResiduals(const char* save = "analysis_results_224x512_theta",
                    const char* csv = "configs/theta_scan.csv",
                    const char* outdir = "Plots_thetaScan")
{
    gROOT->SetBatch(kTRUE);
    gStyle->SetOptStat(0);

    auto fitSigma = [](TH1D* h) {
        if (!h || h->GetEntries() < 100) return -1.0;
        double m = h->GetMean(), r = h->GetRMS();
        TF1 *g = new TF1("g", "gaus", m - 5*r, m + 5*r);
        g->SetParameters(h->GetMaximum(), m, r);
        h->Fit(g, "QN0");
        return g->GetParameter(2) * 1000.;   // mm -> um
    };

    std::vector<double> th, sy0, sy1, sx0, sx1;
    std::ifstream in(csv);
    std::string line;
    std::getline(in, line);
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        std::stringstream ss(line);
        int run; double t; char c;
        ss >> run >> c >> t;
        TString path = TString::Format("Plots/local_%04d/%s/histos.root", run, save);
        TFile *f = TFile::Open(path);
        if (!f || f->IsZombie()) { std::cerr << "skip " << run << std::endl; continue; }
        TDirectory *d = f->GetDirectory("Thr100");
        if (!d) { std::cerr << "skip " << run << " (no Thr100)" << std::endl; f->Close(); continue; }
        th.push_back(t);
        sy0.push_back(fitSigma((TH1D*)d->Get("h1ResidualY_planeZ0")));
        sy1.push_back(fitSigma((TH1D*)d->Get("h1ResidualY_planeZ1")));
        sx0.push_back(fitSigma((TH1D*)d->Get("h1ResidualX_planeZ0")));
        sx1.push_back(fitSigma((TH1D*)d->Get("h1ResidualX_planeZ1")));
        f->Close();
    }

    gSystem->mkdir(outdir, kTRUE);
    TCanvas *c = new TCanvas("c", "residual vs theta", 900, 600);
    TGraph *g0 = new TGraph(th.size(), th.data(), sy0.data());
    TGraph *g1 = new TGraph(th.size(), th.data(), sy1.data());
    TGraph *gx0 = new TGraph(th.size(), th.data(), sx0.data());
    g0->SetTitle("Hit-position residual vs incidence angle;#theta_{y} [deg];#sigma [#mum]");
    g0->SetMarkerStyle(20); g0->SetMarkerColor(kBlue);
    g1->SetMarkerStyle(21); g1->SetMarkerColor(kRed);
    gx0->SetMarkerStyle(22); gx0->SetMarkerColor(kGreen+2);
    g0->Draw("APL"); g1->Draw("PL SAME"); gx0->Draw("PL SAME");
    TLegend *leg = new TLegend(0.62, 0.70, 0.90, 0.88);
    leg->AddEntry(g0, "Y, plane 0", "p");
    leg->AddEntry(g1, "Y, plane 1", "p");
    leg->AddEntry(gx0, "X, plane 0", "p");
    leg->Draw();
    c->SaveAs(TString::Format("%s/residual_vs_theta.png", outdir));
    c->SaveAs(TString::Format("%s/residual_vs_theta.pdf", outdir));
    std::cout << "Saved " << outdir << "/residual_vs_theta.{png,pdf}" << std::endl;

    printf("%6s %10s %10s %10s %10s\n", "theta", "sigY_p0", "sigY_p1", "sigX_p0", "sigX_p1");
    for (size_t i = 0; i < th.size(); ++i)
        printf("%6.2f %10.2f %10.2f %10.2f %10.2f\n",
               th[i], sy0[i], sy1[i], sx0[i], sx1[i]);
}
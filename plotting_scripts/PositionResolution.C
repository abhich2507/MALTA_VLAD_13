// plotting_scripts/PositionResolution.C
// ---------------------------------------------------------------------------
// Position resolution from the residual
//     Delta = clusterX - trackX     (cluster centre of mass minus extrapolated track)
// measured on the analyzedHits_planeZ%d trees written by run_Clustering.
// Both clusterX/Y and trackX/Y are in mm, module-local frame.
//
// For every plane it produces:
//   1. residual histograms (X and Y) + core Gaussian fit + RMS + 68% half-width
//   2. signal cluster-size distribution
//   3. core sigma broken down per cluster size (explains why sigma ~20 um)
// and writes:
//   <outDir>/residual_planeZ0.png|pdf
//   <outDir>/clSize_planeZ0.png|pdf
//   <outDir>/resolution_summary.txt
//
// Usage (from malta_simulation/):
//   root -l -b -q 'plotting_scripts/PositionResolution.C()'
//   root -l -b -q 'plotting_scripts/PositionResolution.C(1, "Results_p120GeV_eEIC_cl_COM", "", 100)'
// ---------------------------------------------------------------------------

#include "TFile.h"
#include "TTree.h"
#include "TH1.h"
#include "TH2.h"
#include "TF1.h"
#include "TCanvas.h"
#include "TPad.h"
#include "TLatex.h"
#include "TStyle.h"
#include "TROOT.h"
#include "TSystem.h"
#include "TString.h"
#include "TMath.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include "TGraphErrors.h"
#include "TLegend.h"

struct ResStats
{
    Long64_t n{0};
    double   mean{0};
    double   meanErr{0};
    double   rms{0};
    double   amp{0};
    double   coreSig{0};
    double   coreSigErr{0};
    double   coreMean{0};
    double   coreMeanErr{0};
    double   hw68{0};
};

// Two-pass Gaussian: fit +/-3*RMS, then refit the core +/-2.5 sigma.
// The residual is binary-pixel-quantised, so also report the 68% half-width.
void FitResidual(TH1D* h, ResStats& s)
{
    s.n        = h->GetEntries();
    s.mean     = h->GetMean();
    s.meanErr  = h->GetMeanError();
    s.rms      = h->GetRMS();
    s.amp = s.coreSig = s.coreSigErr = s.coreMean = s.coreMeanErr = s.hw68 = 0.;

    if (s.n > 2)
    {
        double q[3] = {0., 0., 0.};
        double prob[3] = {0.16, 0.5, 0.84};
        h->GetQuantiles(3, q, prob);
        s.hw68 = 0.5 * (q[2] - q[0]);
    }

    if (s.n < 20 || s.rms <= 0.) return;

    double lo = s.mean - 3.0 * s.rms;
    double hi = s.mean + 3.0 * s.rms;
    if (hi <= lo) { lo = h->GetXaxis()->GetXmin(); hi = h->GetXaxis()->GetXmax(); }

    TF1* f1 = new TF1(Form("f1_%s", h->GetName()), "gaus", lo, hi);
    h->Fit(f1, "RQN");
    double s0 = f1->GetParameter(2);
    if (s0 <= 0.) return;

    TF1* f2 = new TF1(Form("f2_%s", h->GetName()), "gaus", s.mean - 2.5 * s0, s.mean + 2.5 * s0);
    f2->SetParameters(f1->GetParameter(0), f1->GetParameter(1), f1->GetParameter(2));
    h->Fit(f2, "RQ");

    s.amp         = f2->GetParameter(0);
    s.coreMean    = f2->GetParameter(1);
    s.coreMeanErr = f2->GetParError(1);
    s.coreSig     = f2->GetParameter(2);
    s.coreSigErr  = f2->GetParError(2);
}

void DrawResidualPad(TPad* pad, TH1D* h, const ResStats& s, const char* axisLabel)
{
    pad->cd();
    h->SetTitle(Form("%s residual (cluster COM - extrapolated track)", axisLabel));
    h->SetLineColor(kBlack);
    h->SetMarkerStyle(20);
    h->SetMarkerSize(0.6);
    h->Draw("E1");

    if (s.coreSig > 0.)
    {
        TF1* gf = new TF1(Form("gf_%s", h->GetName()), "gaus",
                          s.coreMean - 3.5 * s.coreSig, s.coreMean + 3.5 * s.coreSig);
        gf->SetParameters(s.amp, s.coreMean, s.coreSig);
        gf->SetLineColor(kRed);
        gf->SetLineWidth(2);
        gf->Draw("same");
    }

    TLatex l;
    l.SetNDC();
    l.SetTextSize(0.05);
    l.SetTextFont(42);
    l.DrawLatex(0.16, 0.85, Form("#sigma_{core} = %.2f #pm %.2f #mum", s.coreSig * 1000., s.coreSigErr * 1000.));
    l.DrawLatex(0.16, 0.78, Form("RMS = %.2f #mum,   mean = %+.2f #mum", s.rms * 1000., s.mean * 1000.));
    l.DrawLatex(0.16, 0.71, Form("68%% half-width = %.2f #mum   (N = %lld)", s.hw68 * 1000., s.n));
}

void PositionResolution(int run = 1,
                        const char* resDir = "Results_p120GeV_eEIC_cl_COM",
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

    TString fname = TString::Format("%s/local_%04d/analysis_results_MP/analysisThr%d.root",
                                    resDir, run, threshold);
    TFile* f = TFile::Open(fname, "READ");
    if (!f || f->IsZombie())
    {
        std::cerr << "Cannot open " << fname << std::endl;
        return;
    }

    std::ofstream sum(TString::Format("%s/resolution_summary.txt", od.Data()).Data());
    auto report = [&](const char* line) { std::cout << line; sum << line; };
    report(TString::Format("# Position resolution, run %d, %s\n", run, resDir).Data());
    report("# residual = clusterX/Y - trackX/Y  [values in um; 1 um = 0.001 mm]\n");
    report("# selection: mcFlag==0 && clSize>0 (signal clusters)\n");
    report("# plane    axis     N        mean[um]      RMS[um]   coreSig[um]    coreMean[um]    hw68[um]\n");

    const double um = 1000.;

    for (int pz = 0; pz < 32; ++pz)
    {
        TTree* t = (TTree*)f->Get(Form("analyzedHits_planeZ%d", pz));
        if (!t) break;   // planes are contiguous 0..nPlanes_100-1

        double clusterX, clusterY, trackX, trackY;
        int    clSize, mcFlag;
        t->SetBranchAddress("clusterX", &clusterX);
        t->SetBranchAddress("clusterY", &clusterY);
        t->SetBranchAddress("trackX",   &trackX);
        t->SetBranchAddress("trackY",   &trackY);
        t->SetBranchAddress("clSize",   &clSize);
        t->SetBranchAddress("mcFlag",   &mcFlag);

        TH1D* hx  = new TH1D(Form("hResX_planeZ%d",  pz), Form("hResX_planeZ%d",  pz), 300, -0.15, 0.15);
        TH1D* hy  = new TH1D(Form("hResY_planeZ%d",  pz), Form("hResY_planeZ%d",  pz), 300, -0.15, 0.15);
        TH1D* hc  = new TH1D(Form("hClSz_planeZ%d",  pz), Form("hClSz_planeZ%d",  pz), 10, 0, 10);
        TH1D* hx1 = new TH1D(Form("hResX_planeZ%d_c1", pz), Form("hResX_planeZ%d_c1", pz), 300, -0.15, 0.15);
        TH1D* hx2 = new TH1D(Form("hResX_planeZ%d_c2", pz), Form("hResX_planeZ%d_c2", pz), 300, -0.15, 0.15);
        TH1D* hx3 = new TH1D(Form("hResX_planeZ%d_c3", pz), Form("hResX_planeZ%d_c3", pz), 300, -0.15, 0.15);
        TH1D* hx4 = new TH1D(Form("hResX_planeZ%d_c4", pz), Form("hResX_planeZ%d_c4", pz), 300, -0.15, 0.15);

        hx->GetXaxis()->SetTitle("clusterX - trackX [mm]");
        hy->GetXaxis()->SetTitle("clusterY - trackY [mm]");
        hc->GetXaxis()->SetTitle("cluster size [pixels]");

        for (Long64_t i = 0; i < t->GetEntries(); ++i)
        {
            t->GetEntry(i);
            if (mcFlag != 0 || clSize <= 0) continue;
            double dx = clusterX - trackX;
            double dy = clusterY - trackY;
            hx->Fill(dx);
            hy->Fill(dy);
            hc->Fill(clSize);
            if (clSize == 1)      hx1->Fill(dx);
            else if (clSize == 2) hx2->Fill(dx);
            else if (clSize == 3) hx3->Fill(dx);
            else if (clSize == 4) hx4->Fill(dx);
        }

        ResStats sx, sy;
        FitResidual(hx, sx);
        FitResidual(hy, sy);

        char line[256];
        snprintf(line, sizeof(line), "%6d     X  %8lld   %+9.2f   %8.2f   %7.2f+/-%.2f   %+9.2f   %8.2f\n",
                 pz, sx.n, sx.mean * um, sx.rms * um, sx.coreSig * um, sx.coreSigErr * um, sx.coreMean * um, sx.hw68 * um);
        report(line);
        snprintf(line, sizeof(line), "%6d     Y  %8lld   %+9.2f   %8.2f   %7.2f+/-%.2f   %+9.2f   %8.2f\n",
                 pz, sy.n, sy.mean * um, sy.rms * um, sy.coreSig * um, sy.coreSigErr * um, sy.coreMean * um, sy.hw68 * um);
        report(line);

        // per-cluster-size breakdown (X) to explain the overall sigma
        report(TString::Format("# planeZ%d  <clSize> = %.3f   X core sigma vs clSize:\n", pz, hc->GetMean()).Data());
        for (int c = 1; c <= 4; ++c)
        {
            TH1D* h = (c == 1 ? hx1 : (c == 2 ? hx2 : (c == 3 ? hx3 : hx4)));
            ResStats sc;
            FitResidual(h, sc);
            snprintf(line, sizeof(line), "#   clSize=%d  N=%8lld   coreSig=%6.2f+/-%.2f um   RMS=%6.2f um\n",
                     c, sc.n, sc.coreSig * um, sc.coreSigErr * um, sc.rms * um);
            report(line);
        }

        // --- residual canvas (X top, Y bottom) ---
        TCanvas* c1 = new TCanvas(Form("cRes_%d", pz), "residual", 1000, 800);
        TPad* p1 = new TPad(Form("p1_%d", pz), "", 0, 0.52, 1, 1);
        p1->SetBottomMargin(0.02);
        TPad* p2 = new TPad(Form("p2_%d", pz), "", 0, 0, 1, 0.48);
        p2->SetTopMargin(0.02);
        p2->SetBottomMargin(0.15);
        c1->cd();
        p1->Draw();
        p2->Draw();
        DrawResidualPad(p1, hx, sx, "X");
        DrawResidualPad(p2, hy, sy, "Y");
        c1->SaveAs(Form("%s/residual_planeZ%d.png", od.Data(), pz));
        c1->SaveAs(Form("%s/residual_planeZ%d.pdf", od.Data(), pz));

        // --- cluster-size canvas ---
        TCanvas* c2 = new TCanvas(Form("cCl_%d", pz), "cluster size", 700, 600);
        c2->cd();
        hc->Draw("HIST");
        TLatex l;
        l.SetNDC();
        l.SetTextSize(0.05);
        l.SetTextFont(42);
        l.DrawLatex(0.62, 0.82, Form("<clSize> = %.3f", hc->GetMean()));
        c2->SaveAs(Form("%s/clSize_planeZ%d.png", od.Data(), pz));
        c2->SaveAs(Form("%s/clSize_planeZ%d.pdf", od.Data(), pz));
    }

    sum.close();
    f->Close();
    std::cout << "Wrote plots and summary to " << od << std::endl;
}

// ---------------------------------------------------------------------------
// Position resolution vs background rate (plot 11.2).
// Loops over the runs listed in configs/bkg_rate.csv, computes the residual
// (clusterX/Y - trackX/Y) for signal clusters (mcFlag==0 && clSize>0) per
// plane, and plots three metrics vs bkgRateMean:
//   - core Gaussian sigma  (with fit error bars)
//   - RMS
//   - 68% central half-width
// Outputs:
//   <outDir>/resolution_vs_bkg.csv
//   <outDir>/resVsBkg_core.png|pdf, _rms, _hw68
//
// Usage (from malta_simulation/):
//   root -l -b -q -e 'gROOT->ProcessLine(".L plotting_scripts/PositionResolution.C"); PositionResolutionVsRate()'
// ---------------------------------------------------------------------------

void DrawRatePad(TPad* pad, std::vector<TGraphErrors*>& grs,
                 const char* axisLabel, const char* metricLabel)
{
    pad->cd();
    const int colors[2] = {kRed, kBlue};
    const int mstyles[2] = {20, 21};

    double ymax = 0.;
    for (auto* g : grs)
        for (int i = 0; i < g->GetN(); ++i)
            ymax = TMath::Max(ymax, g->GetY()[i]);
    ymax *= 1.25;

    grs[0]->SetTitle(Form("%s: %s vs background rate;Background rate #lambda [particles/event];%s [#mum]",
                          axisLabel, metricLabel, metricLabel));
    grs[0]->GetYaxis()->SetRangeUser(0, ymax);
    grs[0]->Draw("AP");
    for (size_t i = 1; i < grs.size(); ++i) grs[i]->Draw("P same");

    TLegend* lg = new TLegend(0.15, 0.72, 0.42, 0.88);
    lg->SetTextSize(0.05);
    for (size_t i = 0; i < grs.size(); ++i)
        lg->AddEntry(grs[i], Form("planeZ%zu", i), "p");
    lg->Draw();
}

void PositionResolutionVsRate(const char* resDir = "Results_p120GeV_eEIC_cl_COM",
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

    // (run, rate) from configs/bkg_rate.csv
    struct RatePoint { int run; double rate; };
    std::vector<RatePoint> pts;
    {
        std::ifstream in("configs/bkg_rate.csv");
        std::string line;
        std::getline(in, line);   // header
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

    // discover number of planes from the first available run
    int nPlanes = 0;
    for (const auto& p : pts)
    {
        TFile* f0 = TFile::Open(Form("%s/local_%04d/analysis_results_MP/analysisThr%d.root",
                                     resDir, p.run, threshold), "READ");
        if (!f0 || f0->IsZombie()) { if (f0) f0->Close(); continue; }
        while (f0->Get(Form("analyzedHits_planeZ%d", nPlanes))) ++nPlanes;
        f0->Close();
        break;
    }
    if (nPlanes == 0) { std::cerr << "No analyzedHits trees found in " << resDir << std::endl; return; }

    std::vector<std::vector<double>> coreX(nPlanes), coreY(nPlanes), coreXE(nPlanes), coreYE(nPlanes);
    std::vector<std::vector<double>> rmsX(nPlanes), rmsY(nPlanes);
    std::vector<std::vector<double>> hwX(nPlanes), hwY(nPlanes);
    std::vector<double> rates;

    const double um = 1000.;

    std::ofstream csv(Form("%s/resolution_vs_bkg.csv", od.Data()));
    csv << "plane,axis,rate,n,mean_um,coreSig_um,coreSigErr_um,rms_um,hw68_um\n";

    for (const auto& p : pts)
    {
        TFile* f = TFile::Open(Form("%s/local_%04d/analysis_results_MP/analysisThr%d.root",
                                    resDir, p.run, threshold), "READ");
        if (!f || f->IsZombie())
        {
            if (f) f->Close();
            std::cerr << "skip run " << p.run << " (missing file)" << std::endl;
            continue;
        }
        rates.push_back(p.rate);

        for (int pz = 0; pz < nPlanes; ++pz)
        {
            TTree* t = (TTree*)f->Get(Form("analyzedHits_planeZ%d", pz));
            if (!t) continue;

            double clusterX, clusterY, trackX, trackY;
            int    clSize, mcFlag;
            t->SetBranchAddress("clusterX", &clusterX);
            t->SetBranchAddress("clusterY", &clusterY);
            t->SetBranchAddress("trackX",   &trackX);
            t->SetBranchAddress("trackY",   &trackY);
            t->SetBranchAddress("clSize",   &clSize);
            t->SetBranchAddress("mcFlag",   &mcFlag);

            TH1D* hx = new TH1D(Form("vR_hx_%d_%d", p.run, pz), "", 300, -0.15, 0.15);
            TH1D* hy = new TH1D(Form("vR_hy_%d_%d", p.run, pz), "", 300, -0.15, 0.15);
            for (Long64_t i = 0; i < t->GetEntries(); ++i)
            {
                t->GetEntry(i);
                if (mcFlag != 0 || clSize <= 0) continue;
                hx->Fill(clusterX - trackX);
                hy->Fill(clusterY - trackY);
            }

            ResStats sx, sy;
            FitResidual(hx, sx);
            FitResidual(hy, sy);

            coreX[pz].push_back(sx.coreSig * um);
            coreY[pz].push_back(sy.coreSig * um);
            coreXE[pz].push_back(sx.coreSigErr * um);
            coreYE[pz].push_back(sy.coreSigErr * um);
            rmsX[pz].push_back(sx.rms * um);
            rmsY[pz].push_back(sy.rms * um);
            hwX[pz].push_back(sx.hw68 * um);
            hwY[pz].push_back(sy.hw68 * um);

            csv << pz << ",X," << p.rate << "," << sx.n << ","
                << sx.mean * um << "," << sx.coreSig * um << "," << sx.coreSigErr * um << ","
                << sx.rms * um << "," << sx.hw68 * um << "\n";
            csv << pz << ",Y," << p.rate << "," << sy.n << ","
                << sy.mean * um << "," << sy.coreSig * um << "," << sy.coreSigErr * um << ","
                << sy.rms * um << "," << sy.hw68 * um << "\n";
        }
        f->Close();
    }
    csv.close();

    const char* axes[2] = {"X", "Y"};
    // core sigma canvas
    {
        TCanvas* c = new TCanvas("cVsRate_core", "core sigma vs rate", 1000, 800);
        TPad* p1 = new TPad("p1", "", 0, 0.52, 1, 1);  p1->SetBottomMargin(0.02);
        TPad* p2 = new TPad("p2", "", 0, 0, 1, 0.48);  p2->SetTopMargin(0.02); p2->SetBottomMargin(0.15);
        c->cd(); p1->Draw(); p2->Draw();
        std::vector<TGraphErrors*> gx, gy;
        for (int pz = 0; pz < nPlanes; ++pz)
        {
            gx.push_back(new TGraphErrors(rates.size(), rates.data(), coreX[pz].data(), nullptr, coreXE[pz].data()));
            gy.push_back(new TGraphErrors(rates.size(), rates.data(), coreY[pz].data(), nullptr, coreYE[pz].data()));
        }
        for (int pz = 0; pz < nPlanes; ++pz)
        {
            gx[pz]->SetMarkerStyle(pz == 0 ? 20 : 21);
            gx[pz]->SetMarkerColor(pz == 0 ? kRed : kBlue);
            gx[pz]->SetLineColor(pz == 0 ? kRed : kBlue);
            gy[pz]->SetMarkerStyle(pz == 0 ? 20 : 21);
            gy[pz]->SetMarkerColor(pz == 0 ? kRed : kBlue);
            gy[pz]->SetLineColor(pz == 0 ? kRed : kBlue);
        }
        DrawRatePad(p1, gx, "X", "core #sigma");
        DrawRatePad(p2, gy, "Y", "core #sigma");
        c->SaveAs(Form("%s/resVsBkg_core.png", od.Data()));
        c->SaveAs(Form("%s/resVsBkg_core.pdf", od.Data()));
    }
    // RMS canvas
    {
        TCanvas* c = new TCanvas("cVsRate_rms", "RMS vs rate", 1000, 800);
        TPad* p1 = new TPad("p1", "", 0, 0.52, 1, 1);  p1->SetBottomMargin(0.02);
        TPad* p2 = new TPad("p2", "", 0, 0, 1, 0.48);  p2->SetTopMargin(0.02); p2->SetBottomMargin(0.15);
        c->cd(); p1->Draw(); p2->Draw();
        std::vector<TGraphErrors*> gx, gy;
        for (int pz = 0; pz < nPlanes; ++pz)
        {
            gx.push_back(new TGraphErrors(rates.size(), rates.data(), rmsX[pz].data()));
            gy.push_back(new TGraphErrors(rates.size(), rates.data(), rmsY[pz].data()));
            gx[pz]->SetMarkerStyle(pz == 0 ? 20 : 21); gx[pz]->SetMarkerColor(pz == 0 ? kRed : kBlue); gx[pz]->SetLineColor(pz == 0 ? kRed : kBlue);
            gy[pz]->SetMarkerStyle(pz == 0 ? 20 : 21); gy[pz]->SetMarkerColor(pz == 0 ? kRed : kBlue); gy[pz]->SetLineColor(pz == 0 ? kRed : kBlue);
        }
        DrawRatePad(p1, gx, "X", "RMS");
        DrawRatePad(p2, gy, "Y", "RMS");
        c->SaveAs(Form("%s/resVsBkg_rms.png", od.Data()));
        c->SaveAs(Form("%s/resVsBkg_rms.pdf", od.Data()));
    }
    // 68% half-width canvas
    {
        TCanvas* c = new TCanvas("cVsRate_hw", "68% half-width vs rate", 1000, 800);
        TPad* p1 = new TPad("p1", "", 0, 0.52, 1, 1);  p1->SetBottomMargin(0.02);
        TPad* p2 = new TPad("p2", "", 0, 0, 1, 0.48);  p2->SetTopMargin(0.02); p2->SetBottomMargin(0.15);
        c->cd(); p1->Draw(); p2->Draw();
        std::vector<TGraphErrors*> gx, gy;
        for (int pz = 0; pz < nPlanes; ++pz)
        {
            gx.push_back(new TGraphErrors(rates.size(), rates.data(), hwX[pz].data()));
            gy.push_back(new TGraphErrors(rates.size(), rates.data(), hwY[pz].data()));
            gx[pz]->SetMarkerStyle(pz == 0 ? 20 : 21); gx[pz]->SetMarkerColor(pz == 0 ? kRed : kBlue); gx[pz]->SetLineColor(pz == 0 ? kRed : kBlue);
            gy[pz]->SetMarkerStyle(pz == 0 ? 20 : 21); gy[pz]->SetMarkerColor(pz == 0 ? kRed : kBlue); gy[pz]->SetLineColor(pz == 0 ? kRed : kBlue);
        }
        DrawRatePad(p1, gx, "X", "68% half-width");
        DrawRatePad(p2, gy, "Y", "68% half-width");
        c->SaveAs(Form("%s/resVsBkg_hw68.png", od.Data()));
        c->SaveAs(Form("%s/resVsBkg_hw68.pdf", od.Data()));
    }

    std::cout << "Wrote " << od << "/resolution_vs_bkg.csv and resVsBkg_*.png|pdf" << std::endl;
}

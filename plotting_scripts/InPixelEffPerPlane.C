// plotting_scripts/InPixelEffPerPlane.C
// Reads per-run in-pixel efficiencies (with binomial errors) for planeZ0 and planeZ1
// from Plots/local_NNNN/analysis_results_MP/histos.root (directory "Thr100").
// Maps run -> background count via configs/bkg_count.csv and writes a single CSV
// to Results/inPixel_eff.csv with columns:
//   run,count,eff_z0,effErr_z0,eff_z1,effErr_z1
//
// Note: h2PASSInPixel_planeZ% is the per-bin efficiency map (scaled to %),
// while h2PASSInPixelAux_planeZ% holds the raw PASS counts used for the error.
// Usage (from malta_simulation/):  root -l -b -q plotting_scripts/InPixelEffPerPlane.C

#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <cmath>
#include "TFile.h"
#include "TDirectory.h"
#include "TH2.h"
#include "TString.h"

const char* THR_DIR = "Thr100";
const char* OUT_CSV = "Results/inPixel_eff.csv";
const char* OUT_PNG = "Results/inPixel_eff_z0_z1.png";
const char* OUT_PDF = "Results/inPixel_eff_z0_z1.pdf";

struct EffErr
{
    double eff = -999.;
    double err = -999.;
};

EffErr ComputeEffErr(TH2D* hPassScaled, TH2D* hPassRaw, TH2D* hAll)
{
    EffErr out;
    if (!hAll || !(hPassRaw || hPassScaled)) return out;

    double Nall = hAll->Integral();
    if (Nall <= 0.) return out;

    double Npass = 0.;
    if (hPassRaw)
    {
        // raw pass counts, as used by GetStatistics in the analysis
        Npass = hPassRaw->Integral();
    }
    else
    {
        // fallback: recover raw pass counts from the scaled efficiency map
        // (bin value is the in-pixel efficiency in percent)
        for (int bx = 1; bx <= hPassScaled->GetNbinsX(); ++bx)
        {
            for (int by = 1; by <= hPassScaled->GetNbinsY(); ++by)
            {
                Npass += hPassScaled->GetBinContent(bx, by) / 100.0 * hAll->GetBinContent(bx, by);
            }
        }
    }

    double p = Npass / Nall;
    out.eff = 100.0 * p;
    out.err = 100.0 * std::sqrt(p * (1.0 - p) / Nall);  // same formula as AnalysisUtils::getEffErr
    return out;
}

void InPixelEffPerPlane()
{
    // read run -> count map (keeps the order of bkg_count.csv)
    std::vector<std::pair<int,int> > runCount;
    {
        std::ifstream in("configs/bkg_count.csv");
        if (!in) { std::cerr << "Cannot open configs/bkg_count.csv" << std::endl; return; }
        std::string line;
        std::getline(in, line);  // skip header
        while (std::getline(in, line))
        {
            if (line.empty()) continue;
            std::stringstream ss(line);
            int run = 0, count = 0;
            char comma;
            ss >> run >> comma >> count;
            runCount.push_back(std::make_pair(run, count));
        }
        in.close();
    }

    std::ofstream out(OUT_CSV);
    out << "run,count,eff_z0,effErr_z0,eff_z1,effErr_z1" << std::endl;

    for (size_t i = 0; i < runCount.size(); ++i)
    {
        int run   = runCount[i].first;
        int count = runCount[i].second;

        TString path = TString::Format("Plots/local_%04d/analysis_results_MP/histos.root", run);
        EffErr z0, z1;

        TFile* f = TFile::Open(path.Data(), "READ");
        if (!f || f->IsZombie())
        {
            std::cerr << "skip run " << run << ": " << path << " missing" << std::endl;
        }
        else
        {
            TDirectory* d = f->GetDirectory(THR_DIR);
            if (!d)
            {
                std::cerr << "skip run " << run << ": directory " << THR_DIR << " missing" << std::endl;
            }
            else
            {
                TH2D* pass0 = (TH2D*)d->Get("h2PASSInPixel_planeZ0");
                TH2D* pass1 = (TH2D*)d->Get("h2PASSInPixel_planeZ1");
                TH2D* raw0  = (TH2D*)d->Get("h2PASSInPixelAux_planeZ0");
                TH2D* raw1  = (TH2D*)d->Get("h2PASSInPixelAux_planeZ1");
                TH2D* all0  = (TH2D*)d->Get("h2ALLInPixel_planeZ0");
                TH2D* all1  = (TH2D*)d->Get("h2ALLInPixel_planeZ1");

                z0 = ComputeEffErr(pass0, raw0, all0);
                z1 = ComputeEffErr(pass1, raw1, all1);
            }
            f->Close();
        }

        out << run << "," << count << ","
            << z0.eff << "," << z0.err << ","
            << z1.eff << "," << z1.err << std::endl;

        std::cout << "run " << run << " (count=" << count << "): "
                  << "z0 = " << z0.eff << " +- " << z0.err << "   "
                  << "z1 = " << z1.eff << " +- " << z1.err << std::endl;
    }

    out.close();
    std::cout << "Written " << OUT_CSV << std::endl;
}

// save_theta_eff.C
// Reads a theta-scan CSV (run, theta_deg) and the per-plane efficiency stored in
// each run's Plots/local_NNNN/<SAVE>/histos.root (Thr<THRESHOLD> dir), then writes
// <outdir>/theta_eff.csv with columns: plane, theta_deg, avgEff, effError.
// Usage:
//   root -l -b -q 'save_theta_eff.C("analysis_results_224x512_thetaX", 100, "Results_thetaX", "Plots_thetaX", "configs/theta_scan_X.csv")'
#include <fstream>
#include <sstream>
#include <iostream>
#include <cmath>
#include "TFile.h"
#include "TDirectory.h"
#include "TH2.h"
#include "TSystem.h"
#include "TString.h"

static TString ResolveHistosPath(int run, const char* save, const char* plotdir)
{
    TString live = TString::Format("Plots/local_%04d/%s/histos.root", run, save);
    if (!gSystem->AccessPathName(live.Data())) return live;
    return TString::Format("%s/local_%04d/%s/histos.root", plotdir, run, save);
}

void save_theta_eff(const char* save = "analysis_results_224x512_thetaX", int threshold = 100,
                    const char* outdir = "Results_thetaX", const char* plotdir = "Plots_thetaX",
                    const char* csvPath = "configs/theta_scan_X.csv")
{
    gSystem->mkdir(outdir, kTRUE);

    std::ifstream in(csvPath);
    if (!in) { std::cerr << "Cannot open " << csvPath << std::endl; return; }

    std::string outPath = std::string(outdir) + "/theta_eff.csv";
    std::ofstream out(outPath.c_str());
    out << "plane,theta_deg,avgEff,effError" << std::endl;

    std::string line;
    std::getline(in, line);  // skip header
    while (std::getline(in, line))
    {
        if (line.empty()) continue;
        std::stringstream ss(line);
        int run; double theta; char comma;
        ss >> run >> comma >> theta;

        TString path = ResolveHistosPath(run, save, plotdir);
        if (gSystem->AccessPathName(path.Data())) { std::cout << "skip run " << run << ": " << path << " missing" << std::endl; continue; }

        TFile *f = TFile::Open(path.Data());
        if (!f || f->IsZombie()) { std::cout << "skip run " << run << ": cannot open " << path << std::endl; continue; }
        TDirectory *thrDir = f->GetDirectory(Form("Thr%d", threshold));
        if (!thrDir) { std::cout << "skip run " << run << ": no Thr" << threshold << " dir" << std::endl; f->Close(); continue; }

        for (int plane = 0; ; ++plane)
        {
            TH2D *hAll = (TH2D*) thrDir->Get(Form("h2ALLInPixel_planeZ%d", plane));
            if (!hAll) break;
            double nGen = hAll->Integral();
            double nPass = 0.0;
            TH2D *hAux = (TH2D*) thrDir->Get(Form("h2PASSInPixelAux_planeZ%d", plane));
            if (hAux) nPass = hAux->Integral();
            else {
                TH2D *hEffMap = (TH2D*) thrDir->Get(Form("h2PASSInPixel_planeZ%d", plane));
                if (!hEffMap) continue;
                for (int bx = 1; bx <= hEffMap->GetNbinsX(); ++bx)
                    for (int by = 1; by <= hEffMap->GetNbinsY(); ++by)
                        nPass += (hEffMap->GetBinContent(bx, by) / 100.0) * hAll->GetBinContent(bx, by);
            }
            double eff = (nGen > 0) ? 100.0 * nPass / nGen : 0.0;
            double err = (nGen > 0) ? 100.0 * std::sqrt((nPass / nGen) * (1.0 - nPass / nGen) / nGen) : 0.0;
            out << plane << "," << theta << "," << eff << "," << err << std::endl;
            std::cout << "run " << run << " planeZ" << plane << ": theta=" << theta
                      << " N_gen=" << (long long)nGen << " N_pass=" << (long long)nPass
                      << " avgEff=" << eff << " +- " << err << std::endl;
        }
        f->Close();
    }
    out.close();
    in.close();
    std::cout << "Written " << outdir << "/theta_eff.csv" << std::endl;
}
// save_bkg_eff.C
// Reads configs/bkg_count.csv (run,count) and the per-plane efficiency stored in
// each run's Plots/local_NNNN/<SAVE>/histos.root (directory Thr<THRESHOLD>),
// then writes configs/bkg_eff.csv with columns:
//   plane, count-1, avgEff, effError
// Efficiency per plane p:
//   N_gen  = Integral(h2ALLInPixel_planeZ<p>)        (all signal tracks)
//   N_pass = Integral(h2PASSInPixelAux_planeZ<p>)   (unscaled pass counts, preferred)
//         or reconstructed from the efficiency map:
//            sum_bins( h2PASSInPixel_planeZ<p>/100 * h2ALLInPixel_planeZ<p> )
//   eff    = 100 * N_pass / N_gen
// Usage (from malta_simulation/):  root -l -b -q save_bkg_eff.C

#include <fstream>
#include <sstream>
#include <iostream>
#include <cmath>
#include "TFile.h"
#include "TDirectory.h"
#include "TH2.h"
#include "TSystem.h"
#include "TString.h"

void save_bkg_eff()
{
    const char* SAVE = "analysis_results_MP";   // must match run_mult.sh SAVE
    const int THRESHOLD = 100;                  // must match run_mult.sh THRESHOLD

    std::ifstream in("configs/bkg_count.csv");
    if (!in) { std::cerr << "Cannot open configs/bkg_count.csv" << std::endl; return; }

    std::ofstream out("configs/bkg_eff.csv");
    out << "plane,count-1,avgEff,effError" << std::endl;

    std::string line;
    std::getline(in, line);  // skip header
    while (std::getline(in, line))
    {
        if (line.empty()) continue;
        std::stringstream ss(line);
        int run, count;
        char comma;
        ss >> run >> comma >> count;

        TString path = TString::Format("Plots/local_%04d/%s/histos.root", run, SAVE);
        if (gSystem->AccessPathName(path.Data()))
        {
            std::cout << "skip run " << run << ": " << path << " missing" << std::endl;
            continue;
        }

        TFile *f = TFile::Open(path.Data());
        if (!f || f->IsZombie())
        {
            std::cout << "skip run " << run << ": cannot open " << path.Data() << std::endl;
            continue;
        }
        TDirectory *thrDir = f->GetDirectory(Form("Thr%d", THRESHOLD));
        if (!thrDir)
        {
            std::cout << "skip run " << run << ": no Thr" << THRESHOLD << " directory in " << path.Data() << std::endl;
            f->Close();
            continue;
        }

        // Loop over planes until the ALL histogram for that plane is not found
        for (int plane = 0; ; ++plane)
        {
            TH2D *hAll = (TH2D*) thrDir->Get(Form("h2ALLInPixel_planeZ%d", plane));
            if (!hAll) break; // no more planes

            double nGen = hAll->Integral();
            double nPass = 0.0;

            // Prefer the unscaled aux histogram; otherwise reconstruct the pass
            // counts from the efficiency map (PASS/ALL*100 per bin).
            TH2D *hAux = (TH2D*) thrDir->Get(Form("h2PASSInPixelAux_planeZ%d", plane));
            if (hAux)
            {
                nPass = hAux->Integral();
            }
            else
            {
                TH2D *hEffMap = (TH2D*) thrDir->Get(Form("h2PASSInPixel_planeZ%d", plane));
                if (!hEffMap)
                {
                    std::cout << "run " << run << " planeZ" << plane << ": no pass histogram found, skipping" << std::endl;
                    continue;
                }
                for (int bx = 1; bx <= hEffMap->GetNbinsX(); ++bx)
                {
                    for (int by = 1; by <= hEffMap->GetNbinsY(); ++by)
                    {
                        nPass += (hEffMap->GetBinContent(bx, by) / 100.0) * hAll->GetBinContent(bx, by);
                    }
                }
            }

            double eff = (nGen > 0) ? 100.0 * nPass / nGen : 0.0;
            double err = (nGen > 0) ? 100.0 * std::sqrt((nPass / nGen) * (1.0 - nPass / nGen) / nGen) : 0.0;

            out << plane << "," << (count - 1) << "," << eff << "," << err << std::endl;
            std::cout << "run " << run << " planeZ" << plane << ": count-1=" << (count - 1)
                      << " N_gen=" << (long long)nGen << " N_pass=" << (long long)nPass
                      << " avgEff=" << eff << " +- " << err << std::endl;
        }
        f->Close();
    }

    out.close();
    in.close();
    std::cout << "Written configs/bkg_eff.csv" << std::endl;
}

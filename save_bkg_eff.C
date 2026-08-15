// save_bkg_eff.C
// Reads configs/bkg_count.csv (run,count) and the efficiency stored in each run's
// Plots/local_NNNN/<SAVE>/summary.root, then writes configs/bkg_eff.csv with columns:
//   count-1, avgEff, effError
// Usage (from malta_simulation/):  root -l -b -q save_bkg_eff.C

#include <fstream>
#include <sstream>
#include <iostream>
#include "TFile.h"
#include "TTree.h"
#include "TSystem.h"
#include "TString.h"

void save_bkg_eff()
{
    const char* SAVE = "analysis_results_MP";   // must match run_mult.sh SAVE

    std::ifstream in("configs/bkg_count.csv");
    if (!in) { std::cerr << "Cannot open configs/bkg_count.csv" << std::endl; return; }

    std::ofstream out("configs/bkg_eff.csv");
    out << "count-1,avgEff,effError" << std::endl;

    std::string line;
    std::getline(in, line);  // skip header
    while (std::getline(in, line))
    {
        if (line.empty()) continue;
        std::stringstream ss(line);
        int run, count;
        char comma;
        ss >> run >> comma >> count;

        TString path = TString::Format("Plots/local_%04d/%s/summary.root", run, SAVE);
        if (gSystem->AccessPathName(path.Data()))
        {
            std::cout << "skip run " << run << ": " << path << " missing" << std::endl;
            continue;
        }

        TFile *f = TFile::Open(path.Data());
        TTree *tree = (TTree*) f->Get("summaryTree");
        double efficiency = 0.;
        double effError = 0.;
        double threshold = 0.;
        tree->SetBranchAddress("efficiency", &efficiency);
        tree->SetBranchAddress("effError", &effError);
        tree->SetBranchAddress("threshold", &threshold);
        tree->GetEntry(tree->GetEntries() - 1);  // last filled entry

        out << (count - 1) << "," << efficiency << "," << effError << std::endl;
        std::cout << "run " << run << ": count-1=" << (count - 1)
                  << " avgEff=" << efficiency << " +- " << effError << std::endl;
        f->Close();
    }

    out.close();
    in.close();
    std::cout << "Written configs/bkg_eff.csv" << std::endl;
}

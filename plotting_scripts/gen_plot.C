#include "TChain.h"
#include "TH1D.h"
#include "TH2D.h"
#include "TH3D.h"
#include "TCanvas.h"
#include "TSystem.h"

// gen_plot.C
// TruthVertex 1D/2D/3D plots for a run.
// IMPORTANT: the truth ntuple stores positions in MM (Geant4 internal units),
// not cm. Beam X ~ 27.96 mm (beamXOffset = 2.79552 cm), gun at z ~ -1000 mm.
// Usage: root -l -b -q 'gen_plot.C(1)'
void gen_plot(int run = 1)
{
    // Chain all thread output files of the run
    TChain* chain = new TChain("TruthVertex");
    chain->Add(Form("Results/local_%04d/output0_t*.root", run));
    if (!chain || chain->GetEntries() == 0)
    {
        std::cerr << "No TruthVertex entries found for run " << run << std::endl;
        return;
    }

    // Histogram ranges in mm:
    // X: ladder spans -9.3 .. 65.2 mm, beam at 27.96 mm
    // Y: beam at 0
    // Z: gun at -1000 mm (sigma 100 mm), planes at z = 0 and 100 mm
    auto histvX   = new TH1D("histvX",   "Vertex X;x [mm]",         100, -10., 70.);
    auto histvY   = new TH1D("histvY",   "Vertex Y;y [mm]",         100, -10., 10.);
    auto histvZ   = new TH1D("histvZ",   "Vertex Z;z [mm]",         100, -1500., 500.);
    auto histvXY  = new TH2D("histvXY",  "Vertex XY;x [mm];y [mm]", 100, -10., 70., 100, -10., 10.);
    auto histvXYZ = new TH3D("histvXYZ", "Vertex XYZ;x [mm];y [mm];z [mm]",
                             100, -10., 70., 100, -10., 10., 100, -1500., 500.);

    // Correct branch names (see RunAction.cc)
    float vertexX = 0, vertexY = 0, vertexZ = 0;
    chain->SetBranchAddress("trueVertexX", &vertexX);
    chain->SetBranchAddress("trueVertexY", &vertexY);
    chain->SetBranchAddress("trueVertexZ", &vertexZ);

    Long64_t nEntries = chain->GetEntries();
    for (Long64_t i = 0; i < nEntries; ++i)
    {
        chain->GetEntry(i);
        histvX->Fill(vertexX);
        histvY->Fill(vertexY);
        histvZ->Fill(vertexZ);
        histvXY->Fill(vertexX, vertexY);
        histvXYZ->Fill(vertexX, vertexY, vertexZ);
    }
    std::cout << "entries: " << nEntries
              << "  meanX = " << histvX->GetMean() << " mm"
              << "  meanY = " << histvY->GetMean() << " mm"
              << "  meanZ = " << histvZ->GetMean() << " mm" << std::endl;

    TCanvas* c1 = new TCanvas("c1", "Vertex Distributions", 1200, 600);
    histvXYZ->Draw("BOX2");

    gSystem->mkdir("Plots", kTRUE);
    c1->SaveAs("Plots/gen_plot_vertexXYZ.png");
    c1->SaveAs(Form("Results/local_%04d/vertex_distributions.root", run));
}
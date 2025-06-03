#include <ROOT/RNTuple.hxx>
#include <TH3D.h>
#include <TFile.h>
#include <iostream>

void in_pixel_plots() {
    

    //TFile *file = TFile::Open("/home/vlad/Documents/Simu/Geant4/DECAL/build/output0_t0.root");
    //TTree *tree = (TTree*)file->Get("EnDeposited");
    // Scale it up to include all threads
    TChain *chain = new TChain("EnDeposited");

    for (int t = 0; t <= 5; ++t) {
        chain->Add(Form("/home/vlad/Documents/Simu/Geant4/DECAL/Data/condor_submission/output0_t%d.root", t));
    }

    // Variables to hold values
    double fX, fY, fZ, Energy;
    int fGlobalTime;

    // Connect branches
    chain->SetBranchAddress("fX", &fX);
    chain->SetBranchAddress("fY", &fY);
    chain->SetBranchAddress("fZ", &fZ);
    chain->SetBranchAddress("Energy", &Energy);
    chain->SetBranchAddress("fGlobalTime", &fGlobalTime);


    int nX = 100, nY = 100, nZ = 100;
    double pixelSizeX = 0.0364 , pixelSizeY = 0.0364;
    TH3D *h3 = new TH3D("h3", "3D Energy Map;X;Y;Z", nX, 0, pixelSizeX *1000, nY, 0, pixelSizeY *1000, nZ, -15, 15);
    TH2D *h2_fullChip = new TH2D("h2_fullChip", "h2_fullChip", 512, 50 - 1.86, 50 + 1.86, 512, 50 - 1.86, 50 + 1.86);

    Long64_t nEntries = chain->GetEntries();
    for (Long64_t i = 0; i < nEntries; ++i) 
    {
        chain->GetEntry(i);

        // Fold positions into 2x2 grid but convery to um (*1000)
        double xFolded = fmod(fX + 1e-6, pixelSizeX) * 1000; 
        double yFolded = fmod(fY + 1e-6, pixelSizeY) * 1000;
        double zFolded = (50 - fZ) * 1000;
        cout << "xFolded = " << fX << "; yFolded = " << fY << "; zFolded = " << fZ << '/n';

        if (Energy > 0)
            h3->Fill(xFolded, yFolded, zFolded, Energy);
            h2_fullChip->Fill(fX, fY, Energy);
    }
    auto h2 = h3->Project3D("xy");
    TCanvas *c1 = new TCanvas("c1", "XY Projection", 800, 600);
    h2->Draw("COLZ");
    TCanvas *c2 = new TCanvas("c2", "3D Energy Map", 800, 600);
    h3->SetTitle("3D Energy Deposition;X [mm];Y [mm];Z [mm]");
    h3->Draw();
    TCanvas *c3 = new TCanvas("c3", " ", 800, 600);
    h2_fullChip->Draw("COLZ");
    // Save output
    TFile outFile("energyMap3D.root", "RECREATE");
    h3->Write();
    outFile.Close();
    
}
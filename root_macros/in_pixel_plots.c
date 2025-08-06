#include <ROOT/RNTuple.hxx>
#include <TH3D.h>
#include <TFile.h>
#include <iostream>

void in_pixel_plots() {
    // Usage:
    // root
    // .L ~/Documents/Simu/Geant4/DECAL_REPO/root_macros/in_pixel_plots.c

    double threshold = 5000; // Threshold in electrons
    std::string inputPath = "/home/vlad/Documents/Simu/Geant4/DECAL_REPO/Results/local_0043/";

    //TFile *file = TFile::Open("/home/vlad/Documents/Simu/Geant4/DECAL/build/output0_t0.root");
    //TTree *tree = (TTree*)file->Get("EnDeposited");
    // Scale it up to include all threads
    TChain *chain = new TChain("TruthEnDeposited");

    for (int t = 0; t <= 5; ++t) {
        chain->Add(Form("%soutput0_t%d.root", inputPath.c_str() , t));
    }

    // Variables to hold values
    double fX, fY, fZ, Energy,leadingEnergy, leadingTime;
    int fGlobalTime, truthEventID;

    // Connect branches
    chain->SetBranchAddress("iEvent", &truthEventID);
    chain->SetBranchAddress("fX", &fX);
    chain->SetBranchAddress("fY", &fY);
    chain->SetBranchAddress("fZ", &fZ);
    chain->SetBranchAddress("Energy", &Energy);
    chain->SetBranchAddress("fGlobalTime", &fGlobalTime);
    //chain->SetBranchAddress("ClSize", &clSize);
    chain->SetBranchAddress("LeadingEnergy", &leadingEnergy);
    chain->SetBranchAddress("LeadingTime", &leadingTime);


    int nX = 100, nY = 100, nZ = 100;
    double pixelSizeX = 0.0364 , pixelSizeY = 0.0364;
    TH3D *h3 = new TH3D("h3", "3D Energy Map;X;Y;Z", nX, 0, pixelSizeX *1000, nY, 0, pixelSizeY *1000, nZ, -15, 15);
    TH2D *h2_fullChip = new TH2D("h2_fullChip", "h2_fullChip", 512, 50 - 18.6/2, 50 + 18.6/2, 512, 50 - 18.6/2, 50 + 18.6/2);
    TH2D *hInPixelClSize = new TH2D("InPixelClSize", "InPixelClSize", 32, 0, 36, 32, 0, 36);
    TH2D *hInPixelPass = new TH2D("InPixelHit", "InPixelHit", 32, 0, 36, 32, 0, 36);
    TH2D *hInPixelMatch = new TH2D("InPixelMatch", "InPixel Efficiency [\%]", 32, 0, 36, 32, 0, 36);
    TH2D *hInPixelTime = new TH2D("InPixelTime", "InPixelTime [ns]", 32, 0, 36, 32, 0, 36);
    //TH2F *hAvgClSize = (TH2F*) hInPixelClSize->Clone("hAvgClSize");
    ///hAvgClSize->SetTitle("Average Cluster Size");
    //TODO: Insure that no bins are 0. Could happen if low stats or fine binning


    TChain *chainPixel = new TChain("RawPixelHits");

    for (int t = 0; t <= 5; ++t) {
        chainPixel->Add(Form("%soutput0_t%d.root", inputPath.c_str() , t));
        //chainPixel->Add(Form("%soutput0_t0.root", inputPath.c_str()));
    }
    double corrEnergy, timeWalkHit;
    int rawEventID, iHit, pixX, pixY;
    chainPixel->SetBranchAddress("iEvent", &rawEventID);
    chainPixel->SetBranchAddress("iHit", &iHit);
    chainPixel->SetBranchAddress("PixX", &pixX);
    chainPixel->SetBranchAddress("PixY", &pixY);
    chainPixel->SetBranchAddress("Energy", &corrEnergy);
    chainPixel->SetBranchAddress("timeWalkHit", &timeWalkHit);
    Long64_t nRawEntries = chainPixel->GetEntries();
    std::map<std::pair<int, int>, double> enMap; // Avoid O(n^2) nested loops via extra map 
    std::map<int, std::vector<double>> timeMap;
    for (Long64_t j = 0; j < nRawEntries; j++)
    {
        chainPixel->GetEntry(j);
        //std::cout << "Raw Event ID: " << rawEventID << "; Hit: " << iHit << "; PixX: " << pixX << "; PixY: " << pixY << "; Energy: " << corrEnergy << std::endl;

        enMap[{rawEventID, iHit}] += corrEnergy; // Accumulate energy for each hit in the event.
        timeMap[rawEventID].push_back(timeWalkHit); // Store all times
    }
    std::map<int, int> clusterMap; // Map to hold cluster size per event
    std::map<int, double> clusterEnergyMap; // Map to hold leading energy per event
    std::map<int, double> clusterTimeMap; // Map to hold leading time per event
    //TODO: This makes sense for primary particles. It fails for delta rays. X, Y position should be extracted from VertexPosition and maybe deltas approached carefully
    for (const auto& entry : enMap) 
    {
        int eventID = entry.first.first;
        int hitID   = entry.first.second;
        double energy = entry.second;
        //std::cout << "Event ID: " << eventID << "; Hit: " << hitID << "; Energy: " << energy << std::endl;
        // Fill the cluster size map
        if (energy > threshold) 
        {
            clusterMap[eventID]++; // Increment cluster size for this event
            //clusterEnergyMap[eventID] = std::max(clusterEnergyMap[eventID], energy); // Store the maximum energy for this event
            clusterTimeMap[eventID] = *std::min_element(timeMap[eventID].begin(), timeMap[eventID].end());
        }
    }


    Long64_t nEntries = chain->GetEntries();
    for (Long64_t i = 0; i < nEntries; ++i) 
    {
        chain->GetEntry(i);
        //std::cout << "Event ID: " << truthEventID << "; X: " << fX << "; Y: " << fY << "; Z: " << fZ << "; Energy: " << Energy << std::endl;
        // Fold positions into 2x2 grid but convery to um (*1000)
        double xFolded = fmod(fX + 1e-6, pixelSizeX) * 1000; 
        double yFolded = fmod(fY + 1e-6, pixelSizeY) * 1000;
        double zFolded = (50 - fZ) * 1000;
        //cout << "xFolded = " << fX << "; yFolded = " << fY << "; zFolded = " << fZ << '/n';
        auto it = clusterMap.find(truthEventID);
        if (it != clusterMap.end()) 
        {
            hInPixelClSize->Fill(xFolded, yFolded, it->second);
            hInPixelMatch->Fill(xFolded, yFolded, it->second != 0 ? 1 : 0);
        }
        auto itTime = clusterTimeMap.find(truthEventID);
        if (itTime != clusterTimeMap.end())
        {
            hInPixelTime->Fill(xFolded, yFolded, itTime->second);
        }



        //hInPixelClSize->Fill(xFolded, yFolded, clSize); // Quick and dirty way to fill cluster size. No threshold implementation
        hInPixelPass->Fill(xFolded, yFolded, 1);
        if (Energy > 0)
        {
            h3->Fill(xFolded, yFolded, zFolded, Energy);
            h2_fullChip->Fill(fX, fY, Energy);
        }
        /*
        if (leadingEnergy > threshold)
        {
            hInPixelMatch->Fill(xFolded, yFolded, leadingEnergy);
            hInPixelTime->Fill(xFolded, yFolded, leadingTime);
        }
        */
    }


    hInPixelClSize->Divide(hInPixelPass);

    double weightedSum = 0.0;
    double totalWeight = 0.0;
    int nx = hInPixelClSize->GetNbinsX();
    int ny = hInPixelClSize->GetNbinsY();
    for (int ix = 1; ix <= nx; ++ix) {
        for (int iy = 1; iy <= ny; ++iy) {
            double content = hInPixelClSize->GetBinContent(ix, iy);
            if (content <= 0) continue; // skip empty or negative bins
            weightedSum += content * content; // weight by content
            totalWeight += content;
        }
    }

    double weightedMeanZ = (totalWeight > 0) ? weightedSum / totalWeight : 0.0;
    std::cout << "Average Cluster Size: " << weightedMeanZ << std::endl;



    hInPixelMatch->Divide(hInPixelPass);

    weightedSum = 0.0;
    totalWeight = 0.0;
    nx = hInPixelMatch->GetNbinsX();
    ny = hInPixelMatch->GetNbinsY();
    for (int ix = 1; ix <= nx; ++ix) {
        for (int iy = 1; iy <= ny; ++iy) {
            double content = hInPixelMatch->GetBinContent(ix, iy);
            if (content <= 0) continue; // skip empty or negative bins
            weightedSum += content * content; // weight by content
            totalWeight += content;
        }
    }

    weightedMeanZ = (totalWeight > 0) ? weightedSum / totalWeight : 0.0;
    std::cout << "Average eff.: " << weightedMeanZ << std::endl;



    hInPixelTime->Divide(hInPixelPass);
    auto h2 = h3->Project3D("xy");
    TCanvas *c1 = new TCanvas("c1", "XY Projection", 800, 600);
    h2->Draw("COLZ");
    TCanvas *c2 = new TCanvas("c2", "3D Energy Map", 800, 600);
    h3->SetTitle("3D Energy Deposition;X [mm];Y [mm];Z [mm]");
    h3->Draw();
    TCanvas *c3 = new TCanvas("c3", " ", 800, 600);
    h2_fullChip->Draw("COLZ");
    TCanvas *c4 = new TCanvas("c4", " ", 800, 600);
    gStyle->SetOptStat(0);
    hInPixelClSize->SetMinimum(1);
    hInPixelClSize->SetMaximum(4);
    hInPixelClSize->Draw("COLZ");
    TCanvas *c5 = new TCanvas("c5", " ", 800, 600);
    gStyle->SetOptStat(0);
    hInPixelMatch->Draw("COLZ");
    TCanvas *c6 = new TCanvas("c6", " ", 800, 600);
    gStyle->SetOptStat(0);
    hInPixelTime->Draw("COLZ");
    // Here i plot manually average cluster size. TODO: automate this?
    TCanvas *c7 = new TCanvas("c7", " ", 800, 600);
    double thr[] = {100,150,200,250,300,350,400,450,500,550,600,650,700,750,800,1000,1300,1600,1900,2000,2100,2200,2300,2400};
    double avgClSize[] = {2.38,2.25,2.15,2.07,2.,1.94,1.88,1.83,1.78,1.74,1.7,1.66,1.62,1.59,1.55,1.4,1.26,1.13,1,0.96,0.92,0.88,0.84,0.8};
    TGraph *g = new TGraph(24, thr, avgClSize);
    g->SetTitle("Thr vs <cl.size>; Threshold [e-]; <cl.size>"); // title and axis labels
    g->SetMarkerStyle(20); // filled circle
    g->SetMarkerSize(1.2);
    g->SetMarkerColor(kBlack);
    g->Draw("AP");



    TCanvas *c8 = new TCanvas("c8", " ", 800, 600);
    double thr2[] = {200,400,800,900,1000,1200,1300,1400,1500,1600,1800,1900,2000,2100,2200,2300,2400,2800,3000,3400,3800,4000,4400,5000};
    double eff[] = {100,99.99,99.15,98.45,97.42,94.59,92.71,90.52,88.02,85.27,79.,75.14,71.51,67.79,64.08,60.46,56.96,45,40.45,33.51,28.64,26.74,23.81,20.59};
    TGraph *thrEff = new TGraph(24, thr2, eff);
    thrEff->SetTitle("Thr vs eff; Threshold [e-]; eff [%]"); // title and axis labels
    thrEff->SetMarkerStyle(20); // filled circle
    thrEff->SetMarkerSize(1.2);
    thrEff->SetMarkerColor(kBlack);
    thrEff->Draw("AP");

    // Save output
    TFile outFile("energyMap3D.root", "RECREATE");
    h3->Write();
    outFile.Close();






    
}
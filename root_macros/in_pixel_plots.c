#include <ROOT/RNTuple.hxx>
#include <TH3D.h>
#include <TFile.h>
#include <iostream>

double GetTimingOffset(double amplitude) 
{
    // assumes 1200e- correspond to 350 mV. Check calibration through injection scans.
    //return 40. * std::exp(amplitude /(-191.));
    if (amplitude < 100.) 
    { // delay only down to amplitudes of 100e-. 
        return 3230. /pow(100-67.0, 1.08);
    }

    return 3230. /pow(amplitude-67.0, 1.08);

}

void in_pixel_plots() {
    // Usage:
    // root
    // .L ~/Documents/Simu/Geant4/DECAL_REPO/root_macros/in_pixel_plots.c

    double threshold = 400; // Threshold in electrons
    std::string inputPath = "/home/vlad/Documents/Simu/Geant4/DECAL_REPO/Results/local_0044/";

    //TFile *file = TFile::Open("/home/vlad/Documents/Simu/Geant4/DECAL/build/output0_t0.root");
    //TTree *tree = (TTree*)file->Get("EnDeposited");
    // Scale it up to include all threads
    TChain *chain = new TChain("TruthEnDeposited");

    for (int t = 0; t <= 5; ++t) {
        chain->Add(Form("%soutput0_t%d.root", inputPath.c_str() , t));
    }

    // Variables to hold values
    double fX, fY, fZ, vertexX, vertexY, vertexZ, Energy,leadingEnergy, leadingTime;
    int fGlobalTime, truthEventID;

    // Connect branches
    chain->SetBranchAddress("iEvent", &truthEventID);
    
    
    chain->SetBranchAddress("fX", &fX);
    chain->SetBranchAddress("fY", &fY);
    chain->SetBranchAddress("fZ", &fZ);
    chain->SetBranchAddress("vertexX", &vertexX);
    chain->SetBranchAddress("vertexY", &vertexY);
    chain->SetBranchAddress("vertexZ", &vertexZ);
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
    //TH2D *hInPixelClSize = new TH2D("InPixelClSize", "InPixelClSize", 100, 0, 36, 100, 0, 36);
    TH2D *hInPixelPass = new TH2D("InPixelHit", "InPixelHit", 32, 0, 36, 32, 0, 36);
    //TH2D *hInPixelPass = new TH2D("InPixelHit", "InPixelHit", 100, 0, 36, 100, 0, 36);
    TH2D *hInPixelMatch = new TH2D("InPixelMatch", "InPixel Efficiency [\%]", 32, 0, 36, 32, 0, 36);
    //TH2D *hInPixelMatch = new TH2D("InPixelMatch", "InPixel Efficiency [\%]", 100, 0, 36, 100, 0, 36);
    TH2D *hInPixelTime = new TH2D("InPixelTime", "InPixelTime [ns]", 32, 0, 36, 32, 0, 36);
    //TH2D *hInPixelTime = new TH2D("InPixelTime", "InPixelTime [ns]", 100, 0, 36, 100, 0, 36);
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
    //std::map<std::pair<int, int>, double> enMap; // Avoid O(n^2) nested loops via extra map 
    std::map<std::pair<int,std::pair<int, int>>, double> enMap; // Avoid O(n^2) nested loops via extra map 
    std::map<int, std::vector<double>> timeMap;
    int eventIDHolder =0;
    int pixXHolder = -1;
    int pixYHolder = -1;
    for (Long64_t j = 0; j < nRawEntries; j++)
    {
        chainPixel->GetEntry(j);
        // This code snippet looks for events that have hits such that the seed crystal changes within the same event => delta like ray
        /*
        if (eventIDHolder == rawEventID && iHit == 0 && pixXHolder != pixX && pixYHolder != pixY)
        {
            std::cout << "Raw Event ID: " << rawEventID << "; Hit: " << iHit << "; PixX: " << pixX << "; PixY: " << pixY << "; Energy: " << corrEnergy << std::endl;
        }

        if (iHit == 0)
        {
            pixXHolder = pixX;
            pixYHolder = pixY;
        }
        */
        // This is an interesting event in run 43. If anyone is curious
        //if(rawEventID == 352902) std::cout << "Raw Event ID: " << rawEventID << "; Hit: " << iHit << "; PixX: " << pixX << "; PixY: " << pixY << "; Energy: " << corrEnergy << std::endl;

        //enMap[{rawEventID, iHit}] += corrEnergy; // Accumulate energy for each hit in the event.
        enMap[{rawEventID, {pixX, pixY}}] += corrEnergy;
        // Deprecated will require to be purged
        timeMap[rawEventID].push_back(timeWalkHit); // Store all times
        eventIDHolder = rawEventID;

    }
    std::map<int, int> clusterMap; // Map to hold cluster size per event
    std::map<int, double> clusterEnergyMap; // Map to hold leading energy per event
    std::map<int, double> clusterTimeMap; // Map to hold leading time per event
    //TODO: This makes sense for primary particles. It fails for delta rays. X, Y position should be extracted from VertexPosition and maybe deltas approached carefully
    int simpleCounter = 0;
    int rememberer =0;
    std::map<int, std::vector<double>> inClusterTimes;
    for (const auto& entry : enMap) 
    {
        int eventID = entry.first.first;
        //int hitID   = entry.first.second;
        double energy = entry.second;
        inClusterTimes[eventID].push_back(GetTimingOffset(energy));
        //std::cout << GetTimingOffset(energy) << std::endl;
        // Fill the cluster size map
        if (energy > threshold) 
        {
            clusterMap[eventID]++; // Increment cluster size for this event
            //clusterEnergyMap[eventID] = std::max(clusterEnergyMap[eventID], energy); // Store the maximum energy for this event
            //I take the fasest hit in an event (cluster) as MALTA does.
            clusterTimeMap[eventID] = *std::min_element(inClusterTimes[eventID].begin(), inClusterTimes[eventID].end());
            //std::cout << "eventID: " << eventID << "timing: " <<  clusterTimeMap[eventID] << std::endl;
        }
        if (eventID == rememberer) 
        {
            simpleCounter++;
        }
        // If we change eventID reset the vector so we dont do exceedingly dumb things
        else
        {
            inClusterTimes.clear();
        }
        rememberer = eventID;
        //if (simpleCounter > 4) // This line looks for eventIDs that correspond to clusters larger than 4
        if (eventID == 27437)
        {
            std::cout << "Event ID: " << eventID << "; pixX: " << entry.first.second.first << ";pixY: " << entry.first.second.second << "; Energy: " << energy << std::endl;
        }

    }

    int previousID =  0;
    Long64_t nEntries = chain->GetEntries();
    for (Long64_t i = 0; i < nEntries; ++i) 
    {
        chain->GetEntry(i);
        // Do me once per event
        if (truthEventID == previousID) continue;
        previousID = truthEventID;
        //std::cout << "Event ID: " << truthEventID << "; X: " << fX << "; Y: " << fY << "; Z: " << fZ << "; Energy: " << Energy << std::endl;
        // Fold positions into 2x2 grid but convert to um (*1000)
        double xFolded = fmod(vertexX + 1e-6, pixelSizeX) * 1000; 
        double yFolded = fmod(vertexY + 1e-6, pixelSizeY) * 1000;
        double zFolded = (50 - vertexZ) * 1000;
        //cout << "xFolded = " << fX << "; yFolded = " << fY << "; zFolded = " << fZ << '/n';
        auto it = clusterMap.find(truthEventID);
        //std::cout << "Event ID: " << truthEventID << "; X: " << vertexX << "; Y: " << vertexY << "En.: " << it->second << std::endl;  
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
    hInPixelClSize->SetMaximum(7);
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

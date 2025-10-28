#include <ROOT/RNTuple.hxx>
#include <TH3D.h>
#include <TFile.h>
#include <iostream>
#include <math.h>

// -------------- How to run: 

// // for 2DEff 1400e- threshold plots:
// specify threshold = 1400e- in threshold_loop()
// .L in_pixel_plots.c
// threshold_loop("/Users/lucianfasselt/DECAL/Simulation/Geant4/MALTASIM/malta_simulation/Results/local_0020/", "test_1400e-.root")

// -------------- todo:
// 1) clearly name "Energy", "corrEnergy", "Charge" and "corrCharge" (rename equivalently in RunAction.cc)
// 2) Get eff. curve for uncorrected energy and compare models with data. (to show that CC-model makes more sense)

// this function is derived from data at threshold = 150e-.
// It assumes that the front-end is readjusted for each threshold change.
// Assumptions: 
// 1) Time-walk for DeltaT vs. amplitude in volts is the same for each threshold
// 2) calibration from volts to charge is linear with threshold. (larger threshold --> larger charge for same voltage amplitude.)
// an upper limit of 200. is defined for amplitudes that fall below threshold. (Could also be set to inf or 0 ?)
double GetTimingOffset(double amplitude, double threshold) 
{
    if (amplitude < threshold) // if less than than threshold 
    {
        return 200.; // set to 200 ns delay if less than threshold (function diverges at threshold)
    }
    return 390.0 / pow((amplitude * 150./threshold) - 149.8, 0.65);
}

// Efficiency in percent
double getEff(int Npassed, int Nall) {
    if (Nall == 0) return 0.0;  // avoid division by zero
    return ((double)Npassed / (double)Nall) * 100.0;
}

// Error in percent, assuming binomial distribution
double getEffErr(int Npassed, int Nall) {
    if (Nall == 0) return 0.0;  // avoid division by zero
    double ratio = (double)Npassed / (double)Nall;
    return 100.0 * sqrt(ratio * (1.0 - ratio) / (double)Nall);
}

void set_style() {
    gStyle->SetOptStat(0);
    gStyle->SetPalette(112);
    gStyle->SetNumberContours(255);
    // gStyle->SetPalette(1); // old default rainbow palette, optional
    gROOT->SetBatch(kTRUE);
}

// result stores [average cluster size, av. efficiency, error on av. eff]
// Threshold in electrons
void in_pixel_plots(std::string inputPath, std::string outROOTname, double result[3], double threshold = 1000) {
    // // Usage:
    // // create subdirectory and run from there (pdf plots will be created in it)
    // root
    // .L ~/Documents/Simu/Geant4/DECAL_REPO/root_macros/in_pixel_plots.c
    // // specify threshold variable in threshold_loop(). Then run:
    // threshold_loop()

    std::cout << "Threshold: " << threshold << std::endl;

    // Create a random number generator with a seed
    TRandom3 rng(0);  // 0 = use machine clock for seed
    double trackunc_X = 4.6/1000.; // tracking uncertainty in X in unit mm
    double trackunc_Y = 4.6/1000.; // tracking uncertainty in X in unit mm

    //std::string inputPath = "/home/vlad/Documents/Simu/Geant4/DECAL_REPO/Results/local_0044/";
    //std::string inputPath = "/Users/lucianfasselt/DECAL/Simulation/Geant4/MALTASIM/malta_simulation/Results/local_0014/"; // reference before
    //std::string inputPath = "/Users/lucianfasselt/DECAL/Simulation/Geant4/MALTASIM/malta_simulation/Results/local_0020/"; // vary 20-28

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

    int nX = 2*16, nY = 2*16, nZ = 100;
    double pixelSizeX = 0.0364 , pixelSizeY = 0.0364; // in mm
    TH3D *h3 = new TH3D(Form("h3_%.0fThr", threshold), "3D Energy Map;Track X pos [#mum];Track Y pos [#mum];Track Z pos [#mum]", 100, 0, pixelSizeX *1000, 100, 0, pixelSizeY *1000, 100, -15, 15);
    TH2D *h2_fullChip = new TH2D(Form("h2_fullChip_%.0fThr", threshold), "h2_fullChip", 512, 50 - 18.6/2, 50 + 18.6/2, 512, 50 - 18.6/2, 50 + 18.6/2);
    TH2D *hInPixelClSize = new TH2D(Form("InPixelClSize_%.0fThr", threshold), "InPixelClSize", nX, 0, 2*pixelSizeX *1000, nY, 0, 2*pixelSizeY *1000);
    hInPixelClSize->SetTitle(";X [#mum]; Y [#mum]; cluster size");
    //TH2D *hInPixelClSize = new TH2D("InPixelClSize", "InPixelClSize", 100, 0, 36, 100, 0, 36);
    TH2D *hInPixelPass = new TH2D(Form(";InPixelHit_%.0fThr", threshold), "InPixelHit", nX, 0, 2*pixelSizeX*1000, nY, 0, 2*pixelSizeY *1000);
    hInPixelPass->SetTitle(";X [#mum]; Y [#mum]; Passed events");
    //TH2D *hInPixelPass = new TH2D("InPixelHit", "InPixelHit", 100, 0, 36, 100, 0, 36);
    TH2D *hInPixelMatch = new TH2D(Form("InPixelMatch_%.0fThr", threshold), "InPixel Efficiency [\%]", nX, 0, 2*pixelSizeX *1000, nY, 0, 2*pixelSizeY *1000);
    hInPixelMatch->SetTitle(";X [#mum]; Y [#mum]; Efficiency [\%]");
    //TH2D *hInPixelMatch = new TH2D("InPixelMatch", "InPixel Efficiency [\%]", 100, 0, 36, 100, 0, 36);
    TH2D *hInPixelTime = new TH2D(Form("InPixelTime_%.0fThr", threshold), "InPixelTime [ns]", nX, 0, 2*pixelSizeX *1000, nY, 0, 2*pixelSizeY *1000);
    hInPixelTime->SetTitle(";X [#mum]; Y [#mum]; InPixelTime [ns]");
    //TH2D *hInPixelTime = new TH2D("InPixelTime", "InPixelTime [ns]", 100, 0, pixelSizeX *1000, 100, 0, 36);
    //TH2F *hAvgClSize = (TH2F*) hInPixelClSize->Clone("hAvgClSize");
    ///hAvgClSize->SetTitle("Average Cluster Size");
    //TODO: Insure that no bins are 0. Could happen if low stats or fine binning

    // histograms with tracking uncertainty:
    TH2D *hInPixelPass_trackunc = new TH2D(Form("InPixelHit_trackunc_%.0fThr", threshold), "InPixelHit_trackunc", nX, 0, 2*pixelSizeX*1000, nY, 0, 2*pixelSizeY *1000);
    hInPixelPass_trackunc->SetTitle(";Track X pos [#mum];Track Y pos [#mum]; Passed tracks");

    TH2D *hInPixelMatch_trackunc = new TH2D(Form("InPixelMatch_trackunc_%.0fThr", threshold), "InPixel Efficiency_trackunc  [\%]", nX, 0, 2*pixelSizeX *1000, nY, 0, 2*pixelSizeY *1000);
    hInPixelMatch_trackunc->SetTitle(";Track X pos [#mum]; Track Y pos [#mum]; Efficiency [\%]");

    TH2D *hInPixelClSize_trackunc = new TH2D(Form("InPixelClSize_trackunc_%.0fThr", threshold), "InPixelClSize_trackunc", nX, 0, 2*pixelSizeX *1000, nY, 0, 2*pixelSizeY *1000);
    hInPixelClSize_trackunc->SetTitle(";Track X pos [#mum];Track Y pos [#mum]; cluster size");

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
    chainPixel->SetBranchAddress("Energy", &corrEnergy); // should probably be renamed from "Energy" to "corrCharge" (same in RunAction.cc)
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
        double cenergy = entry.second; // corrected energy
        inClusterTimes[eventID].push_back(GetTimingOffset(cenergy, threshold));
        //std::cout << GetTimingOffset(cenergy) << std::endl;
        // Fill the cluster size map
        if (cenergy > threshold) 
        {
            clusterMap[eventID]++; // Increment cluster size for this event
            //clusterEnergyMap[eventID] = std::max(clusterEnergyMap[eventID], cenergy); // Store the maximum correnergy for this event
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
        if (simpleCounter > 4) std::cout << "Counter: " << simpleCounter << std::endl;
        if (eventID == 27437)
        {
            std::cout << "Event ID: " << eventID << "; pixX: " << entry.first.second.first << ";pixY: " << entry.first.second.second << "; corrEnergy: " << cenergy << std::endl;
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
        double xFolded = fmod(vertexX, 2*pixelSizeX) * 1000; 
        double yFolded = fmod(vertexY, 2*pixelSizeY) * 1000;
        double zFolded = (50 - vertexZ) * 1000;
        //cout << "xFolded = " << fX << "; yFolded = " << fY << "; zFolded = " << fZ << '/n';

        double xIP_track = fmod(vertexX + rng.Gaus(0., trackunc_X), 2*pixelSizeX) * 1000; 
        double yIP_track = fmod(vertexY + rng.Gaus(0., trackunc_Y), 2*pixelSizeX) * 1000; 

        auto it = clusterMap.find(truthEventID);
        //std::cout << "Event ID: " << truthEventID << "; X: " << vertexX << "; Y: " << vertexY << "En.: " << it->second << std::endl;  
        if (it != clusterMap.end()) 
        {
            hInPixelClSize->Fill(xFolded, yFolded, it->second);
            hInPixelMatch->Fill(xFolded, yFolded, it->second != 0 ? 1 : 0);

            hInPixelClSize_trackunc->Fill(xIP_track, yIP_track, it->second);
            hInPixelMatch_trackunc->Fill(xIP_track, yIP_track, it->second != 0 ? 1 : 0);
        }
        auto itTime = clusterTimeMap.find(truthEventID);
        if (itTime != clusterTimeMap.end())
        {
            hInPixelTime->Fill(xFolded, yFolded, itTime->second);
        }

        //hInPixelClSize->Fill(xFolded, yFolded, clSize); // Quick and dirty way to fill cluster size. No threshold implementation
        hInPixelPass->Fill(xFolded, yFolded, 1);
        hInPixelPass_trackunc->Fill(xIP_track, yIP_track, 1);
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

    result[0] = hInPixelClSize_trackunc->Integral() / hInPixelMatch_trackunc->Integral();
    std::cout << "Av. Cl size: " << result[0] << std::endl;

    hInPixelClSize->Divide(hInPixelMatch);
    hInPixelClSize_trackunc->Divide(hInPixelMatch_trackunc);

    result[1] = getEff(hInPixelMatch_trackunc->Integral(), hInPixelPass_trackunc->Integral());// in percent
    result[2] = getEffErr(hInPixelMatch_trackunc->Integral(), hInPixelPass_trackunc->Integral());// in percent

    std::cout << "Matched tracks: " << hInPixelMatch->Integral() << std::endl;
    std::cout << "All tracks: " << hInPixelPass->Integral() << std::endl;
    std::cout << "Av. Eff.: " << result[1] << "+- " 
                << result[2] << std::endl;

    hInPixelMatch->Divide(hInPixelPass);
    hInPixelMatch_trackunc->Divide(hInPixelPass_trackunc);
    hInPixelMatch->Scale(100.); // in percent
    hInPixelMatch_trackunc->Scale(100.); // in percent

    
    hInPixelTime->Divide(hInPixelPass);
    auto h2 = h3->Project3D("xy");
    TCanvas *c1 = new TCanvas("c1", "XY Projection", 800, 600);
    h2->Draw("COLZ");
    //c1->SaveAs(Form("1DEdep_%.0fThr.pdf", threshold));
    c1->Close();

    TCanvas *c2 = new TCanvas("c2", "3D Energy Map", 800, 600);
    h3->SetTitle("3D Energy Deposition;X [#mum];Y [mm];Z [#mum]");
    h3->Draw();
    //c2->SaveAs(Form("3DEdep_%.0fThr.pdf", threshold));
    c2->Close();

    TCanvas *c3 = new TCanvas("c3", " ", 800, 600);
    h2_fullChip->Draw("COLZ");
    //c3->SaveAs(Form("FullChip_%.0fThr.pdf", threshold));
    c3->Close();

    set_style();
    TCanvas *c2D = new TCanvas("c2D", "2DInPix ", 800, 800); // use same for all Inpixplots
    gStyle->SetOptStat(0);
    c2D->SetTicks(1, 1);  // 1 = draw ticks on top/right as well
    c2D->SetRightMargin(0.16); // define margins for all IP plots
    c2D->SetBottomMargin(0.1);
    c2D->SetLeftMargin(0.1);
    c2D->SetTopMargin(0.16);
    // crossing lines to mark 2x2 pixels
    TLine *LinI1 = new TLine(0., 36.4, 72.8, 36.4);
    LinI1->SetLineColor(1);
    LinI1->SetLineWidth(2);
    TLine *LinI2 = new TLine(36.4, 0., 36.4, 72.8);
    LinI2->SetLineColor(1);
    LinI2->SetLineWidth(2);

    // IP CLSIZE plot
    hInPixelClSize->SetMinimum(1);
    //hInPixelClSize->SetMaximum(4);
    hInPixelClSize->Draw("COLZ");
    hInPixelClSize->GetZaxis()->SetTitleOffset(1.5);
    LinI1->Draw("SAMEL");
    LinI2->Draw("SAMEL");
    c2D->SaveAs(Form("2DIPClSize_%.0fThr.pdf", threshold));

    // IP Eff plot
    hInPixelMatch->Draw("COLZ");
    hInPixelMatch->SetMinimum(0.);
    hInPixelMatch->SetMaximum(100.);
    // Create TLatex object
    TLatex *t = new TLatex();
    // Draw the formatted text
    //t->DrawLatex(pixelSizeX/2.*1000., pixelSizeY*2*1000. +2, Form("<eff> = %.2f #pm %.2f %%", result[1], result[2]));
    t->DrawLatex(-2., pixelSizeY*2*1000. +2, Form("MALTA2 Sim.#bf{, 30#mum EPI, <eff> =%.1f%%}", result[1]));
    hInPixelMatch->GetZaxis()->SetTitleOffset(1.5);
    LinI1->Draw("SAMEL");
    LinI2->Draw("SAMEL");
    c2D->SaveAs(Form("2DIPMatch_%.0fThr.pdf", threshold));
    c2D->SaveAs(Form("2DIPMatch_%.0fThr.C", threshold));

    // IP TIMING plot
    hInPixelTime->Draw("COLZ");
    hInPixelTime->GetZaxis()->SetTitleOffset(1.5);
    LinI1->Draw("SAMEL");
    LinI2->Draw("SAMEL");
    c2D->SaveAs(Form("2DIPTime_%.0fThr.pdf", threshold));

    // IP CLSIZE plot with track unc
    hInPixelClSize_trackunc->SetMinimum(1);
    //hInPixelClSize_trackunc->SetMaximum(4);
    hInPixelClSize_trackunc->Draw("COLZ");
    hInPixelClSize_trackunc->GetZaxis()->SetTitleOffset(1.5);
    LinI1->Draw("SAMEL");
    LinI2->Draw("SAMEL");
    t->DrawLatex(-2., pixelSizeY*2*1000. +2, Form("MALTA2 Sim.#bf{, 30#mum EPI, <cl. size> =%.2f}", result[0]));
    c2D->SaveAs(Form("2DIPClSize_trackunc_%.0fThr.pdf", threshold));

    // IP EFF plot with track unc
    hInPixelMatch_trackunc->Draw("COLZ");
    hInPixelMatch_trackunc->SetMinimum(0.);
    hInPixelMatch_trackunc->SetMaximum(100.);
    hInPixelMatch_trackunc->GetZaxis()->SetTitleOffset(1.5);
    LinI1->Draw("SAMEL");
    LinI2->Draw("SAMEL");
    //t->DrawLatex(pixelSizeX/2.*1000., pixelSizeY*2*1000. +2, Form("<eff> = %.2f #pm %.2f %%", result[1], result[2]));
    t->DrawLatex(-2., pixelSizeY*2*1000. +2, Form("MALTA2 Sim.#bf{, 30#mum EPI, <eff> =%.1f%%}", result[1]));
    c2D->SaveAs(Form("2DIPEff_trackunc_%.0fThr.pdf", threshold));
    c2D->SaveAs(Form("2DIPEff_trackunc_%.0fThr.C", threshold));
    c2D->Close();

    // Save output
    TFile outFile(outROOTname.c_str(), "UPDATE");
    //h3->Write();
    //h2_fullChip->Write();
    hInPixelClSize_trackunc->Write();
    hInPixelMatch_trackunc->Write();
    outFile.Close();

}

// specify inputFile = "/Users/lucianfasselt/DECAL/Simulation/Geant4/MALTASIM/malta_simulation/Results/local_0020/"
// specify outROOT = "SimOutput_20.root"
// or use intLoop
int threshold_loop(std::string inputFile, std::string outROOT){
    // List of threshold values:
    //double thresholds[] = {200., 300., 400., 500., 600., 700., 800., 900., 1000., 1200., 1400., 1600., 1800., 2000., 2200., 2400., 2600., 2800., 3000.};
    double thresholds[] = {400, 2000.};
    //double thresholds[] = {200, 230, 343, 448, 544, 632, 712}; // equivalent to thresholds of data points

    int num_values = sizeof(thresholds) / sizeof(thresholds[0]);
    double results[num_values][3];

    // Create two TGraphs
    TGraphErrors *gr_AvClSize = new TGraphErrors(num_values);
    gr_AvClSize->SetName("AverageClSize");
    TGraphErrors *gr_AvEff = new TGraphErrors(num_values);
    gr_AvEff->SetName("AverageEff");

    TFile outFile(outROOT.c_str(), "RECREATE"); // create new file and fill in in_pixel_plots
    outFile.Close();

    // Get average cl. size and eff. for each threshold
    for (int i = 0; i < num_values; i++) {
        in_pixel_plots(inputFile, outROOT, results[i], thresholds[i]);
        gr_AvClSize->SetPoint(i, thresholds[i], results[i][0]);
        gr_AvEff->SetPoint(i, thresholds[i], results[i][1]);
        gr_AvEff->SetPointError(i, 0.0, results[i][2]); // assume no unc. on threshold
    }

    // Draw graphs
    TCanvas *c1 = new TCanvas("c1", "ClSize_versus_Thr", 800, 600);
    gr_AvClSize->SetMarkerStyle(20);
    //gr_AvClSize->SetMarkerColor(kBlue);
    gr_AvClSize->SetTitle(";Threshold [e-]; <cluster size>");
    gr_AvClSize->Draw("AP");
    c1->SaveAs("AvClusterSize.pdf");
    c1->Close();

    TCanvas *c2 = new TCanvas("c2", "Eff_versus_Thr", 800, 600);
    gr_AvEff->SetMarkerStyle(21);
    gr_AvEff->SetTitle(";Threshold [e-]; Efficiency [\%]");
    gr_AvEff->Draw("AP");

    c2->SaveAs("AvEff.pdf");
    c2->Close();

    TFile outFile2(outROOT.c_str(), "UPDATE"); // open again and fill with summary plots
    gr_AvClSize->Write();
    gr_AvEff->Write();
    outFile2.Close();

    // Print results
    for (int i = 0; i < num_values; i++) {
        printf("Thr %.f e-: <Cl. size>: %f, <Eff>: %f +- %f\n",
               thresholds[i], results[i][0], results[i][1], results[i][2]);
    }
    return 0;
}

void RunInt_loop(){
    for (int runNumber = 43; runNumber <= 45; ++runNumber) {
        std::string inputFileName = "/Users/lucianfasselt/DECAL/Simulation/Geant4/MALTASIM/malta_simulation/Results/local_00"+ std::to_string(runNumber)+"/";  
        std::string outROOTName = "SimOutput_" + std::to_string(runNumber) + ".root";
        threshold_loop(inputFileName, outROOTName.c_str());
    }
}
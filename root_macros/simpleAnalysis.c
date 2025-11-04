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

inline std::pair<double,double> PixelPositionReconstruction(int pixelX, int pixelY)
{
    double pixelSize = 0.0364; // in mm
    double detectorXOffset = 50.; // in mm
    double detectorYOffset = 50.; // in mm
    double detectorSizeX = 1.86368*10; // in mm
    double detectorSizeY = 1.86368*10; // in mm

    double xGlobal = pixelX * pixelSize + detectorXOffset - detectorSizeX / 2;
    double yGlobal = pixelY * pixelSize + detectorYOffset - detectorSizeY / 2;
    return {xGlobal, yGlobal};
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
void simpleAnalysis(std::string inputPath, std::string outROOTname, double result[3], double threshold = 1000) {
    // // Usage:
    // // create subdirectory and run from there (pdf plots will be created in it)
    // root
    // .L ~/Documents/Simu/Geant4/DECAL_REPO/root_macros/simpleAnalysis.c
    // // specify threshold variable in threshold_loop(). Then run:
    // threshold_loop()

    std::cout << "Threshold: " << threshold << std::endl;

    // Create a random number generator with a seed
    TRandom3 rng(0);  // 0 = use machine clock for seed
    double trackunc_X = 4.6/1000.; // tracking uncertainty in X in unit mm
    double trackunc_Y = 4.6/1000.; // tracking uncertainty in X in unit mm

    //std::string inputPath = "/home/vlad/Documents/Simu/Geant4/DECAL_REPO/Results/local_0044/";
    //std::string inputPath = "/Users/lucianfasselt/DECAL/Simulation/Geant4/MALTASIM/malta_simulation/Results/local_0046/"; // reference before

    TChain *MCTruthchain = new TChain("TruthVertex");

    for (int t = 0; t <= 5; ++t) {
        MCTruthchain->Add(Form("%soutput0_t%d.root", inputPath.c_str() , t));
    }

    // Variables to hold values
    double MCtrackX, MCtrackY, MCtrackZ, MCtrackGlobalTime;
    int MCtrackEventID;

    // Connect branches
    MCTruthchain->SetBranchAddress("iEvent", &MCtrackEventID);
    bool oldtree = false; // only set to true for old simulation output ( before November 2025)
    if (oldtree){
        MCTruthchain->SetBranchAddress("vertexX", &MCtrackX); // old naming of branch
        MCTruthchain->SetBranchAddress("vertexY", &MCtrackY);
        MCTruthchain->SetBranchAddress("vertexZ", &MCtrackZ);
        MCTruthchain->SetBranchAddress("fGlobalTime", &MCtrackGlobalTime);
    }
    else {
        MCTruthchain->SetBranchAddress("trueVertexX", &MCtrackX);
        MCTruthchain->SetBranchAddress("trueVertexY", &MCtrackY);
        MCTruthchain->SetBranchAddress("trueVertexZ", &MCtrackZ);
        MCTruthchain->SetBranchAddress("trueGlobalTime", &MCtrackGlobalTime);
    }
    Long64_t nEntriesMCTruth = MCTruthchain->GetEntries();
    std::cout << "Number of Events from MC track info: " << nEntriesMCTruth << std::endl;

    // Lookup map: iEvent → (vertexX, vertexY)
    std::unordered_map<int, std::pair<double, double>> vertexMap;
    vertexMap.reserve(nEntriesMCTruth / 2); // heuristic for fewer rehashes

    for (Long64_t i = 0; i < nEntriesMCTruth; ++i) {
        MCTruthchain->GetEntry(i);
        // Only insert the first time we encounter iEvent
        if (vertexMap.find(MCtrackEventID) == vertexMap.end()) {
            vertexMap[MCtrackEventID] = {MCtrackX, MCtrackY};
        } else {
            // Optional consistency check
            auto [vx, vy] = vertexMap[MCtrackEventID];
            std::cout << "("<< vx << ", " << vy << ")"<< std::endl;
            std::cout << "("<< MCtrackX << ", " << MCtrackY << ")"<< std::endl;
            std::cerr << "Warning: Vertex already exists for MCtrackEventID "
                        << MCtrackEventID << "!\n";
        }
    }
    std::cout << "Stored " << vertexMap.size() << " unique events.\n";

    int nX = 2*16, nY = 2*16, nZ = 100;
    double pixelSizeX = 0.0364 , pixelSizeY = 0.0364; // in mm
    //TH3D *h3 = new TH3D(Form("h3_%.0fThr", threshold), "3D Energy Map;Track X pos [#mum];Track Y pos [#mum];Track Z pos [#mum]", 100, 0, pixelSizeX *1000, 100, 0, pixelSizeY *1000, 100, -15, 15);
    //TH2D *h2_fullChip = new TH2D(Form("h2_fullChip_%.0fThr", threshold), "h2_fullChip", 512, 50 - 18.6/2, 50 + 18.6/2, 512, 50 - 18.6/2, 50 + 18.6/2);
    TH2D *hInPixelClSize = new TH2D(Form("InPixelClSize_%.0fThr", threshold), "InPixelClSize", nX, 0, 2*pixelSizeX *1000, nY, 0, 2*pixelSizeY *1000);
    hInPixelClSize->SetTitle(";X [#mum]; Y [#mum]; cluster size");
    TH2D *hInPixelAll = new TH2D(Form(";InPixelHit_%.0fThr", threshold), "InPixelHit", nX, 0, 2*pixelSizeX*1000, nY, 0, 2*pixelSizeY *1000);
    hInPixelAll->SetTitle(";X [#mum]; Y [#mum]; All events");
    TH2D *hInPixelMatch = new TH2D(Form("InPixelMatch_%.0fThr", threshold), "InPixel Efficiency [\%]", nX, 0, 2*pixelSizeX *1000, nY, 0, 2*pixelSizeY *1000);
    hInPixelMatch->SetTitle(";X [#mum]; Y [#mum]; Efficiency [\%]");
    TH2D *hInPixelTime = new TH2D(Form("InPixelTime_%.0fThr", threshold), "InPixelTime [ns]", nX, 0, 2*pixelSizeX *1000, nY, 0, 2*pixelSizeY *1000);
    hInPixelTime->SetTitle(";X [#mum]; Y [#mum]; InPixelTime [ns]");

    // histograms with tracking uncertainty:
    TH2D *hInPixelAll_trackunc = new TH2D(Form("InPixelHit_trackunc_%.0fThr", threshold), "InPixelHit_trackunc", nX, 0, 2*pixelSizeX*1000, nY, 0, 2*pixelSizeY *1000);
    hInPixelAll_trackunc->SetTitle(";Track X pos [#mum];Track Y pos [#mum]; All tracks");
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
    int rawEventID, iHit, pixX, pixY, iPlane;
    chainPixel->SetBranchAddress("iEvent", &rawEventID);
    chainPixel->SetBranchAddress("iHit", &iHit);
    chainPixel->SetBranchAddress("PixX", &pixX);
    chainPixel->SetBranchAddress("PixY", &pixY);
    if (oldtree){
        iPlane = 0;
        chainPixel->SetBranchAddress("Energy", &corrEnergy); // old naming of branch
        chainPixel->SetBranchAddress("timeWalkHit", &timeWalkHit);
    }
    else {
        chainPixel->SetBranchAddress("iPlane", &iPlane);
        chainPixel->SetBranchAddress("hitEnergy", &corrEnergy); 
        chainPixel->SetBranchAddress("hitTime", &timeWalkHit);
    }
    Long64_t nRawEntries = chainPixel->GetEntries();
    std::map<std::pair<int,std::pair<int, int>>, double> enMap; // Avoid O(n^2) nested loops via extra map 
    std::map<int, std::vector<double>> timeMap;
    
    std::cout << " Raw hit entries: " << nRawEntries << std::endl;
    for (Long64_t j = 0; j < nRawEntries; j++)
    {
        chainPixel->GetEntry(j);
        //std::cout << j << " Plane: " << iPlane << " number" << std::endl;
        if (iPlane != 0) continue; // only consider DUT with iPlane ==0
        enMap[{rawEventID, {pixX, pixY}}] += corrEnergy;
        // Deprecated will require to be purged
        timeMap[rawEventID].push_back(timeWalkHit); // Store all times
    }
    std::map<int, int> clusterMap; // Map to hold cluster size per event
    std::map<int, double> clusterTimeMap; // Map to hold leading time per event

    int simpleCounter = 0;
    int rememberer =0;
    double timecut = 500; // ns
    double dist_cut = 80/1000.; // in mm
    std::map<int, std::vector<double>> inClusterTimes;
    for (const auto& entry : enMap) 
    {
        int eventID = entry.first.first;
        //int hitID   = entry.first.second;
        double cenergy = entry.second; // corrected energy
        double timing = GetTimingOffset(cenergy, threshold);
        //if (timing > timecut) std::cout << eventID << "     charge: " << cenergy << "     time: " << timing << std::endl;

        int pixelX = entry.first.second.first;
        int pixelY = entry.first.second.second;
        //std::cout << "Event ID: " << eventID << "; pixX: " << entry.first.second.first << ";pixY: " << entry.first.second.second << "; corrEnergy: " << cenergy << std::endl;
        
        bool distPass = true;
        
        // check for possible tracking cut based on MC truth vertex:        
        std::pair<double, double> pixelGlobalPosition = PixelPositionReconstruction(pixelX, pixelY);

        auto it = vertexMap.find(eventID);
        if (it != vertexMap.end()) {
            auto [vx, vy] = it->second;
            if (abs(pixelGlobalPosition.first-vx)>dist_cut || abs(pixelGlobalPosition.second-vy)>dist_cut){
                distPass = false; // filter out
                //std::cout << eventID << std::endl;
                //std::cout << "X " << pixelGlobalPosition.first << " - " << vx << " = " << pixelGlobalPosition.first-vx << std::endl;
                //std::cout << "Y " << pixelGlobalPosition.second << " - " << vy << " = " << pixelGlobalPosition.second-vy << std::endl;              
                //std::cout << "    ----   " << std::endl;
            }
        }
        
        inClusterTimes[eventID].push_back(timing);

        // Fill the cluster size map
        if (cenergy > threshold && timing < timecut && distPass) 
        {
            clusterMap[eventID]++; // Increment cluster size for this event
            //I take the fastest hit in an event (cluster) as MALTA does.
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
        if (eventID == 27437)
        {
            std::cout << "Event ID: " << eventID << "; pixX: " << entry.first.second.first << ";pixY: " << entry.first.second.second << "; corrEnergy: " << cenergy << std::endl;
        }
    }

    int previousID =  0;
    for (Long64_t i = 0; i < nEntriesMCTruth; ++i) 
    {
        MCTruthchain->GetEntry(i);
        // Do me once per event

        if (MCtrackEventID == previousID) {
            std::cout << "SameID: " << MCtrackEventID << " " << previousID << std::endl; // is this ever happening?
            continue;
        }
        previousID = MCtrackEventID;
        // Fold positions into 2x2 grid but convert to um (*1000)
        double xFolded = fmod(MCtrackX, 2*pixelSizeX) * 1000; 
        double yFolded = fmod(MCtrackY, 2*pixelSizeY) * 1000;
        double zFolded = (50 - MCtrackZ) * 1000;

        double xIP_track = fmod(MCtrackX + rng.Gaus(0., trackunc_X), 2*pixelSizeX) * 1000; 
        double yIP_track = fmod(MCtrackY + rng.Gaus(0., trackunc_Y), 2*pixelSizeX) * 1000; 

        auto it = clusterMap.find(MCtrackEventID);
        //std::cout << "Event ID: " << MCtrackEventID << "; X: " << MCtrackX << "; Y: " << MCtrackY << "En.: " << it->second << std::endl;  
        if (it != clusterMap.end()) 
        {   
            int NumHits = min(it->second, 8); // restrict clusters to maximum size 8 // this is not needed if distance cut dist_cut is below order 3*pixel size (100um)
            //if (it->second > 8) std::cout << i << " " << it->second << " " << NumHits << std::endl;

            hInPixelClSize->Fill(xFolded, yFolded, NumHits);
            hInPixelMatch->Fill(xFolded, yFolded, NumHits != 0 ? 1 : 0);

            hInPixelClSize_trackunc->Fill(xIP_track, yIP_track, NumHits);
            hInPixelMatch_trackunc->Fill(xIP_track, yIP_track, NumHits != 0 ? 1 : 0);
        }
        auto itTime = clusterTimeMap.find(MCtrackEventID);
        if (itTime != clusterTimeMap.end())
        {
            hInPixelTime->Fill(xFolded, yFolded, itTime->second);
        }

        //hInPixelClSize->Fill(xFolded, yFolded, clSize); // Quick and dirty way to fill cluster size. No threshold implementation
        hInPixelAll->Fill(xFolded, yFolded, 1);
        hInPixelAll_trackunc->Fill(xIP_track, yIP_track, 1);
    }

    result[0] = hInPixelClSize_trackunc->Integral() / hInPixelMatch_trackunc->Integral();
    std::cout << "Av. Cl size: " << result[0] << std::endl;

    // divide cluster size and timing by number of events with hit (to get average across all events with hit)
    hInPixelClSize->Divide(hInPixelMatch); 
    hInPixelClSize_trackunc->Divide(hInPixelMatch_trackunc);
    hInPixelTime->Divide(hInPixelMatch);

    result[1] = getEff(hInPixelMatch_trackunc->Integral(), hInPixelAll_trackunc->Integral());// in percent
    result[2] = getEffErr(hInPixelMatch_trackunc->Integral(), hInPixelAll_trackunc->Integral());// in percent

    std::cout << "Matched tracks: " << hInPixelMatch->Integral() << std::endl;
    std::cout << "All tracks: " << hInPixelAll->Integral() << std::endl;
    std::cout << "Av. Eff.: " << result[1] << "+- " 
                << result[2] << std::endl;

    hInPixelMatch->Divide(hInPixelAll);
    hInPixelMatch_trackunc->Divide(hInPixelAll_trackunc);
    hInPixelMatch->Scale(100.); // in percent
    hInPixelMatch_trackunc->Scale(100.); // in percent

    /*
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
    */

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
    // Create TLatex object
    TLatex *t = new TLatex();
    t->DrawLatex(-2., pixelSizeY*2*1000. +2, Form("MALTA2 Sim.#bf{, 30#mum EPI, <cl. size> =%.2f}", result[0]));
    c2D->SaveAs(Form("2DIPClSize_%.0fThr.pdf", threshold));

    // IP Eff plot
    hInPixelMatch->Draw("COLZ");
    hInPixelMatch->SetMinimum(0.);
    hInPixelMatch->SetMaximum(100.);
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
    hInPixelMatch->Write();
    hInPixelClSize->Write();
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
    double thresholds[] = {2000., 200.};
    //double thresholds[] = {1400., 200., 1200.};
    //double thresholds[] = {200, 230, 343, 448, 544, 632, 712}; // equivalent to thresholds of data points

    int num_values = sizeof(thresholds) / sizeof(thresholds[0]);
    double results[num_values][3];

    // Create two TGraphs
    TGraphErrors *gr_AvClSize = new TGraphErrors(num_values);
    gr_AvClSize->SetName("AverageClSize");
    TGraphErrors *gr_AvEff = new TGraphErrors(num_values);
    gr_AvEff->SetName("AverageEff");


    TFile outFile(outROOT.c_str(), "RECREATE"); // create new file and fill in simpleAnalysis
    outFile.Close();

    // Get average cl. size and eff. for each threshold
    for (int i = 0; i < num_values; i++) {
        simpleAnalysis(inputFile, outROOT, results[i], thresholds[i]);
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
    for (int runNumber = 55; runNumber <= 55; ++runNumber) { // 46 to 50
        std::string inputFileName = "/Users/lucianfasselt/DECAL/Simulation/Geant4/MALTASIM/malta_simulation/Results/local_00"+ std::to_string(runNumber)+"/";  
        std::string outROOTName = "SimOutput_MaxCl8_MCTrue_distcut80_" + std::to_string(runNumber) + ".root";
        threshold_loop(inputFileName, outROOTName.c_str());
    }
}

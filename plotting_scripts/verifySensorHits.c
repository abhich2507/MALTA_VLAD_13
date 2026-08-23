// verifySensorHits.c
// Verification macro: plots pixel distributions and 3D hit maps for all 8 sensors
// Usage: root -l 'verifySensorHits.c(11)' where 11 is the run number
// Or:    root -l 'verifySensorHits.c(11, 6)' where 6 is the number of threads

#include "TChain.h"
#include "TCanvas.h"
#include "TH1D.h"
#include "TH2D.h"
#include "TH3D.h"
#include "TStyle.h"
#include "TLegend.h"
#include "TLatex.h"
#include <iostream>
#include <vector>
#include <map>

void verifySensorHits(int runNumber = 0, int numThreads = 6)
{
    gStyle->SetOptStat(1111);
    gStyle->SetPalette(kRainBow);

    
    // Geometry parameters (must match flags.cfg and DetectorConstructor.cc)
   
    double detectorXOffset = 5.0;   // cm
    double detectorYOffset = 5.0;   // cm
    double detectorZOffset = 5.0;   // cm
    double detectorSizeX   = 1.86368; // cm
    double detectorSizeY   = 0.81536; // cm
    double pixelSize       = 0.0364;  // mm
    double sensorGap       = 0.2;     // cm (2mm)
    double sensorSpacing   = detectorSizeX + sensorGap; // cm, center-to-center

    // Sensor X centers in cm (matching DetectorConstructor.cc)
    // Plane 0: planeID 0,1,2,3  |  Plane 1: planeID 100,101,102,103
    std::map<int, double> sensorXCenter;
    sensorXCenter[0]   = detectorXOffset - 1.5 * sensorSpacing;
    sensorXCenter[1]   = detectorXOffset - 0.5 * sensorSpacing;
    sensorXCenter[2]   = detectorXOffset + 0.5 * sensorSpacing;
    sensorXCenter[3]   = detectorXOffset + 1.5 * sensorSpacing;
    sensorXCenter[100] = detectorXOffset - 1.5 * sensorSpacing;
    sensorXCenter[101] = detectorXOffset - 0.5 * sensorSpacing;
    sensorXCenter[102] = detectorXOffset + 0.5 * sensorSpacing;
    sensorXCenter[103] = detectorXOffset + 1.5 * sensorSpacing;

    // Sensor Z positions in cm
    std::map<int, double> sensorZ;
    sensorZ[0]   = detectorZOffset;
    sensorZ[1]   = detectorZOffset;
    sensorZ[2]   = detectorZOffset;
    sensorZ[3]   = detectorZOffset;
    sensorZ[100] = detectorZOffset + 20.0; // 20 cm offset for plane 1
    sensorZ[101] = detectorZOffset + 20.0;
    sensorZ[102] = detectorZOffset + 20.0;
    sensorZ[103] = detectorZOffset + 20.0;

    // Sensor labels
    std::map<int, TString> sensorLabel;
    sensorLabel[0]   = "P0_L1 (id=0)";
    sensorLabel[1]   = "P0_L2 (id=1)";
    sensorLabel[2]   = "P0_R1 (id=2)";
    sensorLabel[3]   = "P0_R2 (id=3)";
    sensorLabel[100] = "P1_L1 (id=100)";
    sensorLabel[101] = "P1_L2 (id=101)";
    sensorLabel[102] = "P1_R1 (id=102)";
    sensorLabel[103] = "P1_R2 (id=103)";

    std::vector<int> allIDs = {0, 1, 2, 3, 100, 101, 102, 103};

    // ============================================================
    // Load data
    // ============================================================
    TString inputPath = Form("./Results/local_%04d/", runNumber);
    std::cout << "Loading data from: " << inputPath << std::endl;
    TChain *chain = new TChain("RawPixelHits");
    for (int t = 0; t < numThreads; t++)
    {
        chain->Add(Form("%soutput0_t%d.root", inputPath.Data(), t));
    }
    Long64_t nEntries = chain->GetEntries();
    std::cout << "Total entries in RawPixelHits: " << nEntries << std::endl;

    int iEvent, iPlane, iHit, PixX, PixY;
    double hitTime, hitEnergy, totalEnergy;
    chain->SetBranchAddress("iEvent",      &iEvent);
    chain->SetBranchAddress("iPlane",      &iPlane);
    chain->SetBranchAddress("iHit",        &iHit);
    chain->SetBranchAddress("PixX",        &PixX);
    chain->SetBranchAddress("PixY",        &PixY);
    chain->SetBranchAddress("hitTime",     &hitTime);
    chain->SetBranchAddress("hitEnergy",   &hitEnergy);
    chain->SetBranchAddress("totalEnergy", &totalEnergy);

    // ============================================================
    // Create histograms
    // ============================================================

    // 1) PixX 1D histograms per sensor
    std::map<int, TH1D*> h1PixX;
    // 2) PixY 1D histograms per sensor
    std::map<int, TH1D*> h1PixY;
    // 3) Count per sensor (for summary)
    std::map<int, int> hitCount;

    int colors[] = {kBlue, kCyan+1, kGreen+2, kSpring+5, kRed, kOrange+7, kMagenta, kViolet+1};

    for (int idx = 0; idx < allIDs.size(); idx++)
    {
        int id = allIDs[idx];
        h1PixX[id] = new TH1D(Form("h1PixX_%d", id), Form("PixX - %s;PixX;Entries", sensorLabel[id].Data()), 512, 0, 512);
        h1PixY[id] = new TH1D(Form("h1PixY_%d", id), Form("PixY - %s;PixY;Entries", sensorLabel[id].Data()), 224, 0, 224);
        h1PixX[id]->SetLineColor(colors[idx]);
        h1PixY[id]->SetLineColor(colors[idx]);
        h1PixX[id]->SetLineWidth(2);
        h1PixY[id]->SetLineWidth(2);
        hitCount[id] = 0;
    }

    // 4) 1D Z-axis histogram (which sensor was hit)
    TH1D *h1PlaneID = new TH1D("h1PlaneID", "Hit count per sensor ID;Sensor ID (iPlane);Entries", 110, -5, 105);
    h1PlaneID->SetFillColor(kBlue-9);

    // 5) 3D histograms: real coordinates for each plane
    // X range: cover all 4 sensors from leftmost to rightmost edge
    double xMin = sensorXCenter[0] - detectorSizeX/2 - 0.5;
    double xMax = sensorXCenter[3] + detectorSizeX/2 + 0.5;
    double yMin = detectorYOffset - detectorSizeY/2 - 0.5;
    double yMax = detectorYOffset + detectorSizeY/2 + 0.5;

    TH3D *h3Plane0 = new TH3D("h3Plane0", "3D Hits - Plane 0 (id 0-3);X [cm];Y [cm];Z [cm]",
                                100, xMin, xMax,
                                50, yMin, yMax,
                                10, detectorZOffset - 1, detectorZOffset + 1);

    TH3D *h3Plane1 = new TH3D("h3Plane1", "3D Hits - Plane 1 (id 100-103);X [cm];Y [cm];Z [cm]",
                                100, xMin, xMax,
                                50, yMin, yMax,
                                10, detectorZOffset + 20.0 - 1, detectorZOffset + 20.0 + 1);

    // 6) Energy deposition 2D per sensor (pixel grid, summed hitEnergy in e-)
    std::map<int, TH2D*> h2Edep;
    for (int idx = 0; idx < allIDs.size(); idx++)
    {
        int id = allIDs[idx];
        h2Edep[id] = new TH2D(Form("h2Edep_%d", id),
                              Form("Energy deposition - %s;Pixel X;Pixel Y;E_{dep} [e^{-}]", sensorLabel[id].Data()),
                              512, 0, 512, 224, 0, 224);
    }

    // 7) Combined 3D: both planes together
    TH3D *h3All = new TH3D("h3All", "3D Hits - All Sensors;X [cm];Y [cm];Z [cm]",
                            100, xMin, xMax,
                            50, yMin, yMax,
                            50, detectorZOffset - 2, detectorZOffset + 22);

    // ============================================================
    // Event loop
    // ============================================================
    for (Long64_t i = 0; i < nEntries; i++)
    {
        chain->GetEntry(i);

        // Only process known sensor IDs
        if (sensorXCenter.find(iPlane) == sensorXCenter.end()) continue;

        // Fill pixel histograms
        h1PixX[iPlane]->Fill(PixX);
        h1PixY[iPlane]->Fill(PixY);
        h1PlaneID->Fill(iPlane);
        h2Edep[iPlane]->Fill(PixX, PixY, hitEnergy);  // energy deposition per pixel
        hitCount[iPlane]++;

        // Convert pixel coordinates to real world coordinates (cm)
        double realX = sensorXCenter[iPlane] - detectorSizeX/2 + PixX * pixelSize / 10.0; // mm->cm
        double realY = detectorYOffset       - detectorSizeY/2 + PixY * pixelSize / 10.0;
        double realZ = sensorZ[iPlane];

        // Fill 3D histograms
        if (iPlane < 100)
            h3Plane0->Fill(realX, realY, realZ);
        else
            h3Plane1->Fill(realX, realY, realZ);

        h3All->Fill(realX, realY, realZ);
    }

    // ============================================================
    // Print summary
    // ============================================================
    std::cout << "\n========== HIT SUMMARY ==========" << std::endl;
    for (int id : allIDs)
    {
        std::cout << Form("  Sensor %3d (%s): %d hits", id, sensorLabel[id].Data(), hitCount[id]) << std::endl;
    }
    std::cout << "  Total: " << nEntries << " entries" << std::endl;
    std::cout << "=================================\n" << std::endl;

    // ============================================================
    // CANVAS 1: PixX distributions (all sensors overlaid)
    // ============================================================
    TCanvas *c1 = new TCanvas("c1", "PixX per Sensor", 1200, 800);
    c1->Divide(4, 2);
    for (int idx = 0; idx < allIDs.size(); idx++)
    {
        c1->cd(idx + 1);
        h1PixX[allIDs[idx]]->Draw();
    }
    c1->SaveAs(Form("%s/verify_PixX_per_sensor.root", inputPath.Data()));

    // ============================================================
    // CANVAS 2: PixY distributions (all sensors overlaid)
    // ============================================================
    TCanvas *c2 = new TCanvas("c2", "PixY per Sensor", 1200, 800);
    c2->Divide(4, 2);
    for (int idx = 0; idx < allIDs.size(); idx++)
    {
        c2->cd(idx + 1);
        h1PixY[allIDs[idx]]->Draw();
    }
    c2->SaveAs(Form("%s/verify_PixY_per_sensor.root", inputPath.Data()));

    // ============================================================
    // CANVAS 3: Sensor ID distribution (1D Z-axis equivalent)
    // ============================================================
    TCanvas *c3 = new TCanvas("c3", "Hits per Sensor ID", 800, 600);
    h1PlaneID->Draw();
    c3->SaveAs(Form("%s/verify_hits_per_sensorID.root", inputPath.Data()));

    // ============================================================
    // CANVAS 4: 3D hit map - Plane 0
    // ============================================================
    TCanvas *c4 = new TCanvas("c4", "3D Hits Plane 0", 900, 700);
    h3Plane0->SetMarkerStyle(20);
    h3Plane0->SetMarkerSize(0.3);
    h3Plane0->SetMarkerColor(kBlue);
    h3Plane0->Draw("BOX2");
    c4->SaveAs(Form("%s/verify_3D_Plane0.root", inputPath.Data()));

    // ============================================================
    // CANVAS 5: 3D hit map - Plane 1
    // ============================================================
    TCanvas *c5 = new TCanvas("c5", "3D Hits Plane 1", 900, 700);
    h3Plane1->SetMarkerStyle(20);
    h3Plane1->SetMarkerSize(0.3);
    h3Plane1->SetMarkerColor(kRed);
    h3Plane1->Draw("BOX2");
    c5->SaveAs(Form("%s/verify_3D_Plane1.root", inputPath.Data()));

    // ============================================================
    // CANVAS 6: Combined 3D hit map (both planes, all 8 sensors)
    // ============================================================
    TCanvas *c6 = new TCanvas("c6", "3D Hits All Sensors", 1000, 800);
    h3All->SetMarkerStyle(20);
    h3All->SetMarkerSize(0.3);
    h3All->Draw("BOX2");
    c6->SaveAs(Form("%s/verify_3D_AllSensors.root", inputPath.Data()));

    // ============================================================
    // CANVAS 7: 2D XY hit map per sensor (bird's eye view)
    // ============================================================
    TCanvas *c7 = new TCanvas("c7", "2D XY Hit Maps", 1600, 800);
    c7->Divide(4, 2);

    std::map<int, TH2D*> h2XY;
    for (int idx = 0; idx < allIDs.size(); idx++)
    {
        int id = allIDs[idx];
        double sxMin = sensorXCenter[id] - detectorSizeX/2;
        double sxMax = sensorXCenter[id] + detectorSizeX/2;
        h2XY[id] = new TH2D(Form("h2XY_%d", id),
                             Form("XY - %s;X [cm];Y [cm]", sensorLabel[id].Data()),
                             128, sxMin, sxMax,
                             128, yMin, yMax);
    }

    // Second pass for 2D fill (or reuse data)
    for (Long64_t i = 0; i < nEntries; i++)
    {
        chain->GetEntry(i);
        if (sensorXCenter.find(iPlane) == sensorXCenter.end()) continue;
        double realX = sensorXCenter[iPlane] - detectorSizeX/2 + PixX * pixelSize / 10.0;
        double realY = detectorYOffset       - detectorSizeY/2 + PixY * pixelSize / 10.0;
        h2XY[iPlane]->Fill(realX, realY);
    }

    for (int idx = 0; idx < allIDs.size(); idx++)
    {
        c7->cd(idx + 1);
        h2XY[allIDs[idx]]->Draw("COLZ");
    }
    c7->SaveAs(Form("%s/verify_2D_XY_per_sensor.root", inputPath.Data()));

    // ============================================================
    // CANVAS 8: Energy deposition 2D per sensor (pixel-level)
    // ============================================================
    TCanvas *c8 = new TCanvas("c8", "Energy Deposition per Sensor", 1600, 800);
    c8->Divide(4, 2);
    for (int idx = 0; idx < allIDs.size(); idx++)
    {
        c8->cd(idx + 1);
        int id = allIDs[idx];
        h2Edep[id]->Draw("COLZ");
    }
    c8->SaveAs(Form("%s/verify_Edep_per_sensor.root", inputPath.Data()));

    std::cout << "All verification plots saved to: " << inputPath << std::endl;
}

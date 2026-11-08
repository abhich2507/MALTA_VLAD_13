// verify_2D_XY_plane.c
// Standalone script to plot glued 2D XY hit maps for planes 0 and 1 on the SAME page
// Usage: root -l 'verify_2D_XY_plane.c(11)' where 11 is the run number

#include "TChain.h"
#include "TCanvas.h"
#include "TH2D.h"
#include "TStyle.h"
#include "TLatex.h"
#include "TString.h"
#include "TPad.h"
#include "TArrow.h"
#include <iostream>
#include <vector>
#include <map>

void verify_2D_XY_plane(int runNumber = 0, int numThreads = 6)
{
    gStyle->SetOptStat(0); 
    gStyle->SetPalette(kRainBow);
    
    // Using font 43 (Precision 3) forces absolute pixel sizes
    gStyle->SetLabelFont(43, "XYZ");
    gStyle->SetTitleFont(43, "XYZ");
    gStyle->SetTextFont(43);

    // ============================================================
    // Geometry parameters 
    // ============================================================
    double detectorXOffset = 0.0;
    double detectorYOffset = 0.0;
    double detectorSizeX   = 1.86368; 
    double detectorSizeY   = 1.86368; 
    double pixelSize       = 0.0364;  

    std::map<int, double> sensorXCenter;
    sensorXCenter[0]     = detectorXOffset + 0.00000;
    sensorXCenter[1]     = detectorXOffset + 1.86369;
    sensorXCenter[2]     = detectorXOffset + 3.72737;
    sensorXCenter[3]     = detectorXOffset + 5.59105;
    sensorXCenter[10000] = detectorXOffset + 0.00000;
    sensorXCenter[10001] = detectorXOffset + 1.86369;
    sensorXCenter[10002] = detectorXOffset + 3.72737;
    sensorXCenter[10003] = detectorXOffset + 5.59105;

    std::vector<int> allIDs = {0, 1, 2, 3, 10000, 10001, 10002, 10003};

    // ============================================================
    // Load data
    // ============================================================
    TString inputPath = Form("./Results_pi1GeV/local_%04d/", runNumber);
    std::cout << "Loading data from: " << inputPath << std::endl;
    
    TChain *chain = new TChain("RawPixelHits");
    for (int t = 0; t < numThreads; t++)
    {
        chain->Add(Form("%soutput0_t%d.root", inputPath.Data(), t));
    }
    
    Long64_t nEntries = chain->GetEntries();
    std::cout << "Total entries in RawPixelHits: " << nEntries << std::endl;

    int iPlane, PixX, PixY;
    chain->SetBranchAddress("iPlane", &iPlane);
    chain->SetBranchAddress("PixX",   &PixX);
    chain->SetBranchAddress("PixY",   &PixY);

    // ============================================================
    // Create 2D Histograms
    // ============================================================
    const double shYmin = -10.0, shYmax = 10.0; 

    std::map<int, TH2D*> h2XY;
    for (int idx = 0; idx < allIDs.size(); idx++)
    {
        int id = allIDs[idx];
        double sxMin = (sensorXCenter[id] - detectorSizeX/2) * 10.0;   
        double sxMax = (sensorXCenter[id] + detectorSizeX/2) * 10.0;   
        h2XY[id] = new TH2D(Form("h2XY_%d", id), ";X [mm];Y [mm]",
                            128, sxMin, sxMax, 40, shYmin, shYmax);
        h2XY[id]->SetStats(0);
        h2XY[id]->GetXaxis()->SetNdivisions(505);
        h2XY[id]->GetYaxis()->SetNdivisions(505);
        
        h2XY[id]->GetXaxis()->SetTitleFont(43);
        h2XY[id]->GetXaxis()->SetLabelFont(43);
        
        h2XY[id]->GetYaxis()->SetTitleFont(43);
        h2XY[id]->GetYaxis()->SetLabelFont(43);
        
        // Increased offsets so the larger 34px titles don't crash into the 35px labels
        h2XY[id]->GetXaxis()->SetTitleOffset(1.2);
        h2XY[id]->GetYaxis()->SetTitleOffset(1.2); 
        h2XY[id]->GetYaxis()->SetLabelOffset(0.015);
        h2XY[id]->GetXaxis()->SetLabelOffset(0.015);
        
        h2XY[id]->GetZaxis()->SetMaxDigits(3);
    }

    // ============================================================
    // Fill histograms with Global XY coordinates
    // ============================================================
    for (Long64_t i = 0; i < nEntries; i++)
    {
        chain->GetEntry(i);
        if (sensorXCenter.find(iPlane) == sensorXCenter.end()) continue;
        
        double realX = (sensorXCenter[iPlane] - detectorSizeX/2) * 10.0 + PixX * pixelSize;   
        double realY = (detectorYOffset       - detectorSizeY/2) * 10.0 + PixY * pixelSize;   
        
        h2XY[iPlane]->Fill(realX, realY);
    }

    // ============================================================
    // CUSTOMIZABLE TEXT SIZES (ABSOLUTE PIXELS)
    // ============================================================
    int customLabelSize    = 35;  // Pixel height for axis numbers
    int customTitleSize    = 34;  // Pixel height for "X [mm]" and "Y [mm]"
    int customTopLabelSize = 30;  // Pixel height for "S0", "S1", etc.

    // ============================================================
    // Draw and Save glued pads (Combined Canvas)
    // ============================================================
    
    // Make canvas twice as tall to fit both rows (1760 x 1000)
    TCanvas* c = new TCanvas("c_both", "Both Planes", 1760, 1000);
    
    // Increased left margin (l) to 0.12 to make plenty of room for the Z-axis arrow
    double l = 0.12; 
    double r = 0.10; 
    double f = (1.0 - l - r) / 4.0; 

    int planeBase[2] = {0, 10000};
    
    for (int p = 0; p < 2; p++)
    {
        int id0 = planeBase[p];

        double zmax = 1.0;
        for (int s = 0; s < 4; s++)
        {
            double m = h2XY[id0 + s]->GetMaximum();
            if (m > zmax) zmax = m;
        }

        // p == 0 (Plane 0) goes on top (Y: 0.5 to 1.0)
        // p == 1 (Plane 1) goes on bottom (Y: 0.0 to 0.5)
        double yLow  = (p == 0) ? 0.5 : 0.0;
        double yHigh = (p == 0) ? 1.0 : 0.5;

        for (int s = 0; s < 4; s++)
        {
            c->cd();
            
            // Calculate Pad X bounds
            double xLow = 0.0, xHigh = 0.0;
            if (s == 0) { xLow = 0.0;       xHigh = l + f;   }
            if (s == 1) { xLow = l + f;     xHigh = l + 2*f; }
            if (s == 2) { xLow = l + 2*f;   xHigh = l + 3*f; }
            if (s == 3) { xLow = l + 3*f;   xHigh = 1.0;     }

            TPad* pad = new TPad(Form("pad_%d_%d", p, s), Form("pad_%d_%d", p, s), xLow, yLow, xHigh, yHigh);
            pad->Draw();
            pad->cd();

            gPad->SetTopMargin(0.14);
            gPad->SetBottomMargin(0.20); // slightly increased bottom margin for large text

            // Apply calculated left/right margins to keep frame width identical
            if (s == 0) {
                gPad->SetLeftMargin(l / (l + f));
                gPad->SetRightMargin(0.0);
            } else if (s == 3) {
                gPad->SetLeftMargin(0.0);
                gPad->SetRightMargin(r / (f + r));
            } else {
                gPad->SetLeftMargin(0.0);
                gPad->SetRightMargin(0.0);
            }

            TH2D* h = h2XY[id0 + s];
            h->SetMinimum(0.1); 
            h->SetMaximum(zmax); 

            h->GetXaxis()->SetLabelSize(customLabelSize);
            h->GetXaxis()->SetTitleSize(customTitleSize);
            h->GetYaxis()->SetLabelSize(customLabelSize);
            h->GetYaxis()->SetTitleSize(customTitleSize);
            h->GetZaxis()->SetLabelSize(customLabelSize);

            // Hide Y-axis titles and labels on all but the 1st sensor
            if (s > 0)
            {
                h->GetYaxis()->SetTitleSize(0);
                h->GetYaxis()->SetLabelSize(0);
            }

            // Hide X-axis titles on all but the 4th sensor
            if (s != 3)
            {
                h->GetXaxis()->SetTitleSize(0);
            }

            if (s == 3)
                h->Draw("COLZ");  
            else
                h->Draw("COL");

            double labelX = 0.5;
            if (s == 0) labelX = 1.0 - (0.5 * f / (l + f));
            if (s == 3) labelX = 0.5 * f / (f + r);

            TString sLabel = (id0 < 10000) ? Form("S%d", s) : Form("S%d", 100 + s);
            // Append plane info to the outer plots just for clarity (optional)
            //if (s == 0) sLabel = Form("Plane %d - %s", p, sLabel.Data());

            TLatex latex;
            latex.SetNDC();
            latex.SetTextFont(63); // Bold pixel font
            latex.SetTextSize(customTopLabelSize); 
            latex.SetTextAlign(22);
            latex.DrawLatex(labelX, 0.90, sLabel);
        }
    }

    // ============================================================
    // Draw the Z-Axis Arrow on the Main Canvas
    // ============================================================
    c->cd(); 
    


    // ============================================================
    // Save Combined Canvas
    // ============================================================
    gStyle->SetPaperSize(17.6, 10.0);   
    c->SaveAs(Form("%s/verify_2D_XY_both_planes.root", inputPath.Data()));
    c->SaveAs(Form("%s/verify_2D_XY_both_planes.pdf",  inputPath.Data()));
    gStyle->SetPaperSize(20.0, 26.0);  
    
    std::cout << "Done generating combined plane PDF." << std::endl;
}
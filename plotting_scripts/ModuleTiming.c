#include <iostream>
#include <ROOT/RNTuple.hxx>
#include <TH3D.h>
#include <TFile.h>
#include <cmath>


void ModuleTiming()
{
    // Build file path
    const int runNumber = 194;
    std::string tag = "NewTime";
    std::string anaPath = Form("Results/local_%04d/%s/LocalTrackedHitsThr200.root", runNumber, tag.c_str());
    //std::string anaPath = Form("Results/local_%04d/%s/analysisThr200.root", runNumber, tag.c_str());
    std::cout << "Opening: " << anaPath << std::endl;

    TFile *anaFile = TFile::Open(anaPath.c_str(), "READ");
    if (!anaFile || anaFile->IsZombie()) {
        std::cerr << "Could not open file: " << anaPath << std::endl;
        return ;
    }

    TTree *anaTree = (TTree*) anaFile->Get("TrackedHits_planeZ0");
    //TTree *anaTree = (TTree*) anaFile->Get("analyzedHits_planeZ0");
    if (!anaTree) {
        std::cerr << "No anaTree in file: " << anaPath << std::endl;
        return ;
    }

    double timing{};
    int planeID{};
    anaTree->SetBranchAddress("DUTLocalTime", &timing);
    //anaTree->SetBranchAddress("timing", &timing);
    anaTree->SetBranchAddress("planeID", &planeID);

    TH1D *h1Plane0    = new TH1D("h1Plane0", "", 100,116,180);
    TH1D *h1Plane1    = new TH1D("h1Plane1", "h1Plane1", 100,116,180);
    TH1D *h1Plane2    = new TH1D("h1Plane2", "h1Plane2", 100,116,180);
    TH1D *h1Plane3    = new TH1D("h1Plane3", "h1Plane3", 100,116,180);

    // First we need to sort the tree, because not all runs are performed in consecutive threshold values
    int nEntries = anaTree->GetEntries();

    for (Long64_t i = 0; i < nEntries; i++) 
    {
        anaTree->GetEntry(i);
        //std::cout << "planeID: " << planeID << "; timing: " << timing << std::endl; 

        if (planeID == 3) 
        {
            //std::cout << "Filled 0!" << std::endl;
            h1Plane0->Fill(timing + 110);
        }
        else if (planeID == 2) 
        {
            //std::cout << "Filled 1!" << std::endl;
            h1Plane1->Fill(timing + 110);
        }
        else if (planeID == 1) 
        {
            //std::cout << "Filled 2!" << std::endl;
            h1Plane2->Fill(timing + 110);
        }
        else if (planeID == 0) 
        {
            //std::cout << "Filled 3!" << std::endl;
            h1Plane3->Fill(timing + 110);
        }
    }
    /*
    TCanvas *c1 = new TCanvas("c1","2D Threshold",800,800);
    h1Plane0->Draw();
    h1Plane1->Draw("SAME");
    h1Plane2->Draw("SAME");
    h1Plane3->Draw("SAME");
    */

    // Colors per plane
    int colors[4] = {kBlue, kRed, kGreen+1, kMagenta};
    // Marker styles: circle, square, triangle up, triangle down
    int markers[4] = {20, 21, 22, 23};

    TH1* hPlanes[4] = {h1Plane0, h1Plane1, h1Plane2, h1Plane3};

    // Normalize all histograms to a.u.
    for (int i = 0; i < 4; i++) {
        if (hPlanes[i]->Integral() > 0)
            hPlanes[i]->Scale(1.0 / hPlanes[i]->Integral());
    }
    gStyle->SetOptStat(0);
    TCanvas *c1 = new TCanvas("c1", "Hit Timing", 1200, 800);

    for (int i = 0; i < 4; i++) {
        hPlanes[i]->SetMarkerStyle(markers[i]);
        hPlanes[i]->SetMarkerColor(colors[i]);
        hPlanes[i]->SetMarkerSize(1.4);
        hPlanes[i]->SetLineWidth(5.);
        hPlanes[i]->SetLineColor(0);
        hPlanes[i]->SetFillStyle(0);
    }

    hPlanes[0]->GetXaxis()->SetTitle("Hit Timing [ns]");
    hPlanes[0]->GetYaxis()->SetTitle("a.u.");
    hPlanes[0]->Draw("P");
    for (int i = 1; i < 4; i++)
        hPlanes[i]->Draw("P SAME");

    // Legend positioned in the top right like the image
    TLegend *leg = new TLegend(0.58, 0.55, 0.88, 0.88);
    leg->SetBorderSize(0);      // no border
    leg->SetFillStyle(0);       // transparent background
    leg->SetTextSize(0.035);

    // Fit each with a Gaussian, draw the curve, and add to legend

    // Define fit range windows
    std::vector<double> vfpeak {126,134,142,150};
    std::vector<double> vfwidth {2.7,2.7,2.7,2.7};
    for (int i = 0; i < 4; i++) 
    {
        TF1 *fitFunc = new TF1(Form("fit%d", i), "gaus", vfpeak[i] - 1.6*vfwidth[i], vfpeak[i] + vfwidth[i]);
        fitFunc->SetLineColor(colors[i]);
        fitFunc->SetLineWidth(4);
        hPlanes[i]->Fit(fitFunc, "QR");

        double mu    = fitFunc->GetParameter(1);
        double sigma = fitFunc->GetParameter(2);

        fitFunc->Draw("SAME");

        // Two-line legend entry matching the image style
        leg->AddEntry(hPlanes[i],
            Form("Plane %d,  peak = %.1f ns", i, mu),
            "PL");
        leg->AddEntry((TObject*)0,
            Form("#sigma = %.1f ns", sigma),
            "");
    }

    TLatex *t3 = new TLatex();
    t3->SetTextSize(0.043);
    t3->SetNDC();
    t3->DrawLatex(0.32, 0.92, Form("MALTA2 Simulation, #bf{30#mum EPI}"));

    leg->Draw();
    c1->Update();

    c1->SaveAs("PublicPlots/ModuleTiming.pdf");
    c1->SaveAs("PublicPlots/ModuleTiming.C");

}
#include <iostream>
#include <ROOT/RNTuple.hxx>
#include <TH3D.h>
#include <TFile.h>
#include <cmath>

void print_colDistr(TH2D* h2Thres)
{
    bool debug = false;

    int nBinsX = h2Thres->GetNbinsX();
    int nBinsY = h2Thres->GetNbinsY();
    int groupSize = 32;

    for (int iGroup = 0; iGroup < nBinsX / groupSize; iGroup++) 
    {
        int xStart = iGroup * groupSize + 1;
        int xEnd   = (iGroup + 1) * groupSize;

        std::vector<double> values;

        // Loop over the selected 32 columns and all rows
        for (int ix = xStart; ix <= xEnd; ix++) 
        {
            for (int iy = 1; iy <= nBinsY; iy++) 
            {
                double val = h2Thres->GetBinContent(ix, iy);
                if (val != 0) values.push_back(val); // optionally skip empty bins
            }
        }

        // Compute mean and standard deviation
        double sum = 0, sum2 = 0;
        for (auto v : values) {
            sum += v;
            sum2 += v * v;
        }
        double mean = sum / values.size();
        double rms = std::sqrt(sum2 / values.size() - mean * mean);
        std::cout << mean <<"," ;

        if (debug)
        {
            std::cout << std::endl << "Group " << iGroup
                << " (cols " << xStart << "–" << xEnd << "): "
                << "mean = " << mean << ", std = " << rms << std::endl;
        }
    }
    std::cout << std::endl;
} 

// Build file path
void Threshold()
{
    gStyle->SetCanvasPreferGL(kTRUE);
    gROOT->SetStyle("ATLAS");
    // Plot Data
    std::string path = "plotting_scripts/root_input/results_threshold_scan_W12R1_Low.root";
    std::cout << "Opening: " << path << std::endl;

    // Threshold calibration 
    int ITHR = 60;
    double meanThr = 39.6;
    double corrFactor = (8*ITHR + 487) / meanThr;

    TFile *thresholdFile = TFile::Open(path.c_str(), "READ");
    if (!thresholdFile || thresholdFile->IsZombie()) {
        std::cerr << "Could not open file: " << path << std::endl;
        return;
    }
    /*
    TTree *thresholdTree = (TTree*) thresholdFile->Get("threshold_scan");
    if (!thresholdTree) {
        std::cerr << "No summaryTree in file: " << path << std::endl;
        return;
    }
    */

    //TH2D *h2Thres    = new TH2D("h2Thres", "h2Thres", 512, 0, 512, 224, 288, 512);
    //TH1D *h1Thres    = new TH1D("h1Thres", "h1Thres", 100,700,1300);

    TH1D *h1Thres    = new TH1D("h1Thres", "h1Thres", 100,180,270);
    TH2D *h2Thres = (TH2D*) thresholdFile->Get("thres_2D");

    // occupancy map
    TH2D *hOcc = (TH2D*) h2Thres->Clone("hOcc");
    hOcc->Reset();

    for (int ix = 1; ix <= h2Thres->GetNbinsX(); ++ix)
        for (int iy = 1; iy <= h2Thres->GetNbinsY(); ++iy)
            if (h2Thres->GetBinContent(ix,iy) > 0)
                hOcc->SetBinContent(ix,iy,1);

    // rebin both
    //print_colDistr(h2Thres);
    h2Thres->Rebin2D(2,2);
    hOcc->Rebin2D(2,2);

    // divide → mean
    h2Thres->Divide(hOcc);

        int nEntries = h2Thres->GetEntries();
    for (int ix = 1; ix <= h2Thres->GetNbinsX(); ++ix) 
    {
        for (int iy = 1; iy <= h2Thres->GetNbinsY(); ++iy) 
        {
            double val = h2Thres->GetBinContent(ix, iy);
            if (val > 0) h1Thres->Fill(val);
        }
    }

    /*
    int pixX, pixY;
    float scale;
    float threshold;
    thresholdTree->SetBranchAddress("pixX", &pixX);
    thresholdTree->SetBranchAddress("pixY", &pixY);
    thresholdTree->SetBranchAddress("thres", &threshold);
    thresholdTree->SetBranchAddress("scale", &scale);

    int nEntries = thresholdTree->GetEntries();

    
    
    for (int i=0; i < nEntries; i ++)
    {
        thresholdTree->GetEntry(i);
        h2Thres->Fill(pixX, pixY, threshold * corrFactor);
        h1Thres->Fill(threshold * corrFactor);
        //if(threshold > 0) std::cout << "PixX: " << pixX << "; PixY: " << pixY << "Thr: " << threshold << std::endl;
    }
    */


    TCanvas *c1 = new TCanvas("c1","2D Threshold",800,800);
    c1->SetTopMargin(0.53);
    c1->SetRightMargin(0.24);
    TCanvas *c2 = new TCanvas("c2","1D Threshold",800,800);
    c2->SetLeftMargin(0.2);
    TCanvas *c3 = new TCanvas("c3","2D Simu Threshold",800,800);
    c3->SetTopMargin(0.53);
    c3->SetRightMargin(0.24);

    gStyle->SetNumberContours(255);
    gStyle->SetPalette(kBird);

    //TLegend *leg1 = new TLegend(0.65,0.6,0.85,0.75);
    TLegend *leg1 = new TLegend(0.22,0.69,0.42,0.83);
    leg1->SetTextSize(0.05);
    leg1->SetBorderSize(0);

    //path = "Plots/local_0010/Threshold/histos.root";
    path = "Plots/local_0108/Threshold/histos.root";

    TFile *simuThresholdFile = TFile::Open(path.c_str(), "READ");
    if (!simuThresholdFile || simuThresholdFile->IsZombie()) {
        std::cerr << "Could not open file: " << path << std::endl;
        return;
    }
    TDirectory *dir = (TDirectory*)simuThresholdFile->Get("Thr200");
    TH1D *h1DSimuThreshold = (TH1D*) dir->Get("h1DThreshold");
    TH2D *h2DSimuThreshold = (TH2D*) dir->Get("h2DThreshold");
    TH2D *hSimuOcc = (TH2D*) h2DSimuThreshold->Clone("hSimuOcc");
    hSimuOcc->Reset();

    for (int ix = 1; ix <= h2DSimuThreshold->GetNbinsX(); ++ix)
        for (int iy = 1; iy <= h2DSimuThreshold->GetNbinsY(); ++iy)
            if (h2DSimuThreshold->GetBinContent(ix,iy) > 0)
                hSimuOcc->SetBinContent(ix,iy,1);

    // rebin both
    //print_colDistr(h2Thres);
    h2DSimuThreshold->Rebin2D(2,2);
    hSimuOcc->Rebin2D(2,2);

    // divide → mean
    h2DSimuThreshold->Divide(hSimuOcc);

    h1Thres->Scale(1.0 / h1Thres->Integral("width"));
    h1DSimuThreshold->Scale(1.0 / h1DSimuThreshold->Integral("width"));
    //h1DSimuThreshold->GetXaxis()->SetRangeUser(700, 1300);
    h1DSimuThreshold->GetXaxis()->SetRangeUser(180, 270);

    // Data
    h1Thres->SetLineColor(kBlue);
    h1Thres->SetLineWidth(3.2);
    h1Thres->SetMarkerColor(kBlue);
    h1Thres->SetMarkerStyle(20);
    h1Thres->SetMarkerSize(1.4);
    h1Thres->GetXaxis()->SetTitle("Threshold [e#lower[-2.2]{#scale[0.6]{- }}]");
    h1Thres->GetYaxis()->SetTitle("Normalized counts");    
    //h1Thres->GetXaxis()->SetTitleSize(0.04);
    h1Thres->GetYaxis()->SetTitleOffset(2.);
    //h1Thres->GetYaxis()->SetTitleSize(0.04);
    // Simu
    h1DSimuThreshold->SetLineColor(kRed);
    h1DSimuThreshold->SetLineWidth(3.2);
    h1DSimuThreshold->SetMarkerColor(kRed);
    h1DSimuThreshold->SetMarkerStyle(21);
    h1DSimuThreshold->SetMarkerSize(1.6);


    leg1->AddEntry(h1Thres, "Data", "PL");
    leg1->AddEntry(h1DSimuThreshold, "Simulation", "PL");
    c1->cd();  
    //h2Thres->SetMinimum(720);
    //h2Thres->SetMaximum(1190);

    h2Thres->SetMinimum(180);
    h2Thres->SetMaximum(270);
    h2Thres->GetYaxis()->SetRangeUser(288,512);
    h2Thres->GetXaxis()->SetTitle("Pix X");
    h2Thres->GetYaxis()->SetTitle("Pix Y"); 
    h2Thres->GetZaxis()->SetTitle("Threshold [e#lower[-2.2]{#scale[0.6]{- }}]");
    h2Thres->GetXaxis()->SetTitleSize(0.03);
    h2Thres->GetXaxis()->SetTitleOffset(1.1);
    h2Thres->GetXaxis()->SetLabelSize(0.03); 

    h2Thres->GetYaxis()->SetTitleSize(0.03);
    h2Thres->GetYaxis()->SetTitleOffset(1.7);
    h2Thres->GetYaxis()->SetLabelSize(0.03); 

    h2Thres->GetZaxis()->SetTitleSize(0.03);
    h2Thres->GetZaxis()->SetTitleOffset(2.);
    h2Thres->GetZaxis()->SetLabelSize(0.03); 
    h2Thres->GetZaxis()->SetNdivisions(707); 
    
    TLatex *tData = new TLatex();
    tData->SetTextSize(0.035);
    tData->SetNDC();            // coordinates in normalized device coords (0-1)
    h2Thres->Draw("COLZ");
    tData->DrawLatex(0.3, 0.49, "#bf{MALTA2 Data}, 30#mum EPI");
    gPad->Update();  // ensure palette axis is created


    c3->cd();
    h2DSimuThreshold->SetMinimum(180);
    h2DSimuThreshold->SetMaximum(270);
    h2DSimuThreshold->GetYaxis()->SetRangeUser(288,512);
    h2DSimuThreshold->GetXaxis()->SetTitle("Pix X");
    h2DSimuThreshold->GetYaxis()->SetTitle("Pix Y"); 
    h2DSimuThreshold->GetZaxis()->SetTitle("Threshold [e#lower[-2.2]{#scale[0.6]{- }}]");
    h2DSimuThreshold->GetXaxis()->SetTitleSize(0.03);
    h2DSimuThreshold->GetXaxis()->SetTitleOffset(1.1);
    h2DSimuThreshold->GetXaxis()->SetLabelSize(0.03); 

    h2DSimuThreshold->GetYaxis()->SetTitleSize(0.03);
    h2DSimuThreshold->GetYaxis()->SetTitleOffset(1.7);
    h2DSimuThreshold->GetYaxis()->SetLabelSize(0.03); 

    h2DSimuThreshold->GetZaxis()->SetTitleSize(0.03);
    h2DSimuThreshold->GetZaxis()->SetTitleOffset(2.);
    h2DSimuThreshold->GetZaxis()->SetLabelSize(0.03); 
    h2DSimuThreshold->GetZaxis()->SetNdivisions(707); 
    TLatex *tSimu = new TLatex();
    tSimu->SetTextSize(0.035);
    tSimu->SetNDC();            // coordinates in normalized device coords (0-1)
    
    h2DSimuThreshold->Draw("COLZ");
    tSimu->DrawLatex(0.25, 0.49, "#bf{MALTA2 Simulation}, 30#mum EPI");
    c2->cd();

    h1Thres->GetXaxis()->SetNdivisions(806);
    h1Thres->Draw("HIST E P");
    h1Thres->Draw("HIST SAME");
    h1DSimuThreshold->Draw("HIST E P SAME");
    h1DSimuThreshold->Draw("HIST SAME");

    TLatex *t = new TLatex();
    t->SetTextSize(0.05);
    t->SetNDC();            // coordinates in normalized device coords (0-1)
    //t->DrawLatex(0.68, 0.8, "#splitline{#bf{MALTA2}}{30#mum EPI}");
    t->DrawLatex(0.24, 0.87, "#splitline{#bf{MALTA2}}{30#mum EPI}");

    leg1->Draw();

    c1->SaveAs("PublicPlots/Threshold2D_Data.pdf");
    c1->SaveAs("PublicPlots/Threshold2D_Data.C");

    c2->SaveAs("PublicPlots/Threshold1D_SimuvsData.pdf");
    c2->SaveAs("PublicPlots/Threshold1D_SimuvsData.C");

    c3->SaveAs("PublicPlots/Threshold2D_Simu.pdf");
    c3->SaveAs("PublicPlots/Threshold2D_Simu.C");


}
#include <iostream>
#include <ROOT/RNTuple.hxx>
#include <TH3D.h>
#include <TFile.h>
#include <cmath>


double Get2DMean(TH2D *h2D)
{
    double sum = 0;
    int nbins = 0;

    std::cout << h2D->GetNbinsX() << " ; " <<h2D->GetNbinsY() << std::endl;

    for (int i = 1; i <= h2D->GetNbinsX(); ++i) 
    {
        for (int j = 1; j <= h2D->GetNbinsY(); ++j) 
        {

            sum += h2D->GetBinContent(i, j);
            nbins++;
            //std::cout <<sum << " " << nbins <<std::endl;
        }
    }
    return sum / nbins ;
}


void InPixelPlots()
{
    // Simulation in-pixel plots
    std::string path = "Plots/local_0108/DataTiming200/histos.root";
    //std::string path = "Plots/local_0011/offsetX-0.52Y+0.53mu/histos.root";

    TFile *simuThresholdFile = TFile::Open(path.c_str(), "READ");
    if (!simuThresholdFile || simuThresholdFile->IsZombie()) {
        std::cerr << "Could not open file: " << path << std::endl;
        return;
    }
    TDirectory *dir = (TDirectory*)simuThresholdFile->Get("Thr200");
    TH2D *h2PASSInPixel   = (TH2D*) dir->Get("h2PASSInPixel");
    TH2D *h2ClSizeInPixel = (TH2D*) dir->Get("h2ClSizeInPixel");
    TH2D *h2TimingInPixel = (TH2D*) dir->Get("h2TimingInPixel");

    double simuClSizeMean = Get2DMean(h2ClSizeInPixel);
    double simuEffMean = Get2DMean(h2PASSInPixel);
    double simuTimeMean = Get2DMean(h2TimingInPixel) ;
    double simuTimeMin = h2TimingInPixel->GetMinimum();

    for (int i = 1; i <= h2TimingInPixel->GetNbinsX(); ++i) 
    {
        for (int j = 1; j <= h2TimingInPixel->GetNbinsY(); ++j) 
        {
            double old_content = h2TimingInPixel->GetBinContent(i, j);
            h2TimingInPixel->SetBinContent(i, j, old_content - simuTimeMean);
        }
    }


    // Data in-pixel plots
    //// 200 el thr
    path = "plotting_scripts/root_input/Eff_Clsize_W5R23__IDB120_ITHR015_SUB06.0_PWELL06.root";

    /// 1250 el thr
    //path = "plotting_scripts/root_input/W5R23_IBIAS05_rootdatafiles/Eff_Clsize_W5R23__IDB100_ITHR066_SUB06_IBIAS05_PWELL06.root";

    TFile *dataThresholdFile = TFile::Open(path.c_str(), "READ");
    if (!dataThresholdFile || dataThresholdFile->IsZombie()) {
        std::cerr << "Could not open file: " << path << std::endl;
        return;
    }
    TH2D *h2dataPASSInPixel   = (TH2D*) dataThresholdFile->Get("TOT_Eff");
    TH2D *h2dataClSizeInPixel = (TH2D*) dataThresholdFile->Get("TOT_ClSize");
    TH2D *h2dataTimingInPixel = (TH2D*) dataThresholdFile->Get("TOT_ClTime");
    double dataClSizeMean = Get2DMean(h2dataClSizeInPixel);
    double dataEffMean = Get2DMean(h2dataPASSInPixel);
    double dataTimeMean = Get2DMean(h2dataTimingInPixel);
    double dataTimeMin = h2dataTimingInPixel->GetMinimum();
    //std::cout << "Simu mean time: " << simuTimeMean << "; Data mean time: " << dataTimeMean << std::endl;
    std::cout << "Simu mean cl size: " << simuClSizeMean << "; Data mean cl size: " << dataClSizeMean << std::endl;
    std::cout << "Simu mean eff: " << simuEffMean << "; Data mean eff: " << dataEffMean << std::endl;
    //std::cout << "Simu time min: " << simuTimeMin << "; Data time min: " << dataTimeMin << std::endl;

    
    for (int i = 1; i <= h2dataTimingInPixel->GetNbinsX(); ++i) 
    {
        for (int j = 1; j <= h2dataTimingInPixel->GetNbinsY(); ++j) 
        {
            double old_content = h2dataTimingInPixel->GetBinContent(i, j);
            h2dataTimingInPixel->SetBinContent(i, j, old_content - dataTimeMean);
        }
    }
    
    gStyle->SetCanvasPreferGL(kTRUE);
    gROOT->SetStyle("ATLAS");


    h2TimingInPixel->GetZaxis()->SetTitle("Cl time - <Cl time> [ns]");
    h2TimingInPixel->SetMinimum(-2);
    h2TimingInPixel->SetMaximum(3);
    h2dataTimingInPixel->GetZaxis()->SetTitle("Cl time - <Cl time> [ns]");

    h2PASSInPixel->SetMinimum(50);
    h2PASSInPixel->SetMaximum(100);
    h2dataPASSInPixel->SetMinimum(50);
    h2dataPASSInPixel->SetMaximum(100);
    h2ClSizeInPixel->SetMinimum(1);
    h2ClSizeInPixel->SetMaximum(2.4);
    h2dataClSizeInPixel->SetMinimum(1);
    h2dataClSizeInPixel->SetMaximum(2.4);

    TH2D *h2dataPASSInPixelResidual = (TH2D*)h2dataPASSInPixel->Clone("h2dataPASSInPixelCopy");
    h2dataPASSInPixelResidual->Add(h2PASSInPixel, -1.0);
    //h2dataPASSInPixelResidual->Divide(h2PASSInPixel);

    TH2D *h2dataClSizeInPixelResidual = (TH2D*)h2dataClSizeInPixel->Clone("h2dataClSizeInPixelCopy");
    h2dataClSizeInPixelResidual->Add(h2ClSizeInPixel, -1.0);
    h2dataClSizeInPixelResidual->Divide(h2ClSizeInPixel);

    //TH2D *h2dataTimingInPixelResidual = (TH2D*)h2dataTimingInPixel->Clone("h2dataTimingInPixelCopy");
    //h2dataTimingInPixelResidual->Add(h2TimingInPixel, -1.0);
    //h2dataTimingInPixelResidual->Divide(h2TimingInPixel);    

    /*
    for (int i = 1; i <= h2dataTimingInPixelResidual->GetNbinsX(); ++i) 
    {
        for (int j = 1; j <= h2dataTimingInPixelResidual->GetNbinsY(); ++j) 
        {
            double old_content = h2dataTimingInPixelResidual->GetBinContent(i, j);
            double to_divide = h2TimingInPixel->GetBinContent(i, j);
            h2dataTimingInPixelResidual->SetBinContent(i, j, (old_content /to_divide + 1e-6 )*100);
            //std::cout << "binx: " << i << "; biny: " << j << "old_val: " << old_content << "; to_divide: " << to_divide << "; new_val: " << (old_content /to_divide + 1e-6 )*100 << std::endl;
        }
    }
    */

    /*
    h2dataTimingInPixelResidual->SetMinimum(-300);
    h2dataTimingInPixelResidual->SetMaximum(300);
    h2dataTimingInPixelResidual->GetZaxis()->SetTitle("#frac{Data - Sim}{Sim} [%]");
    */

    for (int i = 1; i <= h2dataClSizeInPixelResidual->GetNbinsX(); ++i) 
    {
        for (int j = 1; j <= h2dataClSizeInPixelResidual->GetNbinsY(); ++j) 
        {
            double old_content = h2dataClSizeInPixelResidual->GetBinContent(i, j);
            h2dataClSizeInPixelResidual->SetBinContent(i, j, old_content *100);
        }
    }
    TH2F *h2dataClSizeInPixelResidual_rebinned = (TH2F*)h2dataClSizeInPixelResidual->Rebin2D(1, 1);

    h2dataClSizeInPixelResidual_rebinned->SetMinimum(-20);
    h2dataClSizeInPixelResidual_rebinned->SetMaximum(20);
    h2dataClSizeInPixelResidual_rebinned->GetZaxis()->SetTitle("#frac{Data - Sim}{Sim} [%]");
    /*
    for (int i = 1; i <= h2dataPASSInPixelResidual->GetNbinsX(); ++i) 
    {
        for (int j = 1; j <= h2dataPASSInPixelResidual->GetNbinsY(); ++j) 
        {
            double old_content = h2dataPASSInPixelResidual->GetBinContent(i, j);
            h2dataPASSInPixelResidual->SetBinContent(i, j, old_content *100);
        }
    }
    */
    //h2dataPASSInPixelResidual->SetMinimum(-12);
    //h2dataPASSInPixelResidual->SetMaximum(12);
    h2dataPASSInPixelResidual->GetZaxis()->SetTitle("Data - Sim [%]");

    TCanvas *c1 = new TCanvas("c1","SIMU In-pix Eff",800,800);
    c1->SetTopMargin(0.10);    // leave space for title
    c1->SetBottomMargin(0.12); // leave space for X axis labels
    c1->SetLeftMargin(0.2);   // leave space for Y axis labels
    c1->SetRightMargin(0.2);  // keep right side small for square plot
    TCanvas *c2 = new TCanvas("c2","SIMU In-pix ClSize",800,800);
    c2->SetRightMargin(0.2);
    TCanvas *c3 = new TCanvas("c3","SIMU In-pix Timing",800,800);
    c3->SetRightMargin(0.2);
    TCanvas *c4 = new TCanvas("c4","DATA In-pix Eff",800,800);
    c4->SetTopMargin(0.10);    // leave space for title
    c4->SetBottomMargin(0.12); // leave space for X axis labels
    c4->SetLeftMargin(0.2);   // leave space for Y axis labels
    c4->SetRightMargin(0.2);  // keep right side small for square plot
    TCanvas *c5 = new TCanvas("c5","DATA In-pix ClSize",800,800);
    c5->SetRightMargin(0.2);
    TCanvas *c6 = new TCanvas("c6","DATA In-pix Timing",800,800);
    c6->SetRightMargin(0.2);
    TCanvas *c7 = new TCanvas("c7","DATA - SIMU Cl Size",800,800);
    c7->SetRightMargin(0.2);
    TCanvas *c8 = new TCanvas("c8","DATA - SIMU Timing",800,800);
    c8->SetRightMargin(0.2);
    TCanvas *c9 = new TCanvas("c9","DATA - SIMU Eff",800,800);
    c9->SetTopMargin(0.10);    // leave space for title
    c9->SetBottomMargin(0.12); // leave space for X axis labels
    c9->SetLeftMargin(0.2);   // leave space for Y axis labels
    c9->SetRightMargin(0.2);  // keep right side small for square plot



    double xMin = h2dataPASSInPixelResidual->GetXaxis()->GetXmin();
    double xMax = h2dataPASSInPixelResidual->GetXaxis()->GetXmax();
    double yMin = h2dataPASSInPixelResidual->GetYaxis()->GetXmin();
    double yMax = h2dataPASSInPixelResidual->GetYaxis()->GetXmax();
    double xMid = 0.5*(xMin + xMax);
    double yMid = 0.5*(yMin + yMax);

    c1->cd();
    h2PASSInPixel->Draw("COLZ");
    // Draw vertical line at xMid
    TLine *lineX1 = new TLine(xMid, yMin, xMid, yMax);
    lineX1->SetLineColor(kBlack);
    lineX1->SetLineWidth(3);
    lineX1->Draw("same");

    // Draw horizontal line at yMid
    TLine *lineY1 = new TLine(xMin, yMid, xMax, yMid);
    lineY1->SetLineColor(kBlack);
    lineY1->SetLineWidth(3);
    lineY1->Draw("same");

    TLatex *t1 = new TLatex();
    t1->SetTextSize(0.05);
    t1->SetNDC();
    t1->DrawLatex(0.22, 0.93, "#bf{Simulation} In-pixel eff. #bf{MALTA2} 30#mum EPI.");

    c2->cd();
    h2ClSizeInPixel->Draw("COLZ");
    c3->cd();
    h2TimingInPixel->Draw("COLZ");
    c4->cd();
    h2dataPASSInPixel->Draw("COLZ");
    // Draw vertical line at xMid
    TLine *lineX4 = new TLine(xMid, yMin, xMid, yMax);
    lineX4->SetLineColor(kBlack);
    lineX4->SetLineWidth(3);
    lineX4->Draw("same");

    // Draw horizontal line at yMid
    TLine *lineY4 = new TLine(xMin, yMid, xMax, yMid);
    lineY4->SetLineColor(kBlack);
    lineY4->SetLineWidth(3);
    lineY4->Draw("same");

    TLatex *t4 = new TLatex();
    t4->SetTextSize(0.05);
    t4->SetNDC();
    t4->DrawLatex(0.22, 0.93, "#bf{Data} In-pixel eff. #bf{MALTA2} 30#mum EPI.");
    c5->cd();
    h2dataClSizeInPixel->Draw("COLZ");
    c6->cd();
    h2dataTimingInPixel->Draw("COLZ");
    c7->cd();
    h2dataClSizeInPixelResidual_rebinned->Draw("COLZ");
    c8->cd();
    //h2dataTimingInPixelResidual->Draw("COLZ");
    c9->cd();
    h2dataPASSInPixelResidual->Draw("COLZ");
    
    // Draw vertical line at xMid
    TLine *lineX = new TLine(xMid, yMin, xMid, yMax);
    lineX->SetLineColor(kBlack);
    lineX->SetLineWidth(3);
    lineX->Draw("same");
    // Draw horizontal line at yMid
    TLine *lineY = new TLine(xMin, yMid, xMax, yMid);
    lineY->SetLineColor(kBlack);
    lineY->SetLineWidth(3);
    lineY->Draw("same");

    TLatex *t9 = new TLatex();
    t9->SetTextSize(0.05);
    t9->SetNDC();
    t9->DrawLatex(0.22, 0.93, "#bf{Residual} In-pixel eff. #bf{MALTA2} 30#mum EPI.");



}
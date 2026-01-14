#include <iostream>
#include <ROOT/RNTuple.hxx>
#include <TH3D.h>
#include <TFile.h>
#include <cmath>

void ManualPlot()
{
    gStyle->SetCanvasPreferGL(kTRUE);
    gROOT->SetStyle("ATLAS");

    TCanvas *c1 = new TCanvas("c1","Residual vs XOffset",800,800);
    TCanvas *c2 = new TCanvas("c2","Residual vs YOffset",800,800);

    std::vector<double> YOffset = {-1, -0.9, -0.8, -0.7, -0.6, -0.5, -0.4, -0.3, -0.2, -0.1, 0., 
                                0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1., 1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7, 1.8, 1.9, 2.};

    std::vector<double> YResidual = {5.68, 5.65, 5.51, 5.4, 5.31, 5.25, 5.16, 5.12, 4.99, 4.97, 4.94, 4.88, 4.83, 4.86, 4.83, 4.79, 4.81, 4.84,
                                4.81, 4.9, 4.86, 4.92, 4.99, 5.07, 5.14, 5.16, 5.25, 5.41, 5.42, 5.56, 5.62};

    std::vector<double> XOffset = {-2, -1.9, -1.8, -1.7, -1.6, -1.5, -1.4, -1.3, -1.2, -1.1, -1., -0.9, -0.8, -0.7, -0.6, -0.5, -0.4, -0.3, -0.2, -0.1,
                                0., 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.};

    std::vector<double> XResidual = {5.67, 5.58, 5.41, 5.32, 5.29, 5.22, 5.1, 5.04, 4.99, 4.95, 4.87, 4.86, 4.81, 4.85, 4.89, 4.81, 4.8, 4.85, 
                                    4.82, 4.87, 4.94, 4.95, 5.04, 5.14, 5.18, 5.19, 5.33, 5.39, 5.48, 5.57, 5.7};

    TGraphErrors *gXOffset = new TGraphErrors(XOffset.size(), XOffset.data(), XResidual.data());

    TGraphErrors *gYOffset = new TGraphErrors(YOffset.size(), YOffset.data(), YResidual.data());

    c1->cd();
    //gStyle->SetOptFit(1111); // Show detailed fit results box
    // Fit parabolas to data
    auto fparabolaX = new TF1("fparabolaX", "[0]*x*x + [1]*x + [2]", -2., 2.);
    fparabolaX->SetLineColor(kRed);
    fparabolaX->SetLineWidth(3);
    gXOffset->Fit(fparabolaX);
    gXOffset->Draw("P AX");
    double p1X = - fparabolaX->GetParameter(1) / (2 * fparabolaX->GetParameter(0));

    TLatex *tX = new TLatex();
    tX->SetTextSize(0.05);
    tX->SetNDC();
    tX->DrawLatex(0.33, 0.87, Form("Tracking offset: %.2f #mum",p1X));  
    
    gXOffset->GetXaxis()->SetTitle("X Tracking Offset [#mum]");
    gXOffset->GetYaxis()->SetTitle("Mean Absolute Relative Residual [%]");
    
    c2->cd();
    auto fparabolaY = new TF1("fparabolaY", "[0]*x*x + [1]*x + [2]", -2., 2.);
    fparabolaY->SetLineColor(kRed);
    fparabolaY->SetLineWidth(3);
    gYOffset->Fit(fparabolaY);
    gYOffset->Draw("P AX");
    double p1Y = - fparabolaY->GetParameter(1) / (2 * fparabolaY->GetParameter(0));

    TLatex *tY = new TLatex();
    tY->SetTextSize(0.05);
    tY->SetNDC();
    tY->DrawLatex(0.33, 0.87, Form("Tracking offset: %.2f #mum",p1Y));        

    gYOffset->GetXaxis()->SetTitle("Y Tracking Offset [#mum]");
    gYOffset->GetYaxis()->SetTitle("Mean Absolute Relative Residual [%]");
    
    c1->SaveAs("PublicPlots/XTrackOffsetResidual.pdf");
    c1->SaveAs("PublicPlots/XTrackOffsetResidual.C");

    c2->SaveAs("PublicPlots/YTrackOffsetResidual.pdf");
    c2->SaveAs("PublicPlots/YTrackOffsetResidual.C");
}

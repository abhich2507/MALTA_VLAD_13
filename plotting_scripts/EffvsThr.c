#include <iostream>
#include <ROOT/RNTuple.hxx>
#include <TH3D.h>
#include <TFile.h>
#include <cmath>
#include "ROOTTHelperFunctions.h"

void EffvsThr()
{

    gStyle->SetCanvasPreferGL(kTRUE);
    gROOT->SetStyle("ATLAS");

    std::vector<int> runNumbers= {1};
    std::vector<std::string> runFiles = {"Final"};

    std::vector<std::string> labels = {"Simulation"};

    //std::vector<std::string> labels = {"Perfect matching", "Real matching", "Slow matching"};
    std::string branch = "MultiEff";
    GraphData dataStruct = get_mergedData(branch);

    TGraph* effDataMerged = dataStruct.graphs;
    std::vector <double> dataThr = dataStruct.x;
    for (auto& el: dataThr)
    {
        std::cout << el << std::endl;
    }
    std::vector <double> dataEff = dataStruct.y;
    std::vector <double> dataThrErr = dataStruct.xErr;
    std::vector <double> dataEffErr = dataStruct.yErr;
    int nMergedEff = dataStruct.num;

    TCanvas *c1 = new TCanvas("c1","Efficiency vs Threshold",800,800);
    c1->SetRightMargin(0.15);
    c1->SetLeftMargin(0.15);
    c1->SetTopMargin(0.10);
    c1->SetBottomMargin(0.12);
    c1->Divide(1, 2); // optional — we’ll do it manually for full control
    TPad *pad1 = new TPad("pad1", "Main Plot", 0.04, 0.30, 1, 1.00); // top 70%
    TPad *pad2 = new TPad("pad2", "Residuals", 0.04, 0.00, 1, 0.30); // bottom 30%
    pad1->SetBottomMargin(0.02); // small space between panels
    pad1->SetLeftMargin(0.12);
    pad1->SetRightMargin(0.06);
    pad2->SetTopMargin(0.02);
    pad2->SetBottomMargin(0.35);
    pad2->SetLeftMargin(0.12);
    pad2->SetRightMargin(0.06);
    pad1->Draw();
    pad2->Draw();

    TLegend *leg1 = new TLegend(0.15,0.1,0.5,0.5);
    leg1->SetTextSize(0.06);
    leg1->SetBorderSize(0);

    // Plot Contours for all erorr bars
    TGraph *band = new TGraph(2 * nMergedEff);  
    // Upper edge (forward)
    for (int i = 0; i < nMergedEff; ++i)
        band->SetPoint(i, dataThr[i] + dataThrErr[i], dataEff[i]);
    // Lower edge (backward)
    for (int i = 0; i < nMergedEff; ++i)
        band->SetPoint(2 * nMergedEff - 1 - i, dataThr[i] - dataThrErr[i], dataEff[i]);
    // Ensure the band is closed
    band->SetPoint(2 * nMergedEff, band->GetPointX(0), band->GetPointY(0));
    band->SetLineColor(kBlue);
    band->SetFillColorAlpha(kBlue, 0.25);
    band->SetLineWidth(1);

    c1->cd();
    pad1->cd();
    effDataMerged->SetMarkerStyle(29);
    effDataMerged->SetMarkerSize(3.4);
    effDataMerged->SetMarkerColor(kBlue);
    effDataMerged->SetLineColor(kBlue);
    effDataMerged->SetLineWidth(2.4);
    effDataMerged->Draw("AP SAME");
    effDataMerged->GetHistogram()->GetXaxis()->SetRangeUser(0, 2000);
    gPad->Modified();
    gPad->Update();
    effDataMerged->SetTitle("Efficiency vs Threshold;;Efficiency [%]");
    //effDataMerged->GetYaxis()->SetLabelOffset(0.01);
    effDataMerged->GetYaxis()->SetTitleOffset(1.1);
    effDataMerged->GetXaxis()->SetLabelSize(0);

    band->Draw("F"); 
    //band2->Draw("F");
    TLatex *t = new TLatex();
    t->SetTextSize(0.06);
    t->SetNDC();            // coordinates in normalized device coords (0-1)
    t->DrawLatex(0.7, 0.78, "#splitline{#bf{MALTA2}}{30#mum EPI}");


    int colorIndex = 1; // ROOT color index (kBlue=4, kRed=2, kGreen=3, etc.)

    for (const std::string &tag : runFiles) 
    {

        for (const auto &runNumber : runNumbers)
        {
            // Process Simulation data
            std::vector<double> vThr, vThrErr, vThrNoErr, vEff, vEffErr, vClSize, vClSizeErr, vTiming;
            ProcessedSimulation simValues = sort_simTree(runNumber, tag);
            vThr = simValues.thr;
            vThrErr = simValues.thrErr;
            vThrNoErr = simValues.thrNoErr;
            vEff = simValues.eff;
            vEffErr = simValues.effErr;
            vClSize = simValues.clSize;
            vClSizeErr = simValues.clSizeErr;
            vTiming = simValues.timing;
            TTree* sortedSummary = simValues.tree;
            int numValidEntries = simValues.num;
            // Compute residuals + Uncertainty
            Residuals resValues = compute_residuals(sortedSummary, dataStruct, branch);
            std::vector<double> residuals = resValues.res;
            std::vector<double> uncResiduals = resValues.resUnc;

            // Graphs
            TGraphErrors *gThrVSEff = new TGraphErrors(numValidEntries, vThr.data(), vEff.data(), vThrNoErr.data(), vEffErr.data());
            
            //gThrVSEff->SetTitle("Efficiency vs Threshold;Threshold [e-];Tracking efficiency [%]");
            gThrVSEff->SetMarkerStyle(21);
            gThrVSEff->SetMarkerSize(2.);
            gThrVSEff->SetLineWidth(2.5);
            gThrVSEff->SetMarkerColor(kRed); //colorIndex
            gThrVSEff->SetLineColor(kRed);
            
            leg1->AddEntry(gThrVSEff, labels[colorIndex-1].c_str(), "lp");
            leg1->AddEntry(effDataMerged, "Data", "p");
            leg1->AddEntry(band, "Data #pm 1 #sigma", "f");
            // Draw on same canvases
            c1->cd();    
            pad1->cd();
            if (colorIndex == 1) gThrVSEff->Draw("PL");
            else gThrVSEff->Draw("PL SAME");     
            // Plot residuals
            pad2->cd();

            //TH1F *frame = pad2->DrawFrame(0, -15., 2000, 15., ";Threshold [e#lower[-2.2]{#scale[0.6]{- }}];#frac{Data - Sim}{Sim} [%]");
            TH1F *frame = pad2->DrawFrame(0, -15., 2000, 15., ";Threshold [e#lower[-2.2]{#scale[0.6]{- }}];Rel diff [%]");
            //TH1F *frame = pad2->DrawFrame(0, -15., 2000, 15., ";Threshold [e#lower[-2.2]{#scale[0.6]{- }}];Data - Sim [%]");
            //TH1F *frame = pad2->DrawFrame(0, 0.85, 2000, 1.15, ";Threshold [e#lower[-2.2]{#scale[0.6]{- }}];#frac{Data}{Sim} [%]");
            frame->GetXaxis()->SetTitleSize(0.12);
            frame->GetXaxis()->SetTitleOffset(1.);
            frame->GetXaxis()->SetLabelSize(0.11);
            frame->GetYaxis()->SetTitleSize(0.12);
            frame->GetYaxis()->SetLabelSize(0.11);
            frame->GetYaxis()->SetTitleOffset(0.38);
            frame->GetYaxis()->SetNdivisions(505); 

            TGraphErrors *gEffResiduals = new TGraphErrors(dataThr.size(), dataThr.data(), residuals.data(), vThrNoErr.data(), uncResiduals.data());
            gEffResiduals->Draw("P");
            TLine *line = new TLine(0, 0.0, 2000, 0.0);
            line->SetLineColor(kBlack);
            line->SetLineStyle(2);   // dashed line
            line->SetLineWidth(2);
            line->Draw("SAME");

            // Increase color for next file
            colorIndex++;
        }
    }
    // Draw legends
    c1->cd(); 
    pad1->cd();
    leg1->Draw();
    c1->SaveAs("PublicPlots/EfficiencyvsThreshold.pdf");
    c1->SaveAs("PublicPlots/EfficiencyvsThreshold.C");

}
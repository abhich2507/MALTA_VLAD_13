#include <iostream>
#include <ROOT/RNTuple.hxx>
#include <TH3D.h>
#include <TFile.h>
#include <cmath>
#include "ROOTTHelperFunctions.h"

void EffvsThr_Digital()
{

    gStyle->SetCanvasPreferGL(kTRUE);
    gROOT->SetStyle("ATLAS");

    std::vector<int> runNumbers= {70};
    //std::vector<std::string> runFiles = { "wordSpacingScan0ns", "wordSpacingScan0.5ns", "wordSpacingScan1ns","wordSpacingScan1.6ns"};
    //std::vector<std::string> labels = {"0ns merging", "0.5ns merging", "1ns merging", "1.6ns merging"};

    //std::vector<std::string> runFiles = { "RO8ns", "RO10ns", "RO20ns", "RO100ns"};//, "RO300ns", "RO500ns", "RO1000ns"};
    //std::vector<std::string> labels = { "Veto8ns", "Veto10ns", "Veto20ns", "Veto100ns"};//, "RO300ns", "RO500ns", "RO1000ns"};

    //std::vector<std::string> runFiles = { "RO2x8", "RO2x16", "RO2x32", "RO8x8"};
    //std::vector<std::string> labels = { "RO2x8", "RO2x16", "RO2x32", "RO8x8"};

    std::vector<std::string> runFiles = { "RO2x32", "RO2x8", "RO32x2", "RO8x2"};
    std::vector<std::string> labels = { "RO2x32", "RO2x8", "RO32x2", "RO8x2"};

    //std::vector<std::string> labels = {"Perfect matching", "Real matching", "Slow matching"};

    TCanvas *c1 = new TCanvas("c1","Efficiency vs Threshold",800,800);
    //c1->SetRightMargin(0.15);
    c1->SetLeftMargin(0.17);
    c1->SetTopMargin(0.10);
    //c1->SetBottomMargin(0.12);
    c1->cd();
    TLegend *leg1 = new TLegend(0.55,0.2,0.83,0.5);
    leg1->SetTextSize(0.04);
    leg1->SetBorderSize(0);


    int colorIndex = 1; // ROOT color index (kBlue=4, kRed=2, kGreen=3, etc.)

    for (const std::string &tag : runFiles) 
    {

        for (const auto &runNumber : runNumbers)
        {
            std::cout << tag << "; " << runNumber << std::endl;

            // Process Simulation data
            std::vector<double> vThr, vThrErr, vThrNoErr, vEff, vEffErr, vClSize, vClSizeErr, vTiming;
            ProcessedSimulation simValues = sort_simTree_Digital(runNumber, tag);
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
            std::cout << numValidEntries << std::endl;

            // Graphs
            TGraphErrors *gThrVSEff = new TGraphErrors(numValidEntries, vThr.data(), vEff.data(), vThrNoErr.data(), vEffErr.data());
            
            gThrVSEff->SetTitle(";Threshold [e#lower[-2.2]{#scale[0.6]{- }}];Tracking efficiency [%]");
            gThrVSEff->SetMarkerStyle(21);
            gThrVSEff->SetMarkerSize(2.);
            gThrVSEff->SetLineWidth(5);
            gThrVSEff->SetMarkerColor(colorIndex); //colorIndex
            gThrVSEff->SetLineColor(colorIndex);
            gThrVSEff->GetXaxis()->SetTitleOffset(1.2);
            gThrVSEff->GetYaxis()->SetTitleOffset(1.7);
            
            leg1->AddEntry(gThrVSEff, labels[colorIndex-1].c_str(), "lp");
            // Draw on same canvases
            //c1->cd();    
            std::cout << colorIndex << std::endl;

            c1->cd();
            if (colorIndex == 1) 
            {
                gThrVSEff->Draw("AP"); // first one draws axes
                gThrVSEff->SetMinimum(98.5);  // or some min
                gThrVSEff->SetMaximum(100.1);  // or some max
                //gThrVSEff->SetMinimum(80);  // or some min
                //gThrVSEff->SetMaximum(100.1);  // or some max
            }
            else 
            {
                std::cout << "P SAME!"<< std::endl;
                gThrVSEff->Draw("P SAME");
                std::cout << "Passed drawing" << std::endl;
            }

            // Increase color for next file
            colorIndex++;
        }
    }
    // Draw legends
    //c1->cd(); 
    leg1->Draw();
    TLatex *t = new TLatex();
    t->SetTextSize(0.049);
    t->SetNDC();
    t->DrawLatex(0.25, 0.92, "#bf{MALTA2 Simulation}, 30#mum EPI");
    c1->SaveAs("PublicPlots/EffThr_MergingTime.pdf");
    c1->SaveAs("PublicPlots/EffThr_MergingTime.C");

}
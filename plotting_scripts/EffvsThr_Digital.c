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

    std::vector<int> runNumbers= {70}; //112
    //std::vector<std::string> runFiles = { "wordSpacingScan0ns", "wordSpacingScan0.5ns", "wordSpacingScan1ns","wordSpacingScan1.6ns"};
    //std::vector<std::string> labels = {"No Merging", "0.5 ns merging", "1.0 ns merging", "1.6 ns merging"};
    //std::vector<int> vcolor = {kBlack, kAzure -3, kTeal +2, kOrange +7};
    //std::vector<int> vmarkerStyle = {20, 21, 22, 23};

    //std::vector<std::string> runFiles = { "RO8ns", "RO10ns", "RO20ns", "RO100ns"};//, "RO300ns", "RO500ns", "RO1000ns"};
    //std::vector<std::string> labels = { "Veto8ns", "Veto10ns", "Veto20ns", "Veto100ns"};//, "RO300ns", "RO500ns", "RO1000ns"};

    //std::vector<std::string> runFiles = {"wordSpacingScan0ns", "RO2x8", "RO2x16", "RO2x32", "RO8x8"};
    //std::vector<std::string> labels = { "No Merging", "2 #times 8   pixel group", "2 #times 16 pixel group", "2 #times 32 pixel group", "8 #times 8   pixel group"};//, "RO300ns", "RO500ns", "RO1000ns"};
    //std::vector<int> vcolor = {kBlack, kRed, kBlue, kMagenta +2, kOrange -3};
    //std::vector<int> vmarkerStyle = {20, 21, 22, 23, 47};

    //std::vector<std::string> runFiles = {"wordSpacingScan0ns", "RO2x32", "RO2x8", "RO32x2", "RO8x2"};
    //std::vector<std::string> labels = { "No Merging", "2 #times 32 pixel group", "2 #times 8   pixel group", "32 #times 2 pixel group", "8 #times 2   pixel group"};
    
    //std::vector<std::string> runFiles = {"wordSpacingScan0ns", "RO2x32", "RO4x16", "RO32x2", "RO8x8"};
    //std::vector<std::string> labels = { "No Merging", "2 #times 32 pixel group", "4 #times 16   pixel group","32 #times 2 pixel group", "8 #times 8   pixel group"};
    
    std::vector<std::string> runFiles = {"RO2x32", "RO4x16", "RO8x8", "RO16x4", "RO32x2"};
    std::vector<std::string> labels = {"2#times32", "4#times16", "8#times8", "16#times4", "32#times2"};


    //std::vector<std::string> runFiles = {"MERGETrack", "FIFOTrack"};
    //std::vector<std::string> labels = {"MALTA2", "MALTA3"};

    //std::vector<std::string> runFiles = {"Veto10ns", "Veto8ns", "Veto7ns", "Veto6ns"};
    //std::vector<std::string> labels = {"100MHz beam rate", "125MHz beam rate", "142MHz beam rate", "166MHz beam rate"};

    std::vector<int> vcolor = {kBlack, kMagenta +2, kRed, kAzure -3, kGreen +3};
    std::vector<int> vmarkerStyle = {20, 23, 21, 23, 21};

    //std::vector<std::string> labels = {"Perfect matching", "Real matching", "Slow matching"};

    TCanvas *c1 = new TCanvas("c1","Efficiency vs Threshold",800,800);
    TCanvas *c2 = new TCanvas("c2","Error Rate vs Config",800,800);
    TCanvas *c3 = new TCanvas("c3","Relative Inefficiency vs Config",800,800);
    //c1->SetRightMargin(0.15);
    c1->SetLeftMargin(0.17);
    c1->SetTopMargin(0.10);
    //c1->SetBottomMargin(0.12);
    c1->cd();
    TLegend *leg1 = new TLegend(0.53,0.19,0.83,0.47);
    leg1->SetTextSize(0.04);
    leg1->SetBorderSize(0);


    int colorIndex = 1; // ROOT color index (kBlue=4, kRed=2, kGreen=3, etc.)
    std::vector<double> verrRate{};
    std::vector<double> veta{};
    double normErrRate{}, normEta{};

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

            // Get the error rate directly from the histogramed data
            std::string path = Form("Plots/local_%04d/%s/histos.root", runNumber, tag.c_str());
            std::cout << "Opening: " << path << std::endl;
            TFile* file = TFile::Open(path.c_str());
            TH2* h2Derr = (TH2*) file->Get("Thr200/h2MissMerged");
            TH2* h2All  = (TH2*) file->Get("Thr200/h2DUTHits");
            int numErrEntries = h2Derr->GetEntries();
            int numAllEntries = h2All->GetEntries();
            double errRate = (double) numErrEntries / numAllEntries;
            verrRate.push_back(errRate);
            veta.push_back(vEff[5]);
            std::cout << "veta: " << vEff[5] << std::endl;
            //std::cout << "Error Entries: " << numErrEntries << " ; All entries: " <<numAllEntries << std::endl;
            //std::cout << "Error rate: " << errRate << std::endl;
            if (tag == "RO8x8") 
            {
                normErrRate = errRate;
                normEta = vEff[5];
            }

            // Graphs
            TGraphErrors *gThrVSEff = new TGraphErrors(numValidEntries, vThr.data(), vEff.data(), vThrNoErr.data(), vEffErr.data());
            
            gThrVSEff->SetTitle(";Threshold [e#lower[-2.2]{#scale[0.6]{- }}];Tracking efficiency [%]");
            gThrVSEff->SetMarkerStyle(vmarkerStyle[colorIndex -1]);
            gThrVSEff->SetMarkerSize(2.);
            gThrVSEff->SetLineWidth(5);
            gThrVSEff->SetMarkerColor(vcolor[colorIndex - 1]); //colorIndex
            gThrVSEff->SetLineColor(vcolor[colorIndex - 1]);
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
    c2->cd();
    std::vector<double> x;

    for (size_t i = 0; i < verrRate.size(); ++i) 
    {   
        verrRate[i] /= normErrRate;
        x.push_back(i);
    }
    TGraph* gErrRate = new TGraph (verrRate.size(), x.data(), verrRate.data());
    gErrRate->Draw("APL");
    gErrRate->GetXaxis()->Set(verrRate.size(), -0.5, verrRate.size()-0.5);
    gErrRate->SetTitle(";Grouping; Relative Error Rate");

    for (size_t i = 0; i < labels.size(); ++i)
    {
        gErrRate->GetXaxis()->SetBinLabel(i+1, labels[i].c_str());
    }

    gErrRate->GetXaxis()->LabelsOption("v");  // vertical labels
    gPad->Modified();
    gPad->Update();

    c3->cd();
    std::vector<double> x2;

    for (size_t i = 0; i < veta.size(); ++i) 
    {   
        veta[i] /= normEta;
        x2.push_back(i);
    }
    TGraph* gEta = new TGraph (veta.size(), x2.data(), veta.data());
    gEta->Draw("APL");
    gEta->GetXaxis()->Set(veta.size(), -0.5, veta.size()-0.5);
    gEta->SetTitle(";Grouping; Relative Eff.");

    for (size_t i = 0; i < labels.size(); ++i)
    {
        gEta->GetXaxis()->SetBinLabel(i+1, labels[i].c_str());
    }

    gEta->GetXaxis()->LabelsOption("v");  // vertical labels
    gPad->Modified();
    gPad->Update();

    // Draw legends
    c1->cd(); 
    double x0 = 0.65;
    double y0 = 0.5;


    leg1->Draw();
    TLatex *t = new TLatex();
    t->SetTextSize(0.049);
    t->SetNDC();
    t->DrawLatex(0.25, 0.92, "#bf{MALTA2 Simulation}, 30#mum EPI");
    c1->SaveAs("PublicPlots/EffThr_GroupParityV2.pdf");
    c1->SaveAs("PublicPlots/EffThr_GroupParityV2.C");

}
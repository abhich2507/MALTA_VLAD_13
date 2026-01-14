#include <iostream>
#include <ROOT/RNTuple.hxx>
#include <TH3D.h>
#include <TFile.h>
#include <cmath>
#include "ROOTTHelperFunctions.h"

void Calorimetry()
{

    gStyle->SetCanvasPreferGL(kTRUE);
    gROOT->SetStyle("ATLAS");

    std::vector<int> runNumbers= {83,84,85,86,87,88,89,90,91,92};
    //std::vector<std::string> runFiles = { "wordSpacingScan0ns", "wordSpacingScan0.5ns", "wordSpacingScan1ns","wordSpacingScan1.6ns"};
    //std::vector<std::string> labels = {"0ns merging", "0.5ns merging", "1ns merging", "1.6ns merging"};

    std::vector<std::string> runFiles = { "CaloNoMerg", "CaloYesMerg", "8x8Merging"};//, "RO300ns", "RO500ns", "RO1000ns"};
    std::vector<std::string> labels = { "0ns merging", "1.6ns merging", "1.6ns merging 8x8 group"};//, "RO300ns", "RO500ns", "RO1000ns"};
    std::vector<double> venergy = {5,10,15,20,25,30,35,50,75,100};
    std::vector<double> venergyErr(runNumbers.size(), 0.0);

    //std::vector<std::string> labels = {"Perfect matching", "Real matching", "Slow matching"};
    TCanvas *c1 = new TCanvas("c1","numSecondaries",800,800);
    //c1->SetRightMargin(0.15);
    c1->SetLeftMargin(0.17);
    c1->SetTopMargin(0.10);
    //c1->SetBottomMargin(0.12);
    c1->cd();
    TLegend *leg1 = new TLegend(0.55,0.2,0.83,0.5);
    leg1->SetTextSize(0.04);
    leg1->SetBorderSize(0);

    TLatex *t = new TLatex();
    t->SetTextSize(0.049);
    t->SetNDC();
    t->DrawLatex(0.25, 0.92, "#bf{MALTA2 Simulation}, 30#mum EPI");
    

    int colorIndex = 0; // ROOT color index (kBlue=4, kRed=2, kGreen=3, etc.)

    for (const std::string &tag : runFiles) 
    {
        std::vector<double> vmean;
        std::vector<double> vmeanErr;
        for (const auto &runNumber : runNumbers)
        {
            std::cout << tag << "; " << runNumber << std::endl;            
            
            std::string summaryPath = Form("Results/local_%04d/%s/CalorimetryThr200.root", runNumber, tag.c_str());
            std::cout << "Opening: " << summaryPath << std::endl;

            TFile *summaryFile = TFile::Open(summaryPath.c_str(), "READ");
            if (!summaryFile || summaryFile->IsZombie()) {
                std::cerr << "Could not open file: " << summaryPath << std::endl;
                return;
            }

            TTree *summaryTree = (TTree*) summaryFile->Get("CaloHits");
            if (!summaryTree) {
                std::cerr << "No summaryTree in file: " << summaryPath << std::endl;
                return;
            }

            Long64_t nSummaryEntries = summaryTree->GetEntries();
            int numSecondaries;
            std::vector<int> histoSecondaries;
            histoSecondaries.reserve(nSummaryEntries);
            summaryTree->SetBranchAddress("numSecondaries", &numSecondaries);
            for (Long64_t i = 0; i < nSummaryEntries; i++)
            {
                summaryTree->GetEntry(i);
                histoSecondaries.push_back(numSecondaries);
            }

            double min = *std::min_element(histoSecondaries.begin(), histoSecondaries.end());
            double max = *std::max_element(histoSecondaries.begin(), histoSecondaries.end());

            TH1D *hSecondaries = new TH1D("hSecondaries",";Hits per Event;Counts", 50, min, max);

            for (double val : histoSecondaries) 
            {
                hSecondaries->Fill(val);
            }

            //TCanvas *c = new TCanvas(Form("c_%s_%d", tag.c_str(), runNumber), Form("c_%s_%d", tag.c_str(), runNumber), 800, 600);
            TCanvas *c = new TCanvas("c", "c", 800, 600);
            TF1 *gaus = new TF1("gaus", "gaus", min, max);
            hSecondaries->Fit(gaus, "R");   

            double mean     = gaus->GetParameter(1);
            double sigma    = gaus->GetParameter(2);
            double meanErr  = gaus->GetParError(1);
            double sigmaErr = gaus->GetParError(2);

            std::cout << "Gaussian mean  = " << mean  << " ± " << meanErr  << std::endl;
            std::cout << "Gaussian sigma = " << sigma << " ± " << sigmaErr << std::endl;

            vmean.push_back(mean);
            vmeanErr.push_back(meanErr);
            
            hSecondaries->Draw();
            gaus->Draw("same");
        }
        colorIndex++;
        c1->cd();

        TGraphErrors *gSecondaries = new TGraphErrors(runNumbers.size(), venergy.data(), vmean.data(), venergyErr.data(), vmeanErr.data());
        if (colorIndex == 1) gSecondaries->Draw("APL");
        else gSecondaries->Draw("PL SAME");   
        gSecondaries->SetLineColor(colorIndex);
        gSecondaries->SetTitle("Efficiency vs Threshold;Primary energy [GeV];Hits per Event");
        gSecondaries->SetMarkerStyle(21);
        gSecondaries->SetMarkerSize(2.);
        gSecondaries->SetLineWidth(2.5);
        gSecondaries->SetMarkerColor(colorIndex);
        leg1->AddEntry(gSecondaries, labels[colorIndex-1].c_str(), "lp");
    }

    

    


    // Draw legends
    //c1->cd(); 
    leg1->Draw();
    //c1->SaveAs("PublicPlots/EffThr_MergingTime.pdf");
    //c1->SaveAs("PublicPlots/EffThr_MergingTime.C");

}
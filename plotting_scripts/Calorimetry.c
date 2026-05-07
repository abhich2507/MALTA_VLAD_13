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
    
   // std::vector<int> runNumbers= {83,84,85,86,87,88,89};//,90,91,92};

    //std::vector<std::string> runFiles = { "CaloNoMerg", "CaloYesMerg", "RO2x161.6nsWin", "RO2x321.6nsWin", "8x8Merging"};//, "RO300ns", "RO500ns", "RO1000ns"};
    //std::vector<std::string> labels = { "No Merging", "2 #times 8   pixel group", "2 #times 16 pixel group", "2 #times 32 pixel group", "8 #times 8   pixel group"};//, "RO300ns", "RO500ns", "RO1000ns"};
    //std::vector<int> vcolor = {kBlack, kRed, kBlue, kMagenta +2, kOrange -3};
    //std::vector<int> vmarkerStyle = {20, 21, 22, 23, 47};

    //std::vector<std::string> runFiles = { "CaloNoMerg", "RO2x80.5nsWin", "RO2x81nsWin","CaloYesMerg"};
    //std::vector<std::string> labels = {"No Merging", "0.5 ns merging", "1.0 ns merging", "1.6 ns merging"};
    //std::vector<int> vcolor = {kBlack, kAzure -3, kTeal +2, kOrange +7};
    //std::vector<int> vmarkerStyle = {20, 21, 22, 23};

    //std::vector<std::string> runFiles = {"CaloNoMerg", "CaloYesMerg", "RO8x80.5nsWin"};
    //std::vector<std::string> labels = {"0 ns merging", "1.6 ns merging", "RO 8x8 0.5 ns merging"};

    //std::vector<std::string> runFiles = { "RO8x80.5nsWin", "CaloNoMerg"};

    //std::vector<std::string> labels = {"Optimized design 200 e#lower[-2.2]{#scale[0.6]{- }}", "Optimized design 1000 e#lower[-2.2]{#scale[0.6]{- }}", "No Merging 200 e#lower[-2.2]{#scale[0.6]{- }}", "No Merging 1000 e#lower[-2.2]{#scale[0.6]{- }}"};
    //std::vector<std::string> labels = {"#splitline{8 #times 8 pixel group}{0.5 ns merging 200 e#lower[-2.2]{#scale[0.6]{- }}}","","#splitline{8 #times 8 pixel group}{0.5 ns merging 1000 e#lower[-2.2]{#scale[0.6]{- }}}",""};
    
    
    //std::vector<std::string> runFiles = { "CaloNoMerg", "CaloYesMerg", "RO16x41.6nsWin", "RO2x321.6nsWin", "8x8Merging"};
    //std::vector<std::string> labels = { "No Merging", "2 #times 8   pixel group", "4 #times 16 pixel group", "2 #times 32 pixel group", "8 #times 8   pixel group"};
    
    //std::vector<int> vcolor = {kOrange +7, kOrange +7, kAzure -3, kAzure -3 };
    //std::vector<int> vmarkerStyle = {20, 21, 22, 23};
    //std::vector<int> thrVector = {200,1000};
    //std::vector<int> thrVector = {200};
    //std::vector<double> venergy = {5,10,15,20,25,30,35};//,50,75,100};
    //std::vector<double> venergyErr(runNumbers.size(), 0.0);
    //std::vector<int> markers = {20,22,23,21};

    //std::vector<std::string> labels = {"Perfect matching", "Real matching", "Slow matching"};
    

    
    //std::vector<int> runNumbers= {93,94,95,96,97,98,99};
    //std::vector<int> runNumbers= {100,101,102,103,104,105,106};
    //std::vector<int> runNumbers= {100,101,102,103,104,105,106,121,122,123,124,125,126};
    //std::vector<int> runNumbers= {121,122,123,124,125,126};
    //std::vector<std::string> runFiles = { "FIFOideal", "FIFO100W1F", "FIFO500W1F", "FIFO500W0.001F", "FIFO500W0.01F"};//FIFO100w50nsf
    std::vector<int> runNumbers = {200,201,202,203,204,205,206,207,208,209,210,211,212,213,214};
    std::vector<int> thrVector = {200};

    //std::vector<std::string> runFiles = { "NominalFIFOx1"};//, "FIFOideal", "NominalFIFOx16", "NominalFIFOx32", "NominalFIFOx64"};
    //std::vector<std::string> labels = {"IDEAL", "1 FIFO", "16 FIFO", "32 FIFO", "64 FIFO"};
    //std::vector<std::string> runFiles = {"TestFullFIFO", "FullFIFO_SRAMD400", "FullFIFO_FIFOf0.9"};
    //std::vector<std::string> labels = {"FIFO", "SRAM D=400", "FIFOf0.9"};

    //std::vector<std::string> runFiles = {"RealFIFO", "RealFIFO_FIFOf3.125", "RealFIFO_FIFOf3.125_SRAMf3.125", "RealFIFO_FIFOf0.9_SRAMf0.9"};
    //std::vector<std::string> labels = {"FIFO Frequency = 6.25 ns", "FIFO Frequency = 3.12 ns", "FIFO f = 3.12 ns, SRAM f = 3.12 ns ", "FIFO f = 0.9 ns, SRAM f = 0.9 ns "};
    
    //std::vector<std::string> runFiles = {"RealFIFO", "RealFIFO_FIFOs256_FIFOf12.5", "RealFIFO_FIFOs384_FIFOf18.7", "RealFIFO_FIFOs512_FIFOf25"};
    //std::vector<std::string> labels = {"1x FIFO", "2x FIFO", "3x FIFO", "4x FIFO"};
    //std::vector<std::string> runFiles = {"RealFIFO_FIFOs512_FIFOf25", "RealFIFO_FIFOs512_FIFOf25_bus0.01"};
    //std::vector<std::string> labels = {"4x (FIFO=128w), bus thr = 0.1", "4x (FIFO=128w), bus thr = 0.01"};

    //std::vector<std::string> runFiles = {"HWC_SectorSize2", "HWC_SectorSize3", "HWC_SectorSize4", "HWC_SectorSize5", "HWC_SectorSize7", "HWC_SectorSize7_DebugNominal", "FIFO"};
    //std::vector<std::string> labels   = {"HWC_SectorSize2", "HWC_SectorSize3", "HWC_SectorSize4", "HWC_SectorSize5", "HWC_SectorSize7", "HWC_SectorSize7_DebugNominal", "FIFO"};

    //std::vector<std::string> runFiles = {"HWC_SectorSize7", "HWC_SectorSize7_FIFOs256"};
    //std::vector<std::string> labels   = {"HWC_SectorSize7", "HWC_SectorSize7_FIFOs256"};

    //std::vector<std::string> runFiles = {"FIFO", "HWC_SectorSize4", "HWC_SectorSize4_Optimized_ALGOMOSTFULL_SRAMd20_FIFOf18.75FIFOs384"};
    //std::vector<std::string> labels   = {"MALTA3 current design", "MALTA3 HWC Sec_Size=4", "MALTA3 HWC OPT Sec_Size=4"};

    //std::vector<std::string> runFiles = { "HWC_SectorSize7_DebugNominal", "HWC_SectorSize11_DebugNominal", "HWC_SectorSize18_DebugNominal"};
    //std::vector<std::string> labels   = {"HWC_SectorSize=7", "HWC_SectorSize=11", "HWC_SectorSize=18"};

    std::vector<std::string> runFiles = {"MALTA2", "FIFO", "HWC_SectorSize7_DebugNominal", "HWCFIXED_Optimized_SectorSize11"};
    std::vector<std::string> labels   = {"Ideal case", "Original MALTA3", "Minimal MALTA3 HWC", "Optimized MALTA3 HWC"};

    //std::vector<std::string> runFiles = { "SinglePlaneCheck", "MultiPlaneCheck"};
    //std::vector<std::string> labels   = {"Single Plane", "Multi Plane"};

    //std::vector<std::string> runFiles = { "MALTA2", "FIFO", "HWC_SectorSize3_DebugNominal", "HWC_SectorSize5_DebugNominal", "HWC_SectorSize7_DebugNominal"};
    //std::vector<std::string> labels   = {"Ideal case", "Original MALTA3", "Sector Size=3", "Sector Size=5", "Sector Size=7"};




    std::vector<int> vcolor = {kBlack, kRed, kGreen, kBlue, kMagenta, kYellow, kGray};
    std::vector<int> vmarkerStyle = {20, 21, 22, 20, 21, 22, 22};
    //std::vector<double> venergy = {5,10,15,20,25,30,50,200,400};
    //std::vector<double> venergy = {75,100,125,150,200,300};
    std::vector<double> venergy = {5,10,20,30,50,75,100,125,150,200,300,500,750,1000,2000};
    std::vector<double> venergyErr(runNumbers.size(), 0.0);
    
    TCanvas *c1 = new TCanvas("c1","numSecondaries",1200,800);
    //c1->SetRightMargin(0.15);
    c1->SetLeftMargin(0.17);
    c1->SetTopMargin(0.10);
    //c1->SetBottomMargin(0.12);
    c1->cd();
    //TLegend *leg1 = new TLegend(0.2,0.6,0.5,0.85);
    TLegend *leg1 = new TLegend(0.2,0.66,0.4,0.88);
    //TLegend *leg1 = new TLegend(0.4,0.17,0.8,0.33);
    leg1->SetTextSize(0.04);
    leg1->SetBorderSize(0);

    

    int colorIndex = 0; // ROOT color index (kBlue=4, kRed=2, kGreen=3, etc.)

    for (const int thr : thrVector)
    {

        for (const std::string &tag : runFiles) 
        {
            std::vector<double> vmean;
            std::vector<double> vmeanErr;
            for (const auto &runNumber : runNumbers)
            {
                std::cout << tag << "; " << runNumber << std::endl;            
                
                std::string summaryPath = Form("Results/local_%04d/%s/CalorimetryThr%i.root", runNumber, tag.c_str(), thr);
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
                int numSecondaries, numClusters;
                std::vector<int> histoSecondaries;
                histoSecondaries.reserve(nSummaryEntries);
                std::vector<int> histoClusters;
                histoClusters.reserve(nSummaryEntries);
                summaryTree->SetBranchAddress("numSecondaries", &numSecondaries);
                summaryTree->SetBranchAddress("numClusters", &numClusters);
                for (Long64_t i = 0; i < nSummaryEntries; i++)
                {
                    summaryTree->GetEntry(i);
                    histoSecondaries.push_back(numSecondaries);
                    histoClusters.push_back(numClusters);
                    //TODO: No smart way to plot both cluster and secondaries yet
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
            /*
            if (colorIndex == 2 || colorIndex == 4) 
            {
                double x0 = 0;
                double y0 = 0;
                double x6 = 35;
                double y6 = 350;
                gSecondaries->GetPoint(0, x0, y0);
                gSecondaries->GetPoint(6, x6, y6);

                // Define linear function
                TF1* fLine = new TF1("fLine", "[0] + [1]*x", x0, x6);

                // Fit only in this range
                //gSecondaries->Fit(fLine, "R");
                double xmax = 110;   // or whatever higher x you want
                fLine->SetRange(0, xmax);
                fLine->SetLineStyle(2);
                fLine->SetLineColor(vcolor[colorIndex -1]);
                fLine->Draw("same");
            }
            */
            /*
            else if (colorIndex == 2)
            {
                gSecondaries->Draw("PL SAME");
                double x0 = 0;
                double y0 = 0;
                double x6 = 20;
                double y6 = 156;
                gSecondaries->GetPoint(0, x0, y0);
                gSecondaries->GetPoint(3, x6, y6);
                // Define second fit only for multi threshold plot. Otherwise this should be turned off
                TF1* fLine2 = new TF1("fLine2", "[0] + [1]*x", x0, x6);

                // Fit only in this range
                fLine2->SetLineColor(kRed);
                gSecondaries->Fit(fLine2, "R");
                double xmax = 500;   // or whatever higher x you want
                fLine2->SetRange(0, xmax);
                fLine2->SetLineStyle(2);
                fLine2->Draw("same");

            }
            */


            //else if (colorIndex == 3) gSecondaries->Draw("PL SAME");   
            else if (colorIndex >1) gSecondaries->Draw("PL SAME");  
            gSecondaries->SetMinimum(0);
            gSecondaries->SetMaximum(6500);
            gSecondaries->SetLineColor(vcolor[colorIndex - 1]);
            gSecondaries->SetTitle("Efficiency vs Threshold;Primary energy [GeV];Hits per Event");
            gSecondaries->SetMarkerStyle(vmarkerStyle[colorIndex - 1]);
            if (colorIndex == 4) gSecondaries->SetMarkerSize(2.8);
            else gSecondaries->SetMarkerSize(2.);
            gSecondaries->SetLineWidth(3.);
            gSecondaries->SetMarkerColor(vcolor[colorIndex - 1]);
            
            //if (colorIndex == 1 || colorIndex == 3) leg1->AddEntry(gSecondaries, labels[colorIndex-1].c_str(), "lp");
            leg1->AddEntry(gSecondaries, labels[colorIndex-1].c_str(), "lp");
        }
    }
    

    


    // Draw legends
    c1->cd(); 
    TLatex *t = new TLatex();
    t->SetTextSize(0.049);
    t->SetNDC();
    t->DrawLatex(0.25, 0.92, "#bf{MALTA2 Simulation}, 30#mum EPI");


    leg1->Draw();
    c1->SaveAs("PublicPlots/CaloOptimalvsThreshold.pdf");
    //c1->SaveAs("PublicPlots/EffThr_MergingTime.C");

}
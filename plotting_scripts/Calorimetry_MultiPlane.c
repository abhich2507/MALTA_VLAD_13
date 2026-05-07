#include <iostream>
#include <ROOT/RNTuple.hxx>
#include <TH3D.h>
#include <TFile.h>
#include <cmath>
#include "ROOTTHelperFunctions.h"

void Calorimetry_MultiPlane()
{

    gStyle->SetCanvasPreferGL(kTRUE);
    gROOT->SetStyle("ATLAS");
    gROOT->SetBatch(kTRUE);
    
    std::vector<int> runNumbers = {218};
    std::vector<int> thrVector = {200};

    double radLength = 0.86;

    std::vector<std::string> runFiles = { "Test"};
    std::vector<std::string> labels = {""};

    std::vector<int> vcolor = {kBlack, kRed, kGreen, kBlue, kMagenta, kYellow, kGray};
    std::vector<int> vmarkerStyle = {20, 21, 22, 20, 21, 22, 22};
    std::vector<double> venergy = {5,10,20,30,50,75,100,125,150,200,300,500,750,1000,2000};
    std::vector<double> venergyErr(runNumbers.size(), 0.0);
    
    TCanvas *c1 = new TCanvas("c1","numSecondaries",1200,800);
    //c1->SetRightMargin(0.15);
    c1->SetLeftMargin(0.17);
    c1->SetTopMargin(0.10);
    //c1->SetBottomMargin(0.12);
    c1->cd();
    TLegend *leg1 = new TLegend(0.2,0.66,0.4,0.88);
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
                std::unordered_map<int, std::vector<int>> secondariesPerPlane;
                std::unordered_map<int, int> secondariesPerEvent;
                std::unordered_map<int, std::vector<int>> secondariesVecPerEvent;
                std::unordered_map<int, std::vector<int>> clustersPerPlane;
                std::unordered_map<int, std::vector<int>> xPosPerPlane;
                std::unordered_map<int, std::vector<int>> yPosPerPlane;

                std::cout << tag << "; " << runNumber << std::endl;   
                
                
                TFile *outHistFile = new TFile(Form("Plots/local_%04d/%s/CaloHistos.root",runNumber, tag.c_str()), "RECREATE");
                
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
                int numSecondaries, numClusters, planeID, eventID;
                float meanX, meanY;
                std::vector<int> histoSecondaries;
                histoSecondaries.reserve(nSummaryEntries);
                std::vector<int> histoClusters;
                histoClusters.reserve(nSummaryEntries);

                summaryTree->SetBranchAddress("planeID", &planeID);
                summaryTree->SetBranchAddress("eventID", &eventID);
                summaryTree->SetBranchAddress("meanX", &meanX);
                summaryTree->SetBranchAddress("meanY", &meanY);
                summaryTree->SetBranchAddress("numSecondaries", &numSecondaries);
                summaryTree->SetBranchAddress("numClusters", &numClusters);

                for (Long64_t i = 0; i < nSummaryEntries; i++)
                {
                    summaryTree->GetEntry(i);
                    secondariesPerEvent[eventID] += numSecondaries;
                    secondariesVecPerEvent[eventID].push_back(numSecondaries);
                    //if(eventID == 1) std::cout << "PlaneID: " << planeID << std::endl;
                    //if(planeID == 130000 && eventID  <20000)std::cout << "planeID: " << planeID << " ; eventID: " << eventID << " ; numSec: " << numSecondaries << " ; secPerEvent: " << secondariesPerEvent[eventID] << std::endl;
                    secondariesPerPlane[planeID].push_back(numSecondaries);
                    clustersPerPlane[planeID].push_back(numClusters);
                    xPosPerPlane[planeID].push_back(meanX);
                    yPosPerPlane[planeID].push_back(meanY);
                }

                for (Long64_t i = 0; i< secondariesVecPerEvent.size(); i++)
                {
                    std::vector<double> vsecondaries{};
                    std::vector<int> vlayer{};
                    std::vector<double> vradLength{};
                    for (int j = 0 ; j< secondariesVecPerEvent[i].size() ; j+=2)
                    {
                        // layer index (each pair corresponds to one layer)
                        vlayer.push_back(j / 2);
                        vradLength.push_back((j/2 + 1)* radLength);

                        // make sure we don't go out of bounds
                        int sum = secondariesVecPerEvent[i][j];

                        if (j + 1 < secondariesVecPerEvent[i].size())
                            sum += secondariesVecPerEvent[i][j + 1];

                        vsecondaries.push_back(sum);
                    }
                    
                    if (i == 1002)
                    {
                        for (int j = 0 ; j< vsecondaries.size(); j++)
                        {
                            std::cout << "layer#; " << vlayer[j] << " ; sec: " << vsecondaries[j] << std::endl;
                        }
                    
                    }

                    //X : vlayer; Y: vsecondaries

                    TGraph *gSecondaries = new TGraph(vradLength.size(), vradLength.data(), vsecondaries.data());

                    TF1 *f = new TF1("f", "[0]*pow([1]*x,[2]-1)*exp(-[1]*x)", 0, vradLength.back());
                    const double A_init = 500;
                    const double beta_init = 0.5;
                    const double alpha_init = 1.7;

                    f->SetParameters(A_init, beta_init, alpha_init);

                    TFitResultPtr r = gSecondaries->Fit(f,"QS");
                    double chi2 = f->GetChisquare();
                    double ndf  = f->GetNDF();
                    double chi2ndf = chi2 / ndf;

                    //std::cout << "eventID: " << i << " chi2: " << chi2ndf << std::endl;

                    if (r->Status() != 0) 
                    {
                        std::cout << "Fit for event " <<  i << " failed with status " << r->Status() << std::endl;
                    }

                    double alpha = f->GetParameter(2);
                    double beta  = f->GetParameter(1);

                    double tmax = (alpha - 1)/beta;
                    
                }




                
                // Secondaries Histo
                auto minIt = std::min_element(
                secondariesPerEvent.begin(),
                secondariesPerEvent.end(),
                [](const auto& a, const auto& b) {
                    return a.second < b.second;
                }
                );
                double minSecSum = minIt->second;

                auto maxIt = std::max_element(
                secondariesPerEvent.begin(),
                secondariesPerEvent.end(),
                [](const auto& a, const auto& b) {
                    return a.second < b.second;
                }
                );
                double maxSecSum = maxIt->second;
                
                TH1D* hSecSum = new TH1D(
                    Form("hSecSum_run%d_file%s_thr%d", runNumber, tag.c_str(), thr),
                    Form(";Secondaries;Counts"),
                    50, minSecSum, maxSecSum
                );

                for (const auto& [pid, secSum] : secondariesPerEvent)
                {
                    hSecSum->Fill(secSum);
                }


                outHistFile->cd();
                hSecSum->Write();

                for (const auto& [pid, secVec] : secondariesPerPlane)
                {
                    const auto& cluVec = clustersPerPlane[pid];
                    const auto& xPosVec = xPosPerPlane[pid];
                    const auto& yPosVec = yPosPerPlane[pid];

                    if (secVec.empty()) continue;

                    // Secondaries Histo
                    double minSec = *std::min_element(secVec.begin(), secVec.end());
                    double maxSec = *std::max_element(secVec.begin(), secVec.end());

                    TH1D* hSec = new TH1D(
                        Form("hSec_plane%d_run%d_file%s_thr%d", pid, runNumber, tag.c_str(), thr),
                        Form("Plane %d;Secondaries;Counts", pid),
                        50, minSec, maxSec
                    );

                    for (auto v : secVec) hSec->Fill(v);

                    if (cluVec.empty()) continue;

                    // Cluster Histo
                    double minClu = *std::min_element(cluVec.begin(), cluVec.end());
                    double maxClu = *std::max_element(cluVec.begin(), cluVec.end());

                    TH1D* hClu = new TH1D(
                        Form("hClu_plane%d_run%d_file%s_thr%d", pid, runNumber, tag.c_str(), thr),
                        Form("Plane %d;Clusters;Counts", pid),
                        50, minClu, maxClu
                    );

                    for (auto v : cluVec) hClu->Fill(v);

                    if (xPosVec.empty()) continue;

                    // X position Histo
                    double minX = *std::min_element(xPosVec.begin(), xPosVec.end());
                    double maxX = *std::max_element(xPosVec.begin(), xPosVec.end());
                    if (minX == maxX) { minX -= 0.5; maxX += 0.5; }

                    TH1D* hX = new TH1D(
                        Form("hX_plane%d_run%d_file%s_thr%d", pid, runNumber, tag.c_str(), thr),
                        Form("Plane %d;X position;Counts", pid),
                        50, minX, maxX
                    );

                    for (auto v : xPosVec) hX->Fill(v);

                    if (yPosVec.empty()) continue;

                    // Y position Histo
                    double minY = *std::min_element(yPosVec.begin(), yPosVec.end());
                    double maxY = *std::max_element(yPosVec.begin(), yPosVec.end());
                    if (minY == maxY) { minY -= 0.5; maxY += 0.5; }

                    TH1D* hY = new TH1D(
                        Form("hY_plane%d_run%d_file%s_thr%d", pid, runNumber, tag.c_str(), thr),
                        Form("Plane %d;Y position;Counts", pid),
                        50, minY, maxY
                    );

                    for (auto v : yPosVec) hY->Fill(v);

                    // Save all to root file instead
                    outHistFile->cd();
                    hSec->Write();
                    hClu->Write();
                    hX->Write();
                    hY->Write();
                }
                gDirectory->Delete("*");
                outHistFile->Write();
                outHistFile->Close();
                
                // 


            }
            //colorIndex++;
            //c1->cd();
            /*
            TGraphErrors *gSecondaries = new TGraphErrors(runNumbers.size(), venergy.data(), vmean.data(), venergyErr.data(), vmeanErr.data());
            if (colorIndex == 1) gSecondaries->Draw("APL");
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
            */
        }
    }
    

    

    /*
    // Draw legends
    c1->cd(); 
    TLatex *t = new TLatex();
    t->SetTextSize(0.049);
    t->SetNDC();
    t->DrawLatex(0.25, 0.92, "#bf{MALTA2 Simulation}, 30#mum EPI");


    leg1->Draw();
    c1->SaveAs("PublicPlots/CaloOptimalvsThreshold.pdf");
    //c1->SaveAs("PublicPlots/EffThr_MergingTime.C");
    */

}
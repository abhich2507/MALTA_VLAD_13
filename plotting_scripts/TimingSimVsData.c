#include <iostream>
#include <ROOT/RNTuple.hxx>
#include <TH3D.h>
#include <TFile.h>
#include <cmath>


void TimingSimVsData()
{
    gStyle->SetCanvasPreferGL(kTRUE);
    gROOT->SetStyle("ATLAS");
    TCanvas *c1 = new TCanvas("c1","Hit Timing Sim vs Data",800,800);
    c1->SetTopMargin(0.1);    // leave space for title
    TCanvas *c2 = new TCanvas("c2","Cl Timing Sim vs Data",800,800);
    c2->SetTopMargin(0.1);    // leave space for title

    TCanvas *c3 = new TCanvas("c3","Cl Timing Sim Jitter vs no Jitter",800,800);
    c3->SetTopMargin(0.1);    // leave space for title

    TLegend *leg1 = new TLegend(0.62,0.69,0.82,0.83);
    leg1->SetTextSize(0.05);
    leg1->SetBorderSize(0);

    TLegend *leg2 = new TLegend(0.62,0.49,0.82,0.83);
    leg2->SetTextSize(0.05);
    leg2->SetBorderSize(0);

    TH1D *h1Empty    = new TH1D("h1Empty", "", 100,0,150);
    std::string dataPath = "plotting_scripts/root_input/Eff_Clsize_W5R23__IDB120_ITHR015_SUB06.0_PWELL06.root";
    std::cout << "Opening: " << dataPath << std::endl;

    TFile *dataFile = TFile::Open(dataPath.c_str(), "READ");
    if (!dataFile || dataFile->IsZombie()) {
        std::cerr << "Could not open file: " << dataPath << std::endl;
        return;
    }
    TH1D *h1HitDataTiming = (TH1D*) dataFile->Get("hit_time");
    TH1D *h1ClDataTiming = (TH1D*) dataFile->Get("cl_time");
    h1HitDataTiming->Rebin(2);
    h1HitDataTiming->Scale(1.0 / h1HitDataTiming->Integral("width"));
    h1HitDataTiming->SetLineWidth(3);
    h1HitDataTiming->SetLineColor(kBlue);
    h1HitDataTiming->GetYaxis()->SetTitle("Normalized counts");
    h1HitDataTiming->GetXaxis()->SetRangeUser(118,180);   

    h1HitDataTiming->GetXaxis()->SetTitleSize(0.044);
    h1HitDataTiming->GetXaxis()->SetTickSize(0.044);
    h1HitDataTiming->GetXaxis()->SetLabelSize(0.04);
    h1HitDataTiming->GetYaxis()->SetTitleSize(0.044);
    h1HitDataTiming->GetYaxis()->SetTickSize(0.044);
    h1HitDataTiming->GetYaxis()->SetLabelSize(0.04);

    h1ClDataTiming->Scale(1.0 / h1ClDataTiming->Integral("width"));
    h1ClDataTiming->SetLineWidth(3);
    h1ClDataTiming->SetLineColor(kBlue);
    h1ClDataTiming->GetYaxis()->SetTitle("Normalized counts");
    h1ClDataTiming->GetXaxis()->SetRangeUser(110,150);   


    h1ClDataTiming->GetXaxis()->SetTitleSize(0.044);
    h1ClDataTiming->GetXaxis()->SetTickSize(0.044);
    h1ClDataTiming->GetXaxis()->SetLabelSize(0.04);
    h1ClDataTiming->GetYaxis()->SetTitleSize(0.044);
    h1ClDataTiming->GetYaxis()->SetTickSize(0.044);
    h1ClDataTiming->GetYaxis()->SetLabelSize(0.04);

    std::string simPath = "Plots/local_0195/Nominal/histos.root";
    std::cout << "Opening: " << simPath << std::endl;

    TFile *simFile = TFile::Open(simPath.c_str(), "READ");
    if (!simFile || simFile->IsZombie()) {
        std::cerr << "Could not open file: " << simPath << std::endl;
        return;
    }
    TDirectory *dir = (TDirectory*)simFile->Get("Thr200");
    TH1D *h1SimTiming = (TH1D*) dir->Get("h1CorrectedTiming");
    
    h1SimTiming->Scale(1.0 / h1SimTiming->Integral("width"));



    // Build file path
    const int runNumber = 195;
    std::string tag = "Nominal";
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
    anaTree->SetBranchAddress("DUTLocalTime", &timing);
    int nEntries = anaTree->GetEntries();
    //TH1D *h1SimHitTiming = new TH1D("h1SimHitTiming", "", 100,0,200);
    TH1D *h1SimHitTiming = (TH1D*) h1HitDataTiming->Clone("h1SimHitTiming");
    h1SimHitTiming->Reset();

    for (Long64_t i = 0; i < nEntries; i++) 
    {
        anaTree->GetEntry(i);
        //std::cout <<timing << std::endl;
        if (timing > 0) h1SimHitTiming->Fill(timing + 87.5);

    }
    h1SimHitTiming->Scale(1.0 / h1SimHitTiming->Integral("width"));
    h1SimHitTiming->SetLineWidth(3);
    h1SimHitTiming->SetLineColor(kRed);

    leg1->AddEntry(h1HitDataTiming, "Data", "L");
    leg1->AddEntry(h1SimHitTiming, "Simulation", "L");


    TF1 *fitDataFunc = new TF1("fitData", "gaus");
    fitDataFunc->SetLineColor(kBlue);
    fitDataFunc->SetLineWidth(4);
    h1ClDataTiming->Fit(fitDataFunc,"");
    double sigmaData = fitDataFunc->GetParameter(2);

    // Build Cl file path
    std::string clPath = Form("Results/local_%04d/%s/analysisThr200.root", runNumber, tag.c_str());
    //std::string anaPath = Form("Results/local_%04d/%s/analysisThr200.root", runNumber, tag.c_str());
    std::cout << "Opening: " << clPath << std::endl;

    TFile *clFile = TFile::Open(clPath.c_str(), "READ");
    if (!clFile || clFile->IsZombie()) {
        std::cerr << "Could not open file: " << clPath << std::endl;
        return ;
    }

    TTree *clTree = (TTree*) clFile->Get("analyzedHits_planeZ0");
    if (!clTree) {
        std::cerr << "No anaTree in file: " << clPath << std::endl;
        return ;
    }

    double clTiming{};
    clTree->SetBranchAddress("timing", &clTiming);
    int nClEntries = clTree->GetEntries();
    //TH1D *h1SimHitTiming = new TH1D("h1SimHitTiming", "", 100,0,200);
    TH1D *h1SimClTiming = (TH1D*) h1ClDataTiming->Clone("h1SimClTiming");
    h1SimClTiming->Reset();

    for (Long64_t i = 0; i < nClEntries; i++) 
    {
        clTree->GetEntry(i);
        //std::cout <<timing << std::endl;
        if (clTiming > 0) h1SimClTiming->Fill(clTiming + 85.5);

    }
    h1SimClTiming->Scale(1.0 / h1SimClTiming->Integral("width"));
    h1SimClTiming->SetLineWidth(3);
    h1SimClTiming->SetLineColor(kRed);

    TF1 *fitSimFunc = new TF1("fitSim", "gaus");
    fitSimFunc->SetLineColor(kRed);
    fitSimFunc->SetLineWidth(4);
    h1SimClTiming->Fit(fitSimFunc, "");

    double sigmaSim = fitSimFunc->GetParameter(2);

    leg2->AddEntry(h1ClDataTiming, Form("#splitline{Data}{#sigma=%.1f ns}", sigmaData), "L");
    leg2->AddEntry(h1SimClTiming, Form("#splitline{Simulation}{#sigma=%.1f ns}", sigmaSim), "L");


    // Build Cl file path Add
    tag = "NoJitter";
    clPath = Form("Results/local_%04d/%s/analysisThr200.root", runNumber, tag.c_str());
    //std::string anaPath = Form("Results/local_%04d/%s/analysisThr200.root", runNumber, tag.c_str());
    std::cout << "Opening: " << clPath << std::endl;

    TFile *clFileAdd = TFile::Open(clPath.c_str(), "READ");
    if (!clFileAdd || clFileAdd->IsZombie()) {
        std::cerr << "Could not open file: " << clPath << std::endl;
        return ;
    }

    TTree *clTreeAdd = (TTree*) clFileAdd->Get("analyzedHits_planeZ0");
    if (!clTreeAdd) {
        std::cerr << "No anaTree in file: " << clPath << std::endl;
        return ;
    }

    double clTimingAdd{};
    clTreeAdd->SetBranchAddress("timing", &clTimingAdd);
    int nClEntriesAdd = clTreeAdd->GetEntries();
    //TH1D *h1SimHitTiming = new TH1D("h1SimHitTiming", "", 100,0,200);
    TH1D *h1SimClTimingAdd = (TH1D*) h1ClDataTiming->Clone("h1SimClTiming");
    h1SimClTimingAdd->Reset();

    for (Long64_t i = 0; i < nClEntriesAdd; i++) 
    {
        clTreeAdd->GetEntry(i);
        //std::cout <<timing << std::endl;
        if (clTimingAdd > 0) 
        {
            h1SimClTimingAdd->Fill(clTimingAdd + 85.5);
        }

    }
    h1SimClTimingAdd->Scale(1.0 / h1SimClTimingAdd->Integral("width"));
    h1SimClTimingAdd->SetLineWidth(3);
    h1SimClTimingAdd->SetLineColor(kMagenta);

    TF1 *fitSimFuncAdd = new TF1("fitSimFuncAdd", "gaus");
    fitSimFuncAdd->SetLineColor(kRed);
    fitSimFuncAdd->SetLineWidth(4);
    h1SimClTimingAdd->Fit(fitSimFuncAdd, "");

    double sigmaSimAdd = fitSimFuncAdd->GetParameter(2);




    c1->cd();
    //h1Empty->Draw("A");
    h1HitDataTiming->Draw("HIST");
    h1SimHitTiming->Draw("HIST SAME");

    TLatex *t3 = new TLatex();
    t3->SetTextSize(0.043);
    t3->SetNDC();
    t3->DrawLatex(0.26, 0.92, Form("#bf{MALTA2 Simulation}, 30#mum EPI"));


    leg1->Draw();

    c2->cd();
    //h1Empty->Draw("A");
    h1ClDataTiming->Draw("HIST");
    h1SimClTiming->Draw("HIST SAME");
    //fitDataFunc->Draw("SAME L");
    //fitSimFunc->Draw("SAME L");
    gPad->Modified();
    gPad->Update(); 

    TLatex *t4 = new TLatex();
    t4->SetTextSize(0.043);
    t4->SetNDC();
    t4->DrawLatex(0.26, 0.92, Form("#bf{MALTA2 Simulation}, 30#mum EPI"));

    leg2->Draw();


    c3->cd();
    h1SimClTiming->Draw("HIST");
    h1SimClTimingAdd->Draw("HIST SAME");


    std::cout << "Sim Hit  mean: " << h1SimHitTiming->GetMean()  << " sigma: " << h1SimHitTiming->GetStdDev() << std::endl;
    std::cout << "Data Hit mean: " << h1HitDataTiming->GetMean() << " sigma: " << h1HitDataTiming->GetStdDev() << std::endl;

    std::cout << "Sim Cl  mean: " << h1SimClTiming->GetMean()  << " sigma: " << h1SimClTiming->GetStdDev() << std::endl;
    std::cout << "Data Cl mean: " << h1ClDataTiming->GetMean() << " sigma: " << h1ClDataTiming->GetStdDev() << std::endl;

    c1->SaveAs("PublicPlots/HitTiming_SimvsData.pdf");
    c1->SaveAs("PublicPlots/HitTiming_SimvsData.C");

    c2->SaveAs("PublicPlots/ClTiming_SimvsData.pdf");
    c2->SaveAs("PublicPlots/ClTiming_SimvsData.C");

}
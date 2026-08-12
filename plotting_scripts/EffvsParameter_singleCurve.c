#include <iostream>
#include <ROOT/RNTuple.hxx>
#include <TH3D.h>
#include <TFile.h>
#include <cmath>
#include "ROOTTHelperFunctions.h"

void EffvsParameter_singleCurve()
{

    gStyle->SetCanvasPreferGL(kTRUE);
    gROOT->SetStyle("ATLAS");
    //174 //112  // 70 // 181
    //std::vector<int> runNumbers= {171,172,173,174,175}; // Pix Barrel
    //std::vector<int> runNumbers = {176,177,178,179}; // Strip Barrel
    //std::vector<int> runNumbers = {180,181,182,183}; // Pix EndCap
    //std::vector<int> runNumbers = {184,185}; // Strip Endcap
    //std::vector<int> runNumbers = {186,187,188, 189, 190}; // Revised Pix Barrel 
    //std::vector<int> runNumbers = {237,237,237,237,237};
    std::vector<int> runNumbers = {284,284,284,284,284,284,284};
    //std::vector<std::string> runFiles = { "wordSpacingScan0ns", "wordSpacingScan0.5ns", "wordSpacingScan1ns","wordSpacingScan1.6ns"};
    //std::vector<std::string> labels = {"No Merging", "0.5 ns merging", "1.0 ns merging", "1.6 ns merging"};
    //std::vector<int> vcolor = {kBlack, kAzure -3, kTeal +2, kOrange +7};
    //std::vector<int> vmarkerStyle = {20, 21, 22, 23};

    //std::vector<std::string> runFiles = { "RO8ns", "RO10ns", "RO20ns", "RO100ns"};//, "RO300ns", "RO500ns", "RO1000ns"};
    //std::vector<std::string> labels = { "Veto8ns", "Veto10ns", "Veto20ns", "Veto100ns"};//, "RO300ns", "RO500ns", "RO1000ns"};

    //std::vector<std::string> runFiles = {"wordSpacingScan0ns", "RO2x8", "RO2x16", "RO2x32", "RO8x8_Full"};
    //std::vector<std::string> labels = { "No Merging", "2 #times 8   pixel group", "2 #times 16 pixel group", "2 #times 32 pixel group", "8 #times 8   pixel group"};//, "RO300ns", "RO500ns", "RO1000ns"};
    //std::vector<int> vcolor = {kBlack, kRed, kBlue, kMagenta +2, kOrange -3};
    //std::vector<int> vmarkerStyle = {20, 21, 22, 23, 47};

    //std::vector<std::string> runFiles = {"wordSpacingScan0ns", "RO2x32", "RO2x8", "RO32x2", "RO8x2"};
    //std::vector<std::string> labels = { "No Merging", "2 #times 32 pixel group", "2 #times 8   pixel group", "32 #times 2 pixel group", "8 #times 2   pixel group"};
    
    //std::vector<std::string> runFiles = {"wordSpacingScan0ns", "RO2x32", "RO4x16", "RO32x2", "RO8x8"};
    //std::vector<std::string> labels = { "No Merging", "2 #times 32 pixel group", "4 #times 16   pixel group","32 #times 2 pixel group", "8 #times 8   pixel group"};
    
    //std::vector<std::string> runFiles = {"RO2x32", "RO4x16", "RO8x8", "RO16x4", "RO32x2"};
    //std::vector<std::string> labels = {"2#times32", "4#times16", "8#times8", "16#times4", "32#times2"};


    //std::vector<std::string> runFiles = {"MERGETrack", "FIFOTrack"};
    //std::vector<std::string> labels = {"MALTA2", "MALTA3"};

    //std::vector<std::string> runFiles = {"Veto10ns", "Veto8ns", "Veto7ns", "Veto6ns"};
    //std::vector<std::string> labels = {"100MHz beam rate", "125MHz beam rate", "142MHz beam rate", "166MHz beam rate"};

    /////////// Systematic MALTA3 study Run 174

    //std::vector<std::string> runFiles = {"Nominal", "SlowDelay2ns"};
    //std::vector<std::string> labels = {"Slow Delay = 0.5ns", "Slow Delay = 2ns"};

    //std::vector<std::string> runFiles = {"busThr0.052", "Nominal", "busThr0.210"};
    //std::vector<std::string> labels = {"Bus Thr = 0.052 ns", "Bus Thr = 0.105 ns", "Bus Thr = 0.210 ns"};

    //std::vector<std::string> runFiles = {"SRAMf6.25", "SRAMf3", "Nominal", "SRAMf0.75"};
    //std::vector<std::string> labels = {"SRAM freq. = 160 MHz", "SRAM freq. = 333 MHz", "SRAM freq. 666 MHz", "SRAM freq. 1.33 GHz"};

    //std::vector<std::string> runFiles = {"Nominal_ItkLayer4", "SRAMd1"};
    //std::vector<std::string> labels = {"ITK Layer 4 Nominal", "SRAM depth = 1"};

    //std::vector<std::string> runFiles = {"Nominal"};
    //std::vector<std::string> labels = {"ITK Pix Barrel Layer 0", "ITK Pix Barrel Layer 1", "ITK Pix Barrel Layer 2", "ITK Pix Barrel Layer 3", "ITK Pix Barrel Layer 4"};
    //std::vector<std::string> labels = {"ITK Pix EndCap Ring 0", "ITK Pix EndCap Ring 1", "ITK Pix EndCap Ring 2", "ITK Pix EndCap Ring 3"};
    //std::vector<std::string> labels = {"ITK Strip EndCap Disk 0", "ITK Strip EndCap Disk 1"};

    //std::vector<std::string> runFiles = {"Nominal", "FIFOf3.125", "FIFOf1.5"};

    std::vector<std::string> runFiles = {"EIC_1bkg_proton", "EIC_2bkg_proton", "EIC_20bkg_proton", "EIC_30bkg_proton", "EIC_40bkg_proton", "EIC_50bkg_proton", "EIC_100bkg_proton"};

    //std::vector<std::string> labels = {"Pix Barrel Layer4", "Pix Barrel Layer3", "Pix Barrel Layer2", "Pix Barrel Layer1", "Pix Barrel Layer0"};
    //std::vector<double> parameters = {6.5, 3.125, 1.5};
    //std::vector<double> parameters = {20,40,60,80,100,120,140,160,180,200};
    std::vector<double> parameters = {1,2,20,30,40,50,100};
    //std::vector<std::string> labels = {"#splitline{ITK Pix Barrel}{Layer 0}", "#splitline{ITK Pix Barrel}{Layer 1}", "#splitline{ITK Pix Barrel}{Layer 2}", "#splitline{ITK Pix Barrel}{Layer 3}", "#splitline{ITK Pix Barrel}{Layer 4}"};
    std::vector<std::string> labels = {"1 bkg", "2 bkg", "20 bkg", "30 bkg", "40 bkg", "50 bkg", "100 bkg"};

    std::vector<int> vcolor = {kBlack, kMagenta +2, kRed, kAzure -3, kGreen +3};
    std::vector<int> vmarkerStyle = {20, 23, 21, 23, 21};

    //std::vector<std::string> labels = {"Perfect matching", "Real matching", "Slow matching"};

    TCanvas *c1 = new TCanvas("c1","Efficiency vs Threshold",1200,800);
    TPad *pad1 = new TPad("pad1","plot",0.0,0.0,0.75,1.0);
    TPad *pad2 = new TPad("pad2","legend",0.75,0.0,1.0,0.85);
    TPad *pad3 = new TPad("pad3","latex",0.75,0.85,1.0,1.);
    pad3->Draw();
    //c1->SetRightMargin(0.15);
    c1->SetLeftMargin(0.17);
    c1->SetTopMargin(0.09);
    //c1->SetBottomMargin(0.12);
    pad1->Draw();
    pad1->cd();
    //TLegend *leg1 = new TLegend(0.53,0.19,0.83,0.47);
    TLegend *leg1 = new TLegend(0.,0.2,1.,1.);
    //leg1->SetTextSize(0.04);
    leg1->SetTextSize(0.09);
    leg1->SetBorderSize(0);

    std::vector<TGraphErrors*> graphs;


    int colorIndex = 1; // ROOT color index (kBlue=4, kRed=2, kGreen=3, etc.)
    int globalCount = 1;
    int runCount = 0;
    std::vector<double> verrRate{};
    std::vector<double> veta{};
    double normErrRate{}, normEta{};

    std::vector<double> vEffAll{}, vEffErrAll{}, vThrNoErrAll{};
    
    for (const std::string &tag : runFiles)
    {
        
        // Process Simulation data
        std::vector<double> vThr, vThrErr, vThrNoErr, vEff, vEffErr, vClSize, vClSizeErr, vTiming;
        ProcessedSimulation simValues = sort_simTree_Digital(runNumbers[0], tag);
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
        std::cout <<"numValidEntries: " << numValidEntries << std::endl;

        std::cout << "Eff: " << vEff[0] << std::endl;
        vEffAll.push_back(vEff[0]);
        vEffErrAll.push_back(vEffErr[0]);
        vThrNoErrAll.push_back(vThrNoErr[0]);
        
    }

    std::cout << "Sizes: " << parameters.size() << " " << vEffAll.size() << std::endl;

    TGraphErrors *gThrVSEff = new TGraphErrors(7, parameters.data(), vEffAll.data(), vThrNoErrAll.data(), vEffErrAll.data());
    graphs.push_back(gThrVSEff);
    //gThrVSEff->SetTitle(";FIFO Readout [ns];Efficiency [%]");
    gThrVSEff->SetTitle(";Fixed background / 2#mus;Hit Efficiency [%]");
    gThrVSEff->SetMarkerStyle(vmarkerStyle[colorIndex -1]);
    gThrVSEff->SetMarkerSize(2.);
    gThrVSEff->SetLineWidth(3);
    gThrVSEff->SetMarkerColor(vcolor[colorIndex - 1]); //colorIndex
    gThrVSEff->SetLineColor(vcolor[colorIndex - 1]);
    gThrVSEff->GetXaxis()->SetTitleOffset(1.2);
    gThrVSEff->GetYaxis()->SetTitleOffset(1.7);
    gThrVSEff->GetXaxis()->SetLimits(-9,110);
    c1->cd(); 
    gThrVSEff->Draw("APL"); // first one draws axes
    gThrVSEff->SetMinimum(80);  // or some min
    gThrVSEff->SetMaximum(101.);  // or some max

    TLatex *t = new TLatex();
    //t->SetTextSize(0.049);
    t->SetTextSize(0.04);
    t->SetNDC();
    t->DrawLatex(0.4, 0.94, "#splitline{#bf{EIC FMT MALTA2 Simulation}}{               30#mum EPI}");


    pad1->SetLeftMargin(0.17);
    pad1->SetRightMargin(0.02);

    
    c1->SaveAs("PublicPlots/EffThr_GroupParityV2.pdf");
    c1->SaveAs("PublicPlots/EffThr_GroupParityV2.png");
    c1->SaveAs("PublicPlots/EffThr_GroupParityV2.C");

}


#include <iostream>
#include <fstream>
#include <ROOT/RNTuple.hxx>
#include <TH3D.h>
#include <TFile.h>
#include <cmath>
#include "ROOTTHelperFunctions.h"

void mergingStatistics()
{
    int runNumber = 1;
    std::string tag = "MergingInfo32x2";
    int thr = 200;
    std::string summaryPath = Form("Results/local_%04d/%s/Plane0ReconstructedHitsThr%i.root", runNumber, tag.c_str(), thr);
    std::cout << "Opening: " << summaryPath << std::endl;

    TFile *summaryFile = TFile::Open(summaryPath.c_str(), "READ");
    if (!summaryFile || summaryFile->IsZombie()) {
        std::cerr << "Could not open file: " << summaryPath << std::endl;
        return;
    }

    TTree *mergedTree = (TTree*) summaryFile->Get("MergedHits");
    if (!mergedTree) {
        std::cerr << "No mergedTree in file: " << summaryPath << std::endl;
        return;
    }

    TH1D *hdeltaX = new TH1D("hdeltaX",";X Pixel displacement;Counts", 50, 0, 512);
    TH1D *hdeltaY = new TH1D("hdeltaY",";Y Pixel displacement;Counts", 50, 0, 512);
    int dcCount = 0;
    int parCount = 0;
    int groupCount = 0;
    int yMerging = 0;
    int displacedCount = 0;
    int hitLossCount = 0;
    int deadX = 0;
    int deadY = 0;
    int xyMerging = 0;


    Long64_t nMergedEntries = mergedTree->GetEntries();
    int deltaX, deltaY;
    bool groupMerging, parityMerging, dcMerging, displacedMerging, hitLossMerging;

    mergedTree->SetBranchAddress("deltaX", &deltaX);
    mergedTree->SetBranchAddress("deltaY", &deltaY);
    mergedTree->SetBranchAddress("groupMerging", &groupMerging);
    mergedTree->SetBranchAddress("parityMerging", &parityMerging);
    mergedTree->SetBranchAddress("dcMerging", &dcMerging);
    mergedTree->SetBranchAddress("displacedMerging", &displacedMerging);
    mergedTree->SetBranchAddress("hitLossMerging", &hitLossMerging);
    for (Long64_t i = 0; i < nMergedEntries; i++)
    {
        mergedTree->GetEntry(i);
        if(deltaX > 0) hdeltaX->Fill(deltaX);
        if (deltaX >2) deadX++;
        if(deltaY > 0) hdeltaY->Fill(deltaY);
        if (deltaY >2) deadY++;
        if (groupMerging) groupCount++;
        if (parityMerging) parCount++;
        if (dcMerging) dcCount++;
        if (deltaX > 0 && deltaY > 0) xyMerging++;
        if (parityMerging || groupMerging) yMerging++;
        if (displacedMerging) displacedCount++;
        if (hitLossMerging) hitLossCount++;
    }

    TCanvas *c1 = new TCanvas("c1", "c1", 800, 600);
    TCanvas *c2 = new TCanvas("c2", "c2", 800, 600);
    c1->cd();
    hdeltaX->Draw();
    c2->cd();
    hdeltaY->Draw();
    std::cout << "groupCount: " << groupCount << "; parityCount: " << parCount << "; DoubleColumnCount: " << dcCount << "; displacedCount: " << displacedCount << "; hitLossCount: " << hitLossCount << std::endl;
    std::cout << "xMerging: " << dcCount << "; yMerging: " << yMerging << "; xDead: " << deadX << "; yDead: " << deadY << "; xyMerging: " << xyMerging << std::endl;


    std::ofstream fileX("histogramX.csv");

    fileX << "bin_center,count\n";

    for (int i = 1; i <= hdeltaX->GetNbinsX(); i++) {
        double binCenter = hdeltaX->GetBinCenter(i) - 0.5;
        double count     = hdeltaX->GetBinContent(i);

        fileX << binCenter << "," << count << "\n";
    }

    fileX.close();    
    
    std::ofstream fileY("histogramY.csv");

    fileY << "bin_center,count\n";

    for (int i = 1; i <= hdeltaY->GetNbinsX(); i++) {
        double binCenter = hdeltaY->GetBinCenter(i) - 0.5;
        double count     = hdeltaY->GetBinContent(i);

        fileY << binCenter << "," << count << "\n";
    }

    fileY.close();
}
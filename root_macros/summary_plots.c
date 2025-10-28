#include <iostream>
#include <ROOT/RNTuple.hxx>
#include <TH3D.h>
#include <TFile.h>


void summary_plots()
{
    std::vector<int> runNumbers= {2};
    std::vector<std::string> runFiles = {"Nominal"};

    std::vector<std::string> labels = {"Nominal"};

    //std::vector<std::string> labels = {"Perfect matching", "Real matching", "Slow matching"};

    TCanvas *c1 = new TCanvas("c1","Efficiency vs Threshold",800,600);
    TCanvas *c2 = new TCanvas("c2","Cluster size vs Threshold",800,600);
    TCanvas *c3 = new TCanvas("c3","Timing vs Threshold",800,600);

    TLegend *leg1 = new TLegend(0.15,0.75,0.45,0.9);
    TLegend *leg2 = new TLegend(0.15,0.75,0.45,0.9);
    TLegend *leg3 = new TLegend(0.15,0.75,0.45,0.9);

    int colorIndex = 1; // ROOT color index (kBlue=4, kRed=2, kGreen=3, etc.)

    for (const auto &tag : runFiles) 
    {

        for (const auto &runNumber : runNumbers)
        {
            // Build file path
            std::string summaryPath = Form("/home/vlad/Documents/Simu/Geant4/DECAL_REPO/Plots/local_%04d/%s/summary.root", runNumber, tag.c_str());
            std::cout << "Opening: " << summaryPath << std::endl;

            TFile *summaryFile = TFile::Open(summaryPath.c_str(), "READ");
            if (!summaryFile || summaryFile->IsZombie()) {
                std::cerr << "Could not open file: " << summaryPath << std::endl;
                continue;
            }

            TTree *summaryTree = (TTree*) summaryFile->Get("summaryTree");
            if (!summaryTree) {
                std::cerr << "No summaryTree in file: " << summaryPath << std::endl;
                continue;
            }

            double threshold, efficiency, effError, timing, clSize;
            summaryTree->SetBranchAddress("threshold", &threshold);
            summaryTree->SetBranchAddress("efficiency", &efficiency);
            summaryTree->SetBranchAddress("effError", &effError);
            summaryTree->SetBranchAddress("clSize", &clSize);
            summaryTree->SetBranchAddress("timing", &timing);

            int nEntriesSummary = summaryTree->GetEntries();

            std::vector<double> vThr, vThrErr, vEff, vEffErr, vClSize, vTiming;
            vThr.reserve(nEntriesSummary);
            vThrErr.reserve(nEntriesSummary);
            vEff.reserve(nEntriesSummary);
            vEffErr.reserve(nEntriesSummary);
            vClSize.reserve(nEntriesSummary);
            vTiming.reserve(nEntriesSummary);

            for (int i = 0; i< nEntriesSummary; i++) { // replace 11 with 
                summaryTree->GetEntry(i);

                vThr.push_back(threshold);
                vThrErr.push_back(threshold/3.0); // 3% quoted from calibration paper
                vEff.push_back(efficiency);
                vEffErr.push_back(effError);
                vClSize.push_back(clSize);
                vTiming.push_back(timing);
            }
            // Graphs
            TGraphErrors *gThrVSEff = new TGraphErrors(nEntriesSummary, vThr.data(), vEff.data(), vThrErr.data(), vEffErr.data());
            
            gThrVSEff->SetTitle("Efficiency vs Threshold;Threshold [e-];Tracking efficiency [%]");
            gThrVSEff->SetMarkerStyle(20);
            gThrVSEff->SetMarkerColor(colorIndex);
            // TODO: Smarter solution
            //leg1->AddEntry(gThrVSEff, runFiles[colorIndex-1].c_str(), "p");
            leg1->AddEntry(gThrVSEff, labels[colorIndex-1].c_str(), "p");

            TGraph *gThrVSClSize = new TGraph(nEntriesSummary, vThr.data(),  vClSize.data());
            gThrVSClSize->SetTitle("Cluster size vs Threshold;Threshold [e-];Cluster size");
            gThrVSClSize->SetMarkerStyle(21);
            gThrVSClSize->SetMarkerColor(colorIndex);
            //leg2->AddEntry(gThrVSClSize, runFiles[colorIndex-1].c_str(), "p");
            leg2->AddEntry(gThrVSClSize, labels[colorIndex-1].c_str(), "p");

            TGraph *gThrVSTiming = new TGraph(nEntriesSummary, vThr.data(),  vTiming.data());
            gThrVSTiming->SetTitle("Timing vs Threshold;Threshold [e-];Timing [ns]");
            gThrVSTiming->SetMarkerStyle(22);
            gThrVSTiming->SetMarkerColor(colorIndex);
            //leg3->AddEntry(gThrVSTiming, runFiles[colorIndex-1].c_str(), "p");
            leg3->AddEntry(gThrVSTiming, labels[colorIndex-1].c_str(), "p");

            // Draw on same canvases
            c1->cd();
            if (colorIndex == 1) gThrVSEff->Draw("AP"); // first one draws axes
            else gThrVSEff->Draw("P SAME");

            c2->cd();
            if (colorIndex == 1) gThrVSClSize->Draw("AP");
            else gThrVSClSize->Draw("P SAME");

            c3->cd();
            if (colorIndex == 1) gThrVSTiming->Draw("AP");
            else gThrVSTiming->Draw("P SAME");

            // Increase color for next file
            colorIndex++;
        }
    }

    // Draw legends
    c1->cd(); leg1->Draw();
    c2->cd(); leg2->Draw();
    c3->cd(); leg3->Draw();


}
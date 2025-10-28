#include <iostream>
#include <ROOT/RNTuple.hxx>
#include <TH3D.h>
#include <TFile.h>

<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 6a22808 (Added Data plotting in summary_plots.c. All plotting capabailities of in_pixel_plots and compare_Data should now be implemented there.)
void Remove_everyNpoints(TGraph* g, int N, int startIndex = 0) 
{

    if (!g) return;
    int n = g->GetN();
    std::vector<double> xNew, yNew;
    xNew.reserve(n);
    yNew.reserve(n);

    for (int i = 0; i < n; ++i) {
        // Keep only points that are not every N-th after offset
        if ((i - startIndex) % N != 0) {
            double x, y;
            g->GetPoint(i, x, y);
            xNew.push_back(x);
            yNew.push_back(y);
        }
    }

    // Replace graph contents
    g->Set(n = xNew.size());
    for (int i = 0; i < n; ++i)
        g->SetPoint(i, xNew[i], yNew[i]);

}
<<<<<<< HEAD

void summary_plots()
{
    std::vector<int> runNumbers= {1};
    std::vector<std::string> runFiles = {"Nominal"};

    std::vector<std::string> labels = {"Simulation"};
=======
=======
>>>>>>> 6a22808 (Added Data plotting in summary_plots.c. All plotting capabailities of in_pixel_plots and compare_Data should now be implemented there.)

void summary_plots()
{
    std::vector<int> runNumbers= {1};
    std::vector<std::string> runFiles = {"Nominal"};

<<<<<<< HEAD
    std::vector<std::string> labels = {"Nominal"};
>>>>>>> 8149767 (Added digitization + tracking + clustering + analysis in an automatic fashion for each run)
=======
    std::vector<std::string> labels = {"Simulation"};
>>>>>>> 6a22808 (Added Data plotting in summary_plots.c. All plotting capabailities of in_pixel_plots and compare_Data should now be implemented there.)

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

<<<<<<< HEAD
<<<<<<< HEAD
            double threshold, efficiency, effError, timing, clSize, clSizeError;
=======
            double threshold, efficiency, effError, timing, clSize;
>>>>>>> 8149767 (Added digitization + tracking + clustering + analysis in an automatic fashion for each run)
=======
            double threshold, efficiency, effError, timing, clSize, clSizeError;
>>>>>>> 6a22808 (Added Data plotting in summary_plots.c. All plotting capabailities of in_pixel_plots and compare_Data should now be implemented there.)
            summaryTree->SetBranchAddress("threshold", &threshold);
            summaryTree->SetBranchAddress("efficiency", &efficiency);
            summaryTree->SetBranchAddress("effError", &effError);
            summaryTree->SetBranchAddress("clSize", &clSize);
<<<<<<< HEAD
<<<<<<< HEAD
            summaryTree->SetBranchAddress("clSizeError", &clSizeError);
=======
>>>>>>> 8149767 (Added digitization + tracking + clustering + analysis in an automatic fashion for each run)
=======
            summaryTree->SetBranchAddress("clSizeError", &clSizeError);
>>>>>>> 6a22808 (Added Data plotting in summary_plots.c. All plotting capabailities of in_pixel_plots and compare_Data should now be implemented there.)
            summaryTree->SetBranchAddress("timing", &timing);

            int nEntriesSummary = summaryTree->GetEntries();

<<<<<<< HEAD
<<<<<<< HEAD
            std::vector<double> vThr, vThrErr, vEff, vEffErr, vClSize, vClSizeErr, vTiming;
=======
            std::vector<double> vThr, vThrErr, vEff, vEffErr, vClSize, vTiming;
>>>>>>> 8149767 (Added digitization + tracking + clustering + analysis in an automatic fashion for each run)
=======
            std::vector<double> vThr, vThrErr, vEff, vEffErr, vClSize, vClSizeErr, vTiming;
>>>>>>> 6a22808 (Added Data plotting in summary_plots.c. All plotting capabailities of in_pixel_plots and compare_Data should now be implemented there.)
            vThr.reserve(nEntriesSummary);
            vThrErr.reserve(nEntriesSummary);
            vEff.reserve(nEntriesSummary);
            vEffErr.reserve(nEntriesSummary);
            vClSize.reserve(nEntriesSummary);
<<<<<<< HEAD
<<<<<<< HEAD
            vClSizeErr.reserve(nEntriesSummary);
=======
>>>>>>> 8149767 (Added digitization + tracking + clustering + analysis in an automatic fashion for each run)
=======
            vClSizeErr.reserve(nEntriesSummary);
>>>>>>> 6a22808 (Added Data plotting in summary_plots.c. All plotting capabailities of in_pixel_plots and compare_Data should now be implemented there.)
            vTiming.reserve(nEntriesSummary);

            for (int i = 0; i< nEntriesSummary; i++) { // replace 11 with 
                summaryTree->GetEntry(i);

                vThr.push_back(threshold);
<<<<<<< HEAD
<<<<<<< HEAD
                vThrErr.push_back(threshold * 0.03); // 3% quoted from calibration paper
                vEff.push_back(efficiency);
                vEffErr.push_back(effError);
                vClSize.push_back(clSize);
                vClSizeErr.push_back(clSizeError);
=======
                vThrErr.push_back(threshold/3.0); // 3% quoted from calibration paper
                vEff.push_back(efficiency);
                vEffErr.push_back(effError);
                vClSize.push_back(clSize);
>>>>>>> 8149767 (Added digitization + tracking + clustering + analysis in an automatic fashion for each run)
=======
                vThrErr.push_back(threshold * 0.03); // 3% quoted from calibration paper
                vEff.push_back(efficiency);
                vEffErr.push_back(effError);
                vClSize.push_back(clSize);
                vClSizeErr.push_back(clSizeError);
>>>>>>> 6a22808 (Added Data plotting in summary_plots.c. All plotting capabailities of in_pixel_plots and compare_Data should now be implemented there.)
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

<<<<<<< HEAD
<<<<<<< HEAD
            TGraphErrors *gThrVSClSize = new TGraphErrors(nEntriesSummary, vThr.data(),  vClSize.data(), vThrErr.data(), vClSizeErr.data());
            gThrVSClSize->SetTitle("Cluster size vs Threshold;Threshold [e-];<Cluster size>");
            gThrVSClSize->SetMarkerStyle(21);
            gThrVSClSize->SetMarkerSize(2.);
=======
            TGraph *gThrVSClSize = new TGraph(nEntriesSummary, vThr.data(),  vClSize.data());
            gThrVSClSize->SetTitle("Cluster size vs Threshold;Threshold [e-];Cluster size");
            gThrVSClSize->SetMarkerStyle(21);
>>>>>>> 8149767 (Added digitization + tracking + clustering + analysis in an automatic fashion for each run)
=======
            TGraphErrors *gThrVSClSize = new TGraphErrors(nEntriesSummary, vThr.data(),  vClSize.data(), vThrErr.data(), vClSizeErr.data());
            gThrVSClSize->SetTitle("Cluster size vs Threshold;Threshold [e-];<Cluster size>");
            gThrVSClSize->SetMarkerStyle(21);
            gThrVSClSize->SetMarkerSize(2.);
>>>>>>> 6a22808 (Added Data plotting in summary_plots.c. All plotting capabailities of in_pixel_plots and compare_Data should now be implemented there.)
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


<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 6a22808 (Added Data plotting in summary_plots.c. All plotting capabailities of in_pixel_plots and compare_Data should now be implemented there.)
    // Data plotting optional for data vs sim comparison

    TFile *dataInputLowIBIAS = TFile::Open("/home/vlad/Documents/Simu/Geant4/DECAL_REPO/root_macros/root_input/xybinsIDB100IBIAS05_Clsize_Xbin16_XNsteps1_Xstepsize0_Yfix-1.root");
    TFile *dataInputHighIBIAS = TFile::Open("/home/vlad/Documents/Simu/Geant4/DECAL_REPO/root_macros/root_input/xybinsIDB120IBIAS43_Clsize_Xbin16_XNsteps1_Xstepsize0_Yfix-1.root");
    if (!dataInputLowIBIAS || dataInputLowIBIAS->IsZombie() || !dataInputHighIBIAS || dataInputHighIBIAS->IsZombie()) {
        std::cerr << "Error: cannot open the input data files." << std::endl;
        return;
    }

    // Retrieve the data TMultiGraph. Generated by Lucian Fasselt
    TMultiGraph *clSizeDataLowIBIAS = (TMultiGraph*)dataInputLowIBIAS->Get("MultiClSize");
    TMultiGraph *clSizeDataHighIBIAS = (TMultiGraph*)dataInputHighIBIAS->Get("MultiClSize");
    if (!clSizeDataLowIBIAS || !clSizeDataHighIBIAS) 
    {
        std::cerr << "Error: could not find TMultiGraph for either one of the files." << std::endl;\
        return;
    }


    //Extract the first (and probably only) TGraph from each MultiGraph Object
    TGraph *gLow  = (TGraph*)clSizeDataLowIBIAS->GetListOfGraphs()->At(0);
    TGraph *gHigh = (TGraph*)clSizeDataHighIBIAS->GetListOfGraphs()->At(0);

    // Remove points for over zealous data takers
    for (int i = 0; i < 3; i++)
    {
        Remove_everyNpoints(gLow, 2, 1);
    }

    // Merge high and low threshold data sets
    TGraphErrors *clSizeDataMerged = new TGraphErrors();
    int nMerged = 0;

     // Add points from the low threshold between 200 and 720
    for (int i = 0; i < gHigh->GetN(); ++i) {
        double x, y;
        gHigh->GetPoint(i, x, y);
        if (x >= 200 && x <= 720) {
            clSizeDataMerged->SetPoint(nMerged, x, y);
            clSizeDataMerged->SetPointError(nMerged,x *0.03, 0.0); // TODO: Get automated error bar for cluster size
            nMerged++;
        }
    }

    // --- Add points from g1 for x > 700 ---
    for (int i = 0; i < gLow->GetN(); ++i) {
        double x, y;
        gLow->GetPoint(i, x, y);
        if (x > 900 && x <= 2000) {
            clSizeDataMerged->SetPoint(nMerged, x, y);
            clSizeDataMerged->SetPointError(nMerged,x *0.03, 0.0); // TODO: Get automated error bar for cluster size
            nMerged++;
        }
    }   
    c2->cd();

    
    clSizeDataMerged->SetMarkerStyle(29);
    clSizeDataMerged->SetMarkerSize(3.);
    clSizeDataMerged->SetMarkerColor(kRed);
    clSizeDataMerged->SetLineColor(kRed);
    clSizeDataMerged->SetLineWidth(2.4);
    clSizeDataMerged->Draw("PL SAME");
    leg2->AddEntry(clSizeDataMerged, "Data", "p");
    leg2->Draw();


<<<<<<< HEAD
=======
>>>>>>> 8149767 (Added digitization + tracking + clustering + analysis in an automatic fashion for each run)
=======
>>>>>>> 6a22808 (Added Data plotting in summary_plots.c. All plotting capabailities of in_pixel_plots and compare_Data should now be implemented there.)
}
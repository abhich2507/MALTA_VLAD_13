#include "Analysis.hh"

double getEff(int Npassed, int Nall) {
    if (Nall == 0) return 0.0;  // avoid division by zero
    return ((double)Npassed / (double)Nall) * 100.0;
}

// Error in percent, assuming binomial distribution
double getEffErr(int Npassed, int Nall) {
    if (Nall == 0) return 0.0;  // avoid division by zero
    double ratio = (double)Npassed / (double)Nall;
    return 100.0 * sqrt(ratio * (1.0 - ratio) / (double)Nall);
}


void Analysis(double threshold, int runNumber = 91, std::string saveName = "default")
{
    auto start = std::chrono::high_resolution_clock::now();

    std::string localPath = getVarFromConfig();
    std::string inputPath = localPath +  Form("Results/local_%04d/", runNumber) + saveName + "/";

    std::cout << "############################# Analysis started for:" << std::endl;
    std::cout << inputPath << std::endl;


    TRandom3 rng(0);  // 0 = use machine clock for seed
    double trackunc_X = 4.6/1000.; // tracking uncertainty in X in unit mm
    double trackunc_Y = 4.6/1000.; // tracking uncertainty in X in unit mm
    int nX = 2*16, nY = 2*16, nZ = 100;
    double pixelSizeX = 0.0364 , pixelSizeY = 0.0364; // in mm

    TFile *analysisFile = TFile::Open((inputPath + "analysisThr" + std::to_string(int(threshold)) + ".root").c_str(), "READ");


    // Get tree
    TTree *analysisTree = (TTree*) analysisFile->Get("analyzedHits");

    double fX, fY, timing;
    int clSize;

    analysisTree->SetBranchAddress("analysisVertexX", &fX);
    analysisTree->SetBranchAddress("analysisVertexY", &fY);
    analysisTree->SetBranchAddress("clSize", &clSize);  
    analysisTree->SetBranchAddress("timing", &timing); 

    // Full Matrix histograms
    TH2D *h2ALL    = new TH2D("h2ALL", "h2ALL", 100, 50 - 18.6/2, 50 + 18.6/2, 100, 50 - 18.6/2, 50 + 18.6/2);
    TH2D *h2PASS   = new TH2D("h2PASS", "h2PASS", 100, 50 - 18.6/2, 50 + 18.6/2, 100, 50 - 18.6/2, 50 + 18.6/2);
    TH2D *h2ClSize = new TH2D("h2ClSize", "h2ClSize", 100, 50 - 18.6/2, 50 + 18.6/2, 100, 50 - 18.6/2, 50 + 18.6/2);
    TH2D *h2Timing = new TH2D("h2Timing", "h2Timing", 100, 50 - 18.6/2, 50 + 18.6/2, 100, 50 - 18.6/2, 50 + 18.6/2);

    // In-pixel histrograms
    TH2D *h2ALLInPixel   = new TH2D("h2ALLInPixel", "h2ALLInPixel", nX, 0, 2*pixelSizeX*1000, nY, 0, 2*pixelSizeY *1000);
    TH2D *h2PASSInPixel  = new TH2D("h2PASSInPixel", "h2PASSInPixel", nX, 0, 2*pixelSizeX*1000, nY, 0, 2*pixelSizeY *1000);
    TH2D *h2ClSizeInPixel= new TH2D("h2ClSizeInPixel", "h2ClSizeInPixel", nX, 0, 2*pixelSizeX*1000, nY, 0, 2*pixelSizeY *1000);
    TH2D *h2TimingInPixel= new TH2D("h2TimingInPixel", "h2TimingInPixel", nX, 0, 2*pixelSizeX*1000, nY, 0, 2*pixelSizeY *1000);



    Long64_t nAnalyzedEntries = analysisTree->GetEntries();
    std::cout << nAnalyzedEntries << std::endl;
    // Fill the histograms with events
    for (int i =0; i< nAnalyzedEntries; i++)
    {
        analysisTree->GetEntry(i);
        //std::cout << "clSize: " << clSize << std::endl;
        h2ALL   ->Fill(fX, fY, 1);
        h2PASS  ->Fill(fX, fY, clSize > 0 ? 1 : 0);
        h2ClSize->Fill(fX, fY, clSize);
        h2Timing->Fill(fX, fY, timing);

        double foldedX = fmod(fX + rng.Gaus(0., trackunc_X), 2*pixelSizeX) * 1000; 
        double foldedY = fmod(fY + rng.Gaus(0., trackunc_Y), 2*pixelSizeX) * 1000; 

        //double foldedX = fmod(fX, 2*pixelSizeX) * 1000; 
        //double foldedY = fmod(fY, 2*pixelSizeY) * 1000;

        h2ALLInPixel   ->Fill(foldedX, foldedY, 1);
        h2PASSInPixel  ->Fill(foldedX, foldedY, clSize > 0 ? 1 : 0);
        h2ClSizeInPixel->Fill(foldedX, foldedY, clSize);
        h2TimingInPixel->Fill(foldedX, foldedY, timing);

    }

    h2PASS  ->Divide(h2ALL);
    h2PASS  ->Scale(100.);
    h2ClSize->Divide(h2ALL);
    h2Timing->Divide(h2ALL);


    // Compute average values:
    double avgEff = getEff(h2PASSInPixel->Integral(), h2ALLInPixel->Integral());// in percent
    double errEff = getEffErr(h2PASSInPixel->Integral(), h2ALLInPixel->Integral());// in percent

    double avgTiming = h2TimingInPixel->Integral() / h2PASSInPixel->Integral();

    TH2D *h2PASSInPixelAux = (TH2D*)h2PASSInPixel->Clone("h2PASSInPixel");

    h2ClSizeInPixel->Divide(h2PASSInPixelAux);
    h2PASSInPixel->Divide(h2ALLInPixel);
    h2PASSInPixel->Scale(100.);
    h2TimingInPixel->Divide(h2PASSInPixelAux);

    double weightedSum = 0.0;
    double totalWeight = 0.0;
    int nx = h2ClSizeInPixel->GetNbinsX();
    int ny = h2ClSizeInPixel->GetNbinsY();
    for (int ix = 1; ix <= nx; ++ix) {
        for (int iy = 1; iy <= ny; ++iy) {
            double content = h2ClSizeInPixel->GetBinContent(ix, iy);
            if (content <= 0) continue; // skip empty or negative bins
            weightedSum += content * content; // weight by content
            totalWeight += content;
        }
    }

    double avgClSize = (totalWeight > 0) ? weightedSum / totalWeight : 0.0;


    // Set histogram titles:
    h2ClSizeInPixel->SetTitle( Form("In-pixel cluster size = %.2f;Track X pos [#mum];Track Y pos [#mum];Cluster size", avgClSize) );

    h2PASSInPixel->SetTitle( Form("In-pixel eff. = %.2f %% pm %.2f %% ;Track X pos [#mum];Track Y pos [#mum]; Eff. [%%] ", avgEff, errEff) );

    h2TimingInPixel->SetTitle( Form("In-pixel timing. = %.2f ns ;Track X pos [#mum];Track Y pos [#mum]; Timing [ns] ", avgTiming) );
    // Draw canvases


    // Plot save path
    std::string directoryPath = localPath +"/Plots/";
    std::string runPath = Form("local_%04d/", runNumber);

    // Write histograms into the directory
    savePlot(directoryPath, runPath, threshold, saveName, h2PASS, "h2PASS");
    savePlot(directoryPath, runPath, threshold, saveName, h2ClSize, "h2ClSize");
    savePlot(directoryPath, runPath, threshold, saveName, h2Timing, "h2Timing");
    savePlot(directoryPath, runPath, threshold, saveName, h2PASSInPixel, "h2PASSInPixel");
    savePlot(directoryPath, runPath, threshold, saveName, h2ClSizeInPixel, "h2ClSizeInPixel");
    savePlot(directoryPath, runPath, threshold, saveName, h2TimingInPixel, "h2TimingInPixel");
    savePlot(directoryPath, runPath, threshold, saveName, h2PASSInPixelAux, "h2PASSInPixelAux");
    savePlot(directoryPath, runPath, threshold, saveName, h2ALLInPixel, "h2ALLInPixel");

    // Lastly populate a root tree with the average values for later summary plotting
    // Try opening the file in UPDATE mode (read + write)
    std::string summaryPath = (directoryPath + runPath + saveName + "/summary.root").c_str();
    double summaryThreshold, summaryEff, summaryEffErr, summaryClSize, summaryTiming;

    TFile *f = TFile::Open(summaryPath.c_str(), "UPDATE");
    if (!f || f->IsZombie()) {
        std::cout << "Creating new file: " << summaryPath << std::endl;
        f = new TFile(summaryPath.c_str(), "RECREATE");
    }

    // Check if the tree exists
    TTree *summaryTree = (TTree*) f->Get("summaryTree");
    if (!summaryTree) {
        std::cout << "Creating new summary tree" << std::endl;
        summaryTree = new TTree("summaryTree", "summary Tree");
        summaryTree->Branch("threshold", &summaryThreshold, "threshold/D");
        summaryTree->Branch("efficiency", &summaryEff, "efficiency/D");
        summaryTree->Branch("effError", &summaryEffErr, "effError/D");
        summaryTree->Branch("clSize", &summaryClSize, "clSize/D");
        summaryTree->Branch("timing", &summaryTiming, "timing/D");


    } else {
        std::cout << "Appending to existing tree" << std::endl;
    }

    summaryTree->SetBranchAddress("threshold", &summaryThreshold);
    summaryTree->SetBranchAddress("efficiency",  &summaryEff);
    summaryTree->SetBranchAddress("effError",  &summaryEffErr);
    summaryTree->SetBranchAddress("clSize", &summaryClSize);
    summaryTree->SetBranchAddress("timing", &summaryTiming);

    summaryThreshold = threshold;
    summaryEff = avgEff;
    summaryEffErr = errEff;
    summaryClSize = avgClSize;
    summaryTiming = avgTiming;
    summaryTree->Fill();

    std::cout << "Saving values: " << "Threshold: " << threshold << "; avgEff: " << avgEff << "; errEff: " << errEff << "; avgClSize: " << avgClSize << ";avgTiming: " << avgTiming << std::endl;

    f->cd();
    summaryTree->Write("", TObject::kOverwrite); // overwrite tree object in directory
    f->Close();
    delete f;
    
    auto end = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double, std::milli> elapsed = end - start;

    std::cout << "############################# Analysis stopped after " << elapsed.count() << "ms" << std::endl;
}
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
    auto analysisFlags = new SimFlags;
    const char* configPath = std::getenv("ANALYSIS_CONFIG");
    LoadAnalysisFlagsFromFile(configPath, *analysisFlags);
    // TODO: Implement verbose. Should print out the correct creation of histgrams for example.
    bool verbose = analysisFlags->verboseAnalysis;
    ////////// Function can be used for custom analysis paths
    //std::string localPath = getVarFromConfig();
    //////////////////////////////////////////////////////////
    std::string localPath = analysisFlags->localPath;
    std::string inputPath = analysisFlags->inputPath+Form("_%04d/", runNumber)+ saveName + "/";
    // Quick hack for fast reanalysis:
    //std::string inputPath = localPath +  Form("Results/local_%04d/", runNumber) + "Final/";

    std::cout << "############################# Analysis started for:" << std::endl;
    std::cout << inputPath << std::endl;

    // Values are already set by GEANT4 simulation. If generalization is required the vlues need to come from sim config
    TRandom3 rng(0);  // 0 = use machine clock for seed
    double trackunc_X = 4.6/1000.; // tracking uncertainty in X in unit mm
    double trackunc_Y = 4.6/1000.; // tracking uncertainty in X in unit mm
    int numPixlesX = 2;
    int numPixlesY = 2;
    int nX = numPixlesX*16, nY = numPixlesY*16, nZ = 100;
    //int nX = 10*16, nY = 10*16, nZ = 100;
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
    TH2D *h2ALLInPixel   = new TH2D("h2ALLInPixel", "h2ALLInPixel", nX, 0, numPixlesX*pixelSizeX*1000, nY, 0, numPixlesY*pixelSizeY *1000);
    TH2D *h2PASSInPixel  = new TH2D("h2PASSInPixel", "h2PASSInPixel", nX, 0, numPixlesX*pixelSizeX*1000, nY, 0, numPixlesY*pixelSizeY *1000);
    TH2D *h2ClSizeInPixel= new TH2D("h2ClSizeInPixel", "h2ClSizeInPixel", nX, 0, numPixlesX*pixelSizeX*1000, nY, 0, numPixlesY*pixelSizeY *1000);
    TH2D *h2TimingInPixel= new TH2D("h2TimingInPixel", "h2TimingInPixel", nX, 0, numPixlesX*pixelSizeX*1000, nY, 0, numPixlesY*pixelSizeY *1000);
    TH2D *h2MissMergedInPixel= new TH2D("h2MissMergedInPixel", "h2MissMergedInPixel", nX, 0, 0, nY, numPixlesX, numPixlesY);

    // Projections
    TH1D *h1PASSInPixelXProj = new TH1D("h1PASSInPixelXProj", "h1PASSInPixelXProj", nX, 0, numPixlesX*pixelSizeX*1000);
    TH1D *h1PASSInPixelYProj = new TH1D("h1PASSInPixelYProj", "h1PASSInPixelYProj", nY, 0, numPixlesY*pixelSizeY*1000);

    Long64_t nAnalyzedEntries = analysisTree->GetEntries();
    std::cout << nAnalyzedEntries << std::endl;

    double trackOffsetX = analysisFlags->trackOffsetX;
    double trackOffsetY = analysisFlags->trackOffsetY;

    // Fill the histograms with events
    for (int i =0; i< nAnalyzedEntries; i++)
    {
        analysisTree->GetEntry(i);
        //std::cout << "clSize: " << clSize << std::endl;
        h2ALL   ->Fill(fX, fY, 1);
        h2PASS  ->Fill(fX, fY, clSize > 0 ? 1 : 0);
        h2ClSize->Fill(fX, fY, clSize);
        h2Timing->Fill(fX, fY, timing);
        double foldedX, foldedY;
        
        if(analysisFlags->trkUnc == true)
        {
            foldedX = fmod(fX + trackOffsetX + rng.Gaus(0., trackunc_X), numPixlesX*pixelSizeX) * 1000;  //- 50 + 18.6368 /2
            foldedY = fmod(fY + trackOffsetY + rng.Gaus(0., trackunc_Y), numPixlesY*pixelSizeY) * 1000;  //- 50 + 18.6368 /2
        } 
        else
        {        
            foldedX = fmod(fX + trackOffsetX, numPixlesX*pixelSizeX) * 1000;  //- 50 + 18.6368 /2
            foldedY = fmod(fY + trackOffsetY, numPixlesY*pixelSizeY) * 1000;   // - 50 + 18.6368 /2

        }
        

        //foldedX = (fX - 56.159 +4*0.0364) *1000;
        //foldedY = (fY - 51.00924+ 18*0.0364) *1000;
        /*
        if(foldedX >= 22.75 && foldedX <= 43.2 && foldedY >= 482 && foldedY <= 536)
        {
            std::cout << fX << "; " <<fY <<std::endl;
        }
        */

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
    double avgClSize = h2ClSizeInPixel->Integral() / h2PASSInPixel->Integral();

    TH2D *h2PASSInPixelAux = (TH2D*)h2PASSInPixel->Clone("h2PASSInPixel");

    h2ClSizeInPixel->Divide(h2PASSInPixelAux);
    h2PASSInPixel->Divide(h2ALLInPixel);
    h2PASSInPixel->Scale(100.);
    h2TimingInPixel->Divide(h2PASSInPixelAux);

    double errClSize = getEffErr(h2ClSizeInPixel->Integral(), h2PASSInPixelAux->Integral());


    h1PASSInPixelXProj = h2PASSInPixel->ProjectionX();
    h1PASSInPixelYProj = h2PASSInPixel->ProjectionY();

    // Set histogram titles:
    h2ClSizeInPixel->SetTitle( Form("#bf{MALTA2 Sim.}, 30#mum EPI, <cl. size> =%.2f;Track X pos [#mum];Track Y pos [#mum];Cluster size", avgClSize) );
    h2PASSInPixel->SetTitle( Form("In-pixel eff. = %.2f %% pm %.2f %% ;Track X pos [#mum];Track Y pos [#mum]; Eff. [%%] ", avgEff, errEff) );
    h2TimingInPixel->SetTitle( Form("In-pixel timing. = %.2f ns ;Track X pos [#mum];Track Y pos [#mum]; Timing [ns] ", avgTiming) );
    h1PASSInPixelXProj->SetTitle("In-pixel eff.;Track X pos [#mum];Eff.[%]");
    h1PASSInPixelYProj->SetTitle("In-pixel eff.;Track Y pos [#mum];Eff.[%]");
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
    savePlot(directoryPath, runPath, threshold, saveName, h1PASSInPixelXProj, "h1PASSInPixelXProj");
    savePlot(directoryPath, runPath, threshold, saveName, h1PASSInPixelYProj, "h1PASSInPixelYProj");
    /*
    // Look at miss merged hits only. This is saved previously in DigitalProcessing.cc
    std::string histosPath = (directoryPath + runPath + saveName + "/histos.root").c_str();
    // Check if the tree exists
    TFile *histos = TFile::Open(histosPath.c_str(), "UPDATE");
    if (!histos || histos->IsZombie()) {
        std::cout << "Creating new file: " << histosPath << std::endl;
        histos = new TFile(histosPath.c_str(), "RECREATE");
    }
    TDirectory *dirMerger = (TDirectory*)histos->Get(Form("Thr%i", int(threshold)));
    TH2D *h2MissMerged = (TH2D*) dirMerger->Get("h2MissMerged");

    for (int ix = 1; ix <= h2MissMerged->GetNbinsX(); ix++) 
    {
        for (int iy = 1; iy <= h2MissMerged->GetNbinsY(); iy++) 
        {

            double content = h2MissMerged->GetBinContent(ix, iy);
            double xCenter = h2MissMerged->GetXaxis()->GetBinCenter(ix);
            double yCenter = h2MissMerged->GetYaxis()->GetBinCenter(iy);
            double foldedX = fmod(xCenter, numPixlesX);
            double foldedY = fmod(yCenter, numPixlesY);   
            h2MissMergedInPixel->Fill(foldedX, foldedY, 1);

        }
    }
    dirMerger->cd();                  
    h2MissMergedInPixel->Write("", TObject::kOverwrite);
    histos->Write();                  
    histos->Close();
    */


    // Lastly populate a root tree with the average values for later summary plotting
    // Try opening the file in UPDATE mode (read + write)
    std::string summaryPath = (directoryPath + runPath + saveName + "/summary.root").c_str();
    double summaryThreshold, summaryEff, summaryEffErr, summaryClSize, summaryClSizeErr, summaryTiming;

    TFile *f = TFile::Open(summaryPath.c_str(), "UPDATE");
    if (!f || f->IsZombie()) {
        std::cout << "Creating new file: " << summaryPath << std::endl;
        f = new TFile(summaryPath.c_str(), "RECREATE");
    }
    // Check if the tree exists
    TTree *summaryTree = (TTree*) f->Get("summaryTree");
    if (!summaryTree) 
    {
        std::cout << "Creating new summary tree" << std::endl;
        summaryTree = new TTree("summaryTree", "summary Tree");
        summaryTree->Branch("threshold", &summaryThreshold, "threshold/D");
        summaryTree->Branch("efficiency", &summaryEff, "efficiency/D");
        summaryTree->Branch("effError", &summaryEffErr, "effError/D");
        summaryTree->Branch("clSize", &summaryClSize, "clSize/D");
        summaryTree->Branch("clSizeError", &summaryClSizeErr, "clSizeError/D");
        summaryTree->Branch("timing", &summaryTiming, "timing/D");
    } 
    else 
    {
        std::cout << "Appending to existing tree" << std::endl;
    }

    summaryTree->SetBranchAddress("threshold", &summaryThreshold);
    summaryTree->SetBranchAddress("efficiency",  &summaryEff);
    summaryTree->SetBranchAddress("effError",  &summaryEffErr);
    summaryTree->SetBranchAddress("clSize", &summaryClSize);
    summaryTree->SetBranchAddress("clSizeError", &summaryClSizeErr);
    summaryTree->SetBranchAddress("timing", &summaryTiming);

    summaryThreshold = threshold;
    summaryEff = avgEff;
    summaryEffErr = errEff;
    summaryClSize = avgClSize;
    summaryClSizeErr = errClSize;
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
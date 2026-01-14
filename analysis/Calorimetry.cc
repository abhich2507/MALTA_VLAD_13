#include "Tracking.hh"

void Calorimetry(double threshold, int runNumber, std::string saveName)
{

    auto start = std::chrono::high_resolution_clock::now();
    ////////// Function can be used for custom analysis paths
    //std::string localPath = getVarFromConfig();
    //////////////////////////////////////////////////////////
    std::string localPath = "./";
    std::string inputPath = localPath +  Form("Results/local_%04d/", runNumber);
    std::string inputSubPath = localPath +  Form("Results/local_%04d/", runNumber) + saveName + "/";
    std::string directoryPath = localPath + "Plots/";
    std::string runPath = Form("local_%04d/", runNumber);

    DetectorConfig cfg = LoadConfig(localPath + Form("Results/local_%04d/flags.cfg", runNumber));

    std::cout << "############################# Tracking started for:" << std::endl;
    std::cout << inputPath << std::endl;

    auto analysisFlags = new SimFlags;
    const char* configPath = std::getenv("ANALYSIS_CONFIG");
    LoadAnalysisFlagsFromFile(configPath, *analysisFlags);
    bool verbose = analysisFlags->verboseTracking;

    double dCut = analysisFlags->distCut;
    double matchWindow = analysisFlags->timeCut;
    double vetoValue = analysisFlags->veto;

    TFile *reconstructedFile = TFile::Open((inputSubPath + "Plane0ReconstructedHitsThr" + std::to_string(int(threshold)) + ".root").c_str(), "READ");


    // Get tree
    TTree *reconstructedTree = (TTree*) reconstructedFile->Get("ReconstructedHits");

    int reconstructedPixX, reconstructedPixY;
    double reconstructedTiming;
    reconstructedTree->SetBranchAddress("PixX", &reconstructedPixX);
    reconstructedTree->SetBranchAddress("PixY", &reconstructedPixY);
    reconstructedTree->SetBranchAddress("timing", &reconstructedTiming);
    Long64_t nReconstructedEntries = reconstructedTree->GetEntries();

    // Save to file

    TFile *outfile = new TFile((inputSubPath + "CalorimetryThr" + std::to_string(int(threshold)) + ".root").c_str(), "RECREATE");

    // Create a TTree
    TTree *caloTree = new TTree("CaloHits", "Calorimetry Hits");

    // Variables for branches
    int numSecondaries = 0;
    int eventID = 0;

    // Create branches
    caloTree->Branch("eventID", &eventID, "eventID/I");
    caloTree->Branch("numSecondaries", &numSecondaries, "numSecondaries/I");

    double timeWindow = vetoValue;
    for (Long64_t i = 0; i < nReconstructedEntries; i++)
    {
        reconstructedTree->GetEntry(i);
        int pixX = reconstructedPixX;
        int pixY = reconstructedPixY;
        double timing = reconstructedTiming;
        //std::cout << "pixX: " << pixX << " pixY: " << pixY << " timing: " << timing << std::endl;
        if (timing < timeWindow)
        {
            numSecondaries++;
        }
        else
        {
            timeWindow+= vetoValue;
            caloTree->Fill();
            numSecondaries = 0;
            eventID++;
        }
    }









    outfile->cd();
    auto end = std::chrono::high_resolution_clock::now(); 
    std::chrono::duration<double, std::milli> elapsed = end - start;     

    caloTree->Write();
    outfile->Close();

    std::cout << "############################# Tracking stopped after " << elapsed.count() << " ms" << std::endl;

}
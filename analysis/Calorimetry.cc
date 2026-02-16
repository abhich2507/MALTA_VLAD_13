#include "Tracking.hh"

struct Pixel
{
    int x;
    int y;
    // Overload == operator
    bool operator==(const Pixel& other) const 
    {
        return x == other.x && y == other.y;
    }
};
// Hash value creation for XOR cluster ranking if a ==b hash(a) == hash(b)
struct PixelHash 
{
    std::size_t operator()(const Pixel& p) const 
    {
        return std::hash<int>()(p.x) ^ (std::hash<int>()(p.y) << 1);
    }
};

void Calorimetry(float threshold, int runNumber, std::string saveName)
{

    const int dx[4] = { 1, -1,  0,  0 };
    const int dy[4] = { 0,  0,  1, -1 };

    auto start = std::chrono::high_resolution_clock::now();
    // Set all the analysis flags for the digital processing
    auto analysisFlags = new SimFlags;
    const char* configPath = std::getenv("ANALYSIS_CONFIG");
    LoadAnalysisFlagsFromFile(configPath, *analysisFlags);
    ////////// Function can be used for custom analysis paths
    //std::string localPath = getVarFromConfig();
    //////////////////////////////////////////////////////////
    std::string localPath = analysisFlags->localPath;
    std::string inputPath = analysisFlags->inputPath+Form("_%04d/", runNumber);
    std::string inputSubPath = inputPath + saveName + "/";
    std::string directoryPath = localPath + "Plots/";
    std::string runPath = Form("local_%04d/", runNumber);

    DetectorConfig cfg = LoadConfig(inputPath + "flags.cfg");

    std::cout << "############################# Calorimetry started for:" << std::endl;
    std::cout << inputPath << std::endl;

    bool verbose = analysisFlags->verboseTracking;

    float dCut = analysisFlags->distCut;
    float matchWindow = analysisFlags->timeCut;
    float vetoValue = analysisFlags->veto;

    TFile *reconstructedFile = TFile::Open((inputSubPath + "Plane0ReconstructedHitsThr" + std::to_string(int(threshold)) + ".root").c_str(), "READ");


    // Get tree
    TTree *reconstructedTree = (TTree*) reconstructedFile->Get("ReconstructedHits");

    int reconstructedPixX, reconstructedPixY;
    double reconstructedTiming_float;
    reconstructedTree->SetBranchAddress("PixX", &reconstructedPixX);
    reconstructedTree->SetBranchAddress("PixY", &reconstructedPixY);
    reconstructedTree->SetBranchAddress("timing", &reconstructedTiming_float);
    Long64_t nReconstructedEntries = reconstructedTree->GetEntries();

    // Save to file

    TFile *outfile = new TFile((inputSubPath + "CalorimetryThr" + std::to_string(int(threshold)) + ".root").c_str(), "RECREATE");

    // Create a TTree
    TTree *caloTree = new TTree("CaloHits", "Calorimetry Hits");

    // Variables for branches
    int numSecondaries = 0;
    int numClusters = 0;
    int eventID = 0;

    // Create branches
    caloTree->Branch("eventID", &eventID, "eventID/I");
    caloTree->Branch("numSecondaries", &numSecondaries, "numSecondaries/I");
    caloTree->Branch("numClusters", &numClusters, "numClusters/I");

    float timeWindow = vetoValue;
    std::unordered_set<Pixel, PixelHash> pixelsToCluster{};
    for (Long64_t i = 0; i < nReconstructedEntries; i++)
    {
        reconstructedTree->GetEntry(i);
        double reconstructedTiming = static_cast<float>(reconstructedTiming_float);
        int pixX = reconstructedPixX;
        int pixY = reconstructedPixY;
        float timing = reconstructedTiming;
        //std::cout << "pixX: " << pixX << " pixY: " << pixY << " timing: " << timing << std::endl;
        if (timing < timeWindow)
        {
            numSecondaries++;
            pixelsToCluster.insert({pixX, pixY});
            
            
        }
        else
        {
            timeWindow+= vetoValue;

            // Hash based unordered set look-up algorithm
            while (!pixelsToCluster.empty()) 
            {
                numClusters++;
                std::stack<Pixel> toVisit;
                Pixel start = *pixelsToCluster.begin();
                toVisit.push(start);
                pixelsToCluster.erase(start);
                while (!toVisit.empty()) 
                {
                    Pixel p = toVisit.top();
                    toVisit.pop();
                    for (int i = 0; i < 4; ++i) 
                    {
                        Pixel neighbor{p.x + dx[i], p.y + dy[i]};
                        auto it = pixelsToCluster.find(neighbor);
                        if (it != pixelsToCluster.end()) 
                        {
                            toVisit.push(neighbor);
                            pixelsToCluster.erase(it);
                        }
                    }
                }
            }
            caloTree->Fill();
            numSecondaries = 0;
            numClusters = 0;
            pixelsToCluster.clear();
            eventID++;
        }
    }









    outfile->cd();
    auto end = std::chrono::high_resolution_clock::now(); 
    std::chrono::duration<float, std::milli> elapsed = end - start;     

    caloTree->Write();
    outfile->Close();

    std::cout << "############################# Calorimetry stopped after " << elapsed.count() << " ms" << std::endl;

}
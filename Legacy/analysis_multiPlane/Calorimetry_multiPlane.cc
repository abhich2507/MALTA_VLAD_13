#include "DigitalProcessing.hh"
#include "Tracking_multiPlane.hh"

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


struct PlaneState 
{
    float timeWindow = 0;
    int numSecondaries = 0;
    int numClusters = 0;
    std::unordered_set<Pixel, PixelHash> pixels;
};

auto computeStats = [](const std::vector<float>& v, float& mean, float& sigma)
{
    
    //Mean and Sigma computation helper function
    
    if (v.empty()) {
        mean = 0;
        sigma = 0;
        return;
    }

    float sum = 0;
    for (auto val : v) sum += val;
    mean = sum / v.size();

    float var = 0;
    for (auto val : v) var += (val - mean) * (val - mean);
    sigma = std::sqrt(var / v.size());
};


void Calorimetry_multiPlane(float threshold, int runNumber, std::string saveName)
{

    const int dx[4] = { 1, -1,  0,  0 };
    const int dy[4] = { 0,  0,  1, -1 };

    auto start = std::chrono::high_resolution_clock::now();
    // Set all the analysis flags for the digital processing
    auto analysisFlags = new AnaFlags;
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

    std::cout << "############################# Calorimetry MultiPlane started for:" << std::endl;
    std::cout << inputPath << std::endl;

    bool verbose = analysisFlags->verboseTracking;

    float dCut = analysisFlags->distCut;
    float matchWindow = analysisFlags->timeCut;
    float vetoValue = analysisFlags->veto;
    int nPlanes_100 = analysisFlags->nPlanes_100;
    int nPlanes_10 = analysisFlags->nPlanes_10;
    int nPlanes_1 = analysisFlags->nPlanes_1;
    int nPlanes = nPlanes_100*nPlanes_10*nPlanes_1;

    TFile *reconstructedFile = TFile::Open((inputSubPath + "Plane0ReconstructedHitsThr" + std::to_string(int(threshold)) + ".root").c_str(), "READ");


    // Get Calo tree
    TTree *reconstructedTree = (TTree*) reconstructedFile->Get("ReconstructedHits");

    int reconstructedPixX, reconstructedPixY, NHits, planeID;
    double reconstructedTiming_float;
    reconstructedTree->SetBranchAddress("planeID", &planeID);
    reconstructedTree->SetBranchAddress("PixX", &reconstructedPixX);
    reconstructedTree->SetBranchAddress("PixY", &reconstructedPixY);
    reconstructedTree->SetBranchAddress("timing", &reconstructedTiming_float);
    reconstructedTree->SetBranchAddress("NHits", &NHits);
    Long64_t nReconstructedEntries = reconstructedTree->GetEntries();


    // Get Pos tree
    TTree *positionTree = (TTree*) reconstructedFile->Get("PositionHits");

    int posPlaneID, stripX, stripY;
    double posTiming;
    positionTree->SetBranchAddress("planeID", &posPlaneID);
    positionTree->SetBranchAddress("stripX", &stripX);
    positionTree->SetBranchAddress("stripY", &stripY);
    positionTree->SetBranchAddress("timing", &posTiming);
    Long64_t nPositionEntries = positionTree->GetEntries();

    // Save to file

    TFile *outfile = new TFile((inputSubPath + "CalorimetryThr" + std::to_string(int(threshold)) + ".root").c_str(), "RECREATE");

    // Create a TTree
    TTree *caloTree = new TTree("CaloHits", "Calorimetry Hits");

    // Variables for branches
    int numSecondaries = 0;
    int numClusters = 0;
    int eventNum = 0;
    int planeNum = -1;
    float meanX = 0, meanY = 0, sigmaX = 0, sigmaY = 0;
    // Create branches
    caloTree->Branch("meanX", &meanX, "meanX/F");
    caloTree->Branch("meanY", &meanY, "meanY/F");
    caloTree->Branch("sigmaX", &sigmaX, "sigmaX/F");
    caloTree->Branch("sigmaY", &sigmaY, "sigmaY/F");
    caloTree->Branch("planeID", &planeNum, "planeID/I");
    caloTree->Branch("eventID", &eventNum, "eventID/I");
    caloTree->Branch("numSecondaries", &numSecondaries, "numSecondaries/I");
    caloTree->Branch("numClusters", &numClusters, "numClusters/I");

    std::unordered_set<Pixel, PixelHash> pixelsToCluster{};
    std::unordered_map<int, PlaneState> planeStates;
    std::vector<int> planes;
    cout << "Adding Planes to analysis: ";
    for (int iz = 0; iz < nPlanes_100; ++iz) {
        for (int iy = 0; iy < nPlanes_10; ++iy) {
            for (int ix = 0; ix < nPlanes_1; ++ix) {
                planes.push_back(iz*10000 + iy*100 + ix); // decoded position (works for up to 10 planes in each dimension)
                cout << iz*10000 + iy*100 + ix << ", " ;
            }
        }
    }

    std::unordered_map<int, Long64_t> posIndexMap;
    std::unordered_map<int, Long64_t> eventIDMap;

    for (Long64_t i = 0; i < nReconstructedEntries; i++)
    {
        reconstructedTree->GetEntry(i);

        int pixX = reconstructedPixX;
        int pixY = reconstructedPixY;
        float timing = static_cast<float>(reconstructedTiming_float);

        auto& state = planeStates[planeID];

        // Initialize time window on first hit
        if (state.timeWindow == 0)
            state.timeWindow = vetoValue;

        if (timing < state.timeWindow)
        {
            if (analysisFlags->boolHWC)
                state.numSecondaries += NHits;
            else
                state.numSecondaries++;

            state.pixels.insert({pixX, pixY});
        }
        else
        {
            // Clustering
            while (!state.pixels.empty())
            {
                state.numClusters++;
                std::stack<Pixel> toVisit;
                Pixel start = *state.pixels.begin();
                toVisit.push(start);
                state.pixels.erase(start);

                while (!toVisit.empty())
                {
                    Pixel p = toVisit.top();
                    toVisit.pop();

                    for (int d = 0; d < 4; ++d)
                    {
                        Pixel neighbor{p.x + dx[d], p.y + dy[d]};
                        auto it = state.pixels.find(neighbor);
                        if (it != state.pixels.end())
                        {
                            toVisit.push(neighbor);
                            state.pixels.erase(it);
                        }
                    }
                }
            }

            // Output
            planeNum = planeID;
            numSecondaries = state.numSecondaries;
            numClusters = state.numClusters;

            // compute X/Y stats using positionTree
            std::vector<float> xVals;
            std::vector<float> yVals;

            auto& posIndex = posIndexMap[planeID];

            while (posIndex < nPositionEntries)
            {
                positionTree->GetEntry(posIndex);
                //std::cout << posTiming << std::endl;
                if (posPlaneID != planeID) 
                {
                    posIndex++;
                    continue;
                }
                if (posTiming < state.timeWindow)
                {
                    xVals.push_back(stripX);
                    yVals.push_back(stripY);
                    posIndex++;
                }
                else break;
            }

            computeStats(xVals, meanX, sigmaX);
            computeStats(yVals, meanY, sigmaY);
            auto& eventID = eventIDMap[planeID];
            eventNum = eventID;
            //std::cout << "planeID: " << planeNum << " ; eventID: " << eventNum << std::endl;
            caloTree->Fill();
            eventID++;

            // Reset
            state.numSecondaries = 0;
            state.numClusters = 0;
            state.pixels.clear();
            state.timeWindow += vetoValue;

            // TODO: 1. First insure things are time ordered.
            // 2. Assign clear correlation between time veto and event ID
            // 3. Ensure synchronization between planes.
        }
    }
    // Go through also the last remaining window for each plane
    for (auto& [pid, state] : planeStates)
    {
        if (state.pixels.empty()) continue;

        while (!state.pixels.empty())
        {
            state.numClusters++;
            std::stack<Pixel> toVisit;
            Pixel start = *state.pixels.begin();
            toVisit.push(start);
            state.pixels.erase(start);

            while (!toVisit.empty())
            {
                Pixel p = toVisit.top();
                toVisit.pop();

                for (int d = 0; d < 4; ++d)
                {
                    Pixel neighbor{p.x + dx[d], p.y + dy[d]};
                    auto it = state.pixels.find(neighbor);
                    if (it != state.pixels.end())
                    {
                        toVisit.push(neighbor);
                        state.pixels.erase(it);
                    }
                }
            }
        }

        planeNum = pid;
        numSecondaries = state.numSecondaries;
        numClusters = state.numClusters;
        std::vector<float> xVals;
        std::vector<float> yVals;

        auto& posIndex = posIndexMap[pid];

        while (posIndex < nPositionEntries)
        {
            positionTree->GetEntry(posIndex);

            if (posPlaneID != pid)
            {
                posIndex++;
                continue;
            }

            if (posTiming < state.timeWindow)
            {
                xVals.push_back(stripX);
                yVals.push_back(stripY);
                posIndex++;
            }
            else break;
        }

        computeStats(xVals, meanX, sigmaX);
        computeStats(yVals, meanY, sigmaY);
        caloTree->Fill();
    }



    outfile->cd();
    auto end = std::chrono::high_resolution_clock::now(); 
    std::chrono::duration<float, std::milli> elapsed = end - start;     

    caloTree->Write();
    outfile->Close();

    std::cout << "############################# Calorimetry stopped after " << elapsed.count() << " ms" << std::endl;

}
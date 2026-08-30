#include <map>
#include <vector>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <string>

void Coincidence()
{   std::string inputFile = "/Users/snip/Documents/MALTA_VLAD_13/malta_simulation/Results_10mev_e_mp_mc/local_0001/analysis_results_MP/analysisThr100.root"; 
    auto file = TFile::Open(inputFile.c_str());
    if (!file || file->IsZombie())
    {
        std::cerr << "Error opening file: " << inputFile << std::endl;
        return;
    }

    TTree* planeZ0 = dynamic_cast<TTree*>(file->Get("analyzedHits_planeZ0"));
    TTree* planeZ1 = dynamic_cast<TTree*>(file->Get("analyzedHits_planeZ1"));

    int planeID, clSize,mcFlag;
    double analysisVertexX, analysisVertexY, analysisTiming, correctedTiming;

    planeZ0->SetBranchAddress("planeID", &planeID);
    planeZ0->SetBranchAddress("analysisVertexX", &analysisVertexX);
    planeZ0->SetBranchAddress("analysisVertexY", &analysisVertexY);
    planeZ0->SetBranchAddress("clSize", &clSize);
    planeZ0->SetBranchAddress("correctedTiming", &correctedTiming);
    planeZ0->SetBranchAddress("mcFlag", &mcFlag);

    planeZ1->SetBranchAddress("planeID", &planeID);
    planeZ1->SetBranchAddress("analysisVertexX", &analysisVertexX);
    planeZ1->SetBranchAddress("analysisVertexY", &analysisVertexY);
    planeZ1->SetBranchAddress("clSize", &clSize);
    planeZ1->SetBranchAddress("correctedTiming", &correctedTiming);
    planeZ1->SetBranchAddress("mcFlag", &mcFlag);

    if (!planeZ0 || !planeZ1)
    {
        std::cerr << "Error: One or both trees not found in the file." << std::endl;
        return;
    }

    struct AnalyzedHit
    {
        double vertexX;
        double vertexY;
        int clSize;
        double timing;
        double correctedTiming;
        int mcFlag;
    };

    std::vector<AnalyzedHit> hitsZ0;
    std::vector<AnalyzedHit> hitsZ1;
    std::vector<AnalyzedHit> coincidences;

    Long64_t nEntriesZ0 = planeZ0->GetEntries();
    Long64_t nEntriesZ1 = planeZ1->GetEntries();

    
    std::cout << "Number of entries in plane Z0: " << nEntriesZ0 << std::endl;
    std::cout << "Number of entries in plane Z1: " << nEntriesZ1 << std::endl;
    for (int i = 0; i < nEntriesZ0; ++i)
    {
        planeZ0->GetEntry(i);
        AnalyzedHit hitZ0;
        hitZ0.vertexX = analysisVertexX;
        hitZ0.vertexY = analysisVertexY;
        hitZ0.clSize = clSize;
        hitZ0.correctedTiming = correctedTiming;
        hitZ0.mcFlag = mcFlag;
        hitsZ0.push_back(hitZ0);
    }


    for (int i = 0; i < nEntriesZ1; ++i)
    {
        planeZ1->GetEntry(i);
        AnalyzedHit hitZ1;
        hitZ1.vertexX = analysisVertexX;
        hitZ1.vertexY = analysisVertexY;
        hitZ1.clSize = clSize;
        hitZ1.correctedTiming = correctedTiming;
        hitZ1.mcFlag = mcFlag;
        hitsZ1.push_back(hitZ1);
    }
     struct VertexKey
     {
        double vx;
        double vy;
        bool operator<(const VertexKey& o) const
        {
            return vx < o.vx || (vx == o.vx && vy < o.vy);
        }
     };

    

    std::sort(hitsZ0.begin(), hitsZ0.end(), [](const AnalyzedHit& a, const AnalyzedHit& b) {
        return a.correctedTiming < b.correctedTiming;
    });

    std::sort(hitsZ1.begin(), hitsZ1.end(), [](const AnalyzedHit& a, const AnalyzedHit& b) {
        return a.correctedTiming < b.correctedTiming;
    });
    
    std::map<VertexKey, const AnalyzedHit*> z1ByVertex;
    for (const auto& hitZ1 : hitsZ1)
    { if (hitZ1.mcFlag !=0 || hitZ1.clSize == 0) continue;
         // skip if hit is from background
        
        VertexKey key{hitZ1.vertexX, hitZ1.vertexY};
        z1ByVertex[key] = &hitZ1;
    }


    // check coincidence using sliding window
    double coincidenceWindow = 2.0; // in ns
    double spatialWindow = 100; // in micrometers
    int coinCount = 0;

    for (const auto& hitZ0 :hitsZ0)
    {
        if (hitZ0.mcFlag != 0 || hitZ0.clSize == 0) continue; 
        auto it = z1ByVertex.find({hitZ0.vertexX, hitZ0.vertexY});
        if (it == z1ByVertex.end()) continue; // no Z1 hit for this track -> skip
        const AnalyzedHit& hitZ1 = *(it->second);
        if (std::abs(hitZ0.correctedTiming - hitZ1.correctedTiming) <= coincidenceWindow)
        {
            // Coincidence found
            coinCount++;
        }
    }
    int nSignalGenerated = 0;
    for (const auto& h : hitsZ0) if (h.mcFlag == 0) nSignalGenerated++;

    if (nSignalGenerated == 0)
    {
        std::cerr << "No signal tracks generated!" << std::endl;
        return;
    }

    double eff = 100. * coinCount / nSignalGenerated;

    std::cout << "Total coincidences found: " << coinCount << std::endl;
    std::cout << "Total hits in plane Z0: " << hitsZ0.size() << std::endl;
    std::cout << "Total hits in plane Z1: " << hitsZ1.size() << std::endl;  
    std::cout << "Coincidence efficiency: " << eff << "%" << std::endl;
}

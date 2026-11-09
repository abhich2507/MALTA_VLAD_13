// verifyPixelBounds.c — Phase-1 geometry validation macro (224x512 / MALTA2).
// Checks the RawPixelHits pixel indices against the expected nPixX x nPixY grid
// and reports the hit distribution over planes/modules.
//
// Usage (from malta_simulation/):
//   root -l -b -q 'plotting_scripts/verifyPixelBounds.c(1, 512, 224, 6)'
#include <climits>
#include <map>
#include <iostream>
#include "TChain.h"
#include "TString.h"
#include "TROOT.h"

void verifyPixelBounds(int run = 1, int nPixX = 512, int nPixY = 224, int numThreads = 6)
{
    gROOT->SetBatch(kTRUE);
    TChain *ch = new TChain("RawPixelHits");
    for (int t = 0; t < numThreads; ++t)
        ch->Add(TString::Format("Results/local_%04d/output0_t%d.root", run, t));

    int iEvent, iPlane, iModule, PixX, PixY;
    ch->SetBranchAddress("iEvent", &iEvent);
    ch->SetBranchAddress("iPlane", &iPlane);
    ch->SetBranchAddress("iModule", &iModule);
    ch->SetBranchAddress("PixX", &PixX);
    ch->SetBranchAddress("PixY", &PixY);

    Long64_t n = ch->GetEntries();
    if (n == 0) { std::cerr << "No entries in RawPixelHits chain" << std::endl; return; }

    Long64_t nNeg = 0, nXover = 0, nYover = 0;
    int minX = INT_MAX, maxX = INT_MIN, minY = INT_MAX, maxY = INT_MIN;
    std::map<int, Long64_t> planeHits, moduleHits, yEdgeHits;
    int maxYedge = 0;

    for (Long64_t i = 0; i < n; ++i)
    {
        ch->GetEntry(i);
        if (PixX < 0 || PixY < 0) nNeg++;
        if (PixX >= nPixX) nXover++;
        if (PixY >= nPixY) nYover++;
        if (PixX < minX) minX = PixX;
        if (PixX > maxX) maxX = PixX;
        if (PixY < minY) minY = PixY;
        if (PixY > maxY) { maxY = PixY; }
        if (PixY == nPixY - 1) yEdgeHits[iPlane]++;
        planeHits[iPlane]++;
        moduleHits[iModule]++;
    }

    bool pass = (nNeg == 0 && nXover == 0 && nYover == 0 && maxX == nPixX - 1 && maxY == nPixY - 1);

    std::cout << "=== RawPixelHits bounds check (expected grid " << nPixX << " x " << nPixY << ") ===" << std::endl;
    std::cout << "entries            : " << n << std::endl;
    std::cout << "PixX range         : [" << minX << ", " << maxX << "]" << std::endl;
    std::cout << "PixY range         : [" << minY << ", " << maxY << "]" << std::endl;
    std::cout << "negative indices   : " << nNeg << std::endl;
    std::cout << "PixX >= " << nPixX << "        : " << nXover << std::endl;
    std::cout << "PixY >= " << nPixY << "        : " << nYover << std::endl;
    std::cout << "=== " << (pass ? "PASS" : "FAIL") << " ===" << std::endl;

    std::cout << "--- hits per plane (iPlane) ---" << std::endl;
    for (auto &kv : planeHits)
        std::cout << "  plane " << kv.first << " : " << kv.second
                  << "  (last-row y=" << nPixY - 1 << " hits: " << yEdgeHits[kv.first] << ")" << std::endl;
    std::cout << "--- hits per module (iModule) ---" << std::endl;
    for (auto &kv : moduleHits)
        std::cout << "  module " << kv.first << " : " << kv.second << std::endl;
}

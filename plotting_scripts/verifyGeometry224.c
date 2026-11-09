// verifyGeometry224.c — Phase-1 geometry validation (T2/T3/T4) for the 224x512 sensor.
// Reconstructs global hit positions from RawPixelHits + geoFile offsets and reports
// per-sensor centre and extent. Pass criteria (tolerance = 1 pixel = 0.0364 mm):
//   X centre ~ xoff*10 mm, X width ~ detectorSizeX*10 mm
//   Y centre ~ 0 mm,       Y width ~ detectorSizeY*10 mm (8.1536 mm for 224 rows)
// Saves a global hit map per plane as PDF/PNG.
//
// Usage (from malta_simulation/):
//   root -l -b -q 'plotting_scripts/verifyGeometry224.c(1, 6)'
#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <map>
#include <vector>
#include "TChain.h"
#include "TH2D.h"
#include "TCanvas.h"
#include "TStyle.h"
#include "TROOT.h"
#include "TString.h"
#include "TPaletteAxis.h"

static std::string Trim(const std::string& s)
{
    size_t b = s.find_first_not_of(" \t\r\n");
    if (b == std::string::npos) return "";
    size_t e = s.find_last_not_of(" \t\r\n");
    return s.substr(b, e - b + 1);
}

void verifyGeometry224(int run = 1, int numThreads = 6)
{
    gROOT->SetBatch(kTRUE);
    gStyle->SetOptStat(0);
    gStyle->SetPalette(kRainBow);

    double pixelSize = 0.0364, detXmm = 18.6368, detYmm = 8.1536;
    {
        std::ifstream fl(TString::Format("Results/local_%04d/flags.cfg", run));
        std::string line;
        while (std::getline(fl, line))
        {
            if (line.compare(0, 9, "pixelSize") == 0)
            {
                size_t eq = line.find('=');
                if (eq != std::string::npos) pixelSize = std::stod(line.substr(eq + 1));
            }
            else if (line.compare(0, 13, "detectorSizeX") == 0)
            {
                size_t eq = line.find('=');
                if (eq != std::string::npos) detXmm = std::stod(line.substr(eq + 1)) * 10.;
            }
            else if (line.compare(0, 13, "detectorSizeY") == 0)
            {
                size_t eq = line.find('=');
                if (eq != std::string::npos) detYmm = std::stod(line.substr(eq + 1)) * 10.;
            }
        }
    }

    // planeID -> {xoff[mm], yoff[mm]} from configs/geometry/geo_X4Y1Z2.csv
    std::map<int, std::pair<double, double>> offs;
    {
        std::ifstream geo("configs/geometry/geo_X4Y1Z2.csv");
        std::string line;
        std::getline(geo, line);  // header
        while (std::getline(geo, line))
        {
            if (line.empty()) continue;
            std::stringstream ss(line);
            std::string cell;
            std::vector<std::string> cols;
            while (std::getline(ss, cell, ',')) cols.push_back(Trim(cell));
            if (cols.size() < 6) continue;
            int x = std::stoi(cols[0]);
            int y = std::stoi(cols[1]);
            int z = std::stoi(cols[2]);
            double xo = std::stod(cols[3]);
            double yo = std::stod(cols[4]);
            offs[z * 10000 + y * 100 + x] = {xo * 10., yo * 10.};
        }
    }

    TChain *ch = new TChain("RawPixelHits");
    for (int t = 0; t < numThreads; ++t)
        ch->Add(TString::Format("Results/local_%04d/output0_t%d.root", run, t));

    int iPlane, PixX, PixY;
    ch->SetBranchAddress("iPlane", &iPlane);
    ch->SetBranchAddress("PixX", &PixX);
    ch->SetBranchAddress("PixY", &PixY);

    TH2D *hPlane0 = new TH2D("hPlane0", ";x [mm];y [mm]", 700, -10, 66, 120, -6, 6);
    TH2D *hPlane1 = new TH2D("hPlane1", ";x [mm];y [mm]", 700, -10, 66, 120, -6, 6);

    std::map<int, Long64_t> nPerSensor;
    std::map<int, double> sumX, sumY, sumX2, sumY2;
    std::map<int, double> minX, maxX, minY, maxY;

    Long64_t n = ch->GetEntries();
    for (Long64_t i = 0; i < n; ++i)
    {
        ch->GetEntry(i);
        auto it = offs.find(iPlane);
        if (it == offs.end()) continue;
        double gx = it->second.first  + (PixX + 0.5) * pixelSize - detXmm / 2.;
        double gy = it->second.second + (PixY + 0.5) * pixelSize - detYmm / 2.;

        (iPlane < 10000 ? hPlane0 : hPlane1)->Fill(gx, gy);

        if (nPerSensor[iPlane] == 0)
        {
            minX[iPlane] = maxX[iPlane] = gx;
            minY[iPlane] = maxY[iPlane] = gy;
        }
        nPerSensor[iPlane]++;
        sumX[iPlane] += gx;
        sumY[iPlane] += gy;
        sumX2[iPlane] += gx * gx;
        sumY2[iPlane] += gy * gy;
        if (gx < minX[iPlane]) minX[iPlane] = gx;
        if (gx > maxX[iPlane]) maxX[iPlane] = gx;
        if (gy < minY[iPlane]) minY[iPlane] = gy;
        if (gy > maxY[iPlane]) maxY[iPlane] = gy;
    }

    bool pass = true;
    std::cout << std::fixed << std::setprecision(4);
    std::cout << "=== Geometry check (pixelSize=" << pixelSize << " mm, detX=" << detXmm
              << " mm, detY=" << detYmm << " mm) ===" << std::endl;
    for (auto &kv : offs)
    {
        int pid = kv.first;
        if (!nPerSensor.count(pid)) { std::cout << "plane " << pid << ": NO HITS -> FAIL" << std::endl; pass = false; continue; }
        double meanX = sumX[pid] / nPerSensor[pid];
        double meanY = sumY[pid] / nPerSensor[pid];
        double rmsX = std::sqrt(sumX2[pid] / nPerSensor[pid] - meanX * meanX);
        double rmsY = std::sqrt(sumY2[pid] / nPerSensor[pid] - meanY * meanY);
        double expCX = kv.second.first, expCY = kv.second.second;

        bool okX = std::abs(meanX - expCX) < pixelSize && std::abs((maxX[pid] - minX[pid]) - detXmm) < 2 * pixelSize;
        bool okY = std::abs(meanY - expCY) < pixelSize && std::abs((maxY[pid] - minY[pid]) - detYmm) < 2 * pixelSize;
        if (!okX || !okY) pass = false;

        std::cout << "plane " << std::setw(5) << pid
                  << " : n=" << std::setw(6) << nPerSensor[pid]
                  << "  X center " << meanX << " (exp " << expCX << ")  span [" << minX[pid] << ", " << maxX[pid]
                  << "] mm  w=" << maxX[pid] - minX[pid]
                  << " | Y center " << meanY << " (exp " << expCY << ")  span [" << minY[pid] << ", " << maxY[pid]
                  << "] mm  w=" << maxY[pid] - minY[pid]
                  << "  -> " << (okX && okY ? "OK" : "MISALIGNED") << std::endl;
    }
    std::cout << "=== " << (pass ? "GEOMETRY PASS" : "GEOMETRY FAIL") << " ===" << std::endl;

    for (int p = 0; p < 2; ++p)
    {
        TCanvas *c = new TCanvas(TString::Format("cg%d", p), "geom", 1000, 320);
        TH2D *h = p == 0 ? hPlane0 : hPlane1;
        h->Draw("COLZ");
        h->GetZaxis()->SetTitle("hits");
        c->SaveAs(TString::Format("Results/local_%04d/geom_224_plane%d.png", run, p));
        c->SaveAs(TString::Format("Results/local_%04d/geom_224_plane%d.pdf", run, p));
    }
    std::cout << "Saved geom_224_plane0/1 png+pdf in Results/local_%04d/" << std::endl;
}

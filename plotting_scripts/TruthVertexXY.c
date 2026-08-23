// plotting_scripts/TruthVertexXY.c
// Two side-by-side 2D histograms of TruthVertex positions, binned in actual
// MALTA2 pixel coordinates: 4 chips side-by-side (2048 columns) x 512 rows,
// 1 bin = 1 pixel (36.4 um). Left pad = background (mcFlag=1), right pad = signal (mcFlag=0).
// Pixel size and beam offsets are read from Results/local_NNNN/flags.cfg.
// Usage: root -l -b -q 'plotting_scripts/TruthVertexXY.c(1, 6)'

#include "TChain.h"
#include "TCanvas.h"
#include "TH2D.h"
#include "TStyle.h"
#include "TLine.h"
#include "TGaxis.h"
#include "TPaletteAxis.h"
#include "TString.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <algorithm> // Added for std::max / std::min

// Read a numeric value for a given key from the dumped sim flags file
static double ReadFlagValue(const char* filePath, const char* key, double fallback)
{
    std::ifstream in(filePath);
    if (!in) return fallback;
    std::string line;
    while (std::getline(in, line))
    {
        if (line.empty() || line[0] == '#') continue;
        std::stringstream ss(line);
        std::string k;
        std::getline(ss, k, '=');
        while (!k.empty() && (k.back() == ' ' || k.back() == '\t')) k.pop_back();
        if (k == key)
        {
            std::string v;
            std::getline(ss, v);
            try { return std::stod(v); } catch (...) { return fallback; }
        }
    }
    return fallback;
}

void TruthVertexXY(int runNumber = 1, int numThreads = 6)
{
    gStyle->SetOptStat(0);
    gStyle->SetPalette(kRainBow);
    
    // Force scientific notation for axes if a number has more than 2 digits
    TGaxis::SetMaxDigits(2); 

    // Geometry from the dumped sim flags (fallbacks match flags_MP_EIC.cfg)
    TString flagsPath = Form("./Results/local_%04d/flags.cfg", runNumber);
    double pixelSize   = ReadFlagValue(flagsPath.Data(), "pixelSize", 0.0364);     // mm
    double beamXOffset = ReadFlagValue(flagsPath.Data(), "beamXOffset", 2.79552);  // cm
    double beamYOffset = ReadFlagValue(flagsPath.Data(), "beamYOffset", 0.0);      // cm

    const int nChipsX   = 4;     // 4 chips side-by-side in X
    const int pixPerChip = 512;  // MALTA2: 512 columns x 512 rows
    const int nBinsX = nChipsX * pixPerChip;   // 2048
    const int nBinsY = pixPerChip;             // 512

    // Pixel-exact range centred on the beam (1 bin = 1 pixel)
    double xLow  = beamXOffset * 10.0 - (nBinsX / 2.0) * pixelSize;  // mm
    double xHigh = beamXOffset * 10.0 + (nBinsX / 2.0) * pixelSize;
    double yLow  = beamYOffset * 10.0 - (nBinsY / 2.0) * pixelSize;
    double yHigh = beamYOffset * 10.0 + (nBinsY / 2.0) * pixelSize;

    // Load TruthVertex
    TString inputPath = Form("./Results/local_%04d/", runNumber);
    std::cout << "Loading TruthVertex from: " << inputPath << std::endl;
    TChain *chain = new TChain("TruthVertex");
    for (int t = 0; t < numThreads; ++t)
        chain->Add(Form("%soutput0_t%d.root", inputPath.Data(), t));

    float vertexX = 0.f, vertexY = 0.f;
    int mcFlag = -1;
    chain->SetBranchAddress("trueVertexX", &vertexX);
    chain->SetBranchAddress("trueVertexY", &vertexY);
    chain->SetBranchAddress("mcFlag", &mcFlag);

    // Fixed axis titles
    TH2D *h2Sig = new TH2D("h2Sig", ";Pixels Column;Pixels Row",
                           nBinsX, 0, nBinsX, nBinsY, 0, nBinsY);
    TH2D *h2Bkg = new TH2D("h2Bkg", ";Pixels Column;Pixels Row",
                           nBinsX, 0, nBinsX, nBinsY, 0, nBinsY);

    // Make the primary axes exactly match the size and visibility of the secondary mm axes
    auto formatPrimaryAxes = [](TH2D* h) {
        h->GetXaxis()->SetTitleSize(0.045);
        h->GetXaxis()->SetLabelSize(0.032);
        h->GetXaxis()->SetTitleOffset(1.2); 
        
        h->GetYaxis()->SetTitleSize(0.045);
        h->GetYaxis()->SetLabelSize(0.032);
        h->GetYaxis()->SetTitleOffset(1.2);
    };
    
    formatPrimaryAxes(h2Sig);
    formatPrimaryAxes(h2Bkg);

    long nSig = 0, nBkg = 0;
    for (Long64_t i = 0; i < chain->GetEntries(); ++i)
    {
        chain->GetEntry(i);
        double px = (vertexX - xLow) / pixelSize;  // pixel index in [0, 2048)
        double py = (vertexY - yLow) / pixelSize;  // pixel index in [0, 512)
        if (mcFlag == 0) { h2Sig->Fill(px, py); ++nSig; }
        else             { h2Bkg->Fill(px, py); ++nBkg; }
    }
    std::cout << "Run " << runNumber << ": signal vertices = " << nSig
              << ", background vertices = " << nBkg << std::endl;

    // --- NEW: Force a common Z-axis scale for both color palettes ---
    double maxZ = std::max(h2Bkg->GetMaximum(), h2Sig->GetMaximum());
    double minZ = std::min(h2Bkg->GetMinimum(), h2Sig->GetMinimum());

    h2Bkg->SetMinimum(minZ);
    h2Bkg->SetMaximum(maxZ);
    h2Sig->SetMinimum(minZ);
    h2Sig->SetMaximum(maxZ);
    // ----------------------------------------------------------------

    // Set histogram titles natively so they appear in the top margin
    h2Bkg->SetTitle(Form("Background (mcFlag=1), %ld", nBkg));
    h2Sig->SetTitle(Form("Signal (mcFlag=0), %ld", nSig));

    // Two pads on one canvas: background left, signal right
    TCanvas *c = new TCanvas("cTruthXY", "Truth vertex pixels", 1600, 650);
    c->Divide(2, 1);
    
    // Adjust margins
    for (int p = 1; p <= 2; ++p)
    {
        c->cd(p);
        gPad->SetLeftMargin(0.12);
        gPad->SetRightMargin(0.28); 
        gPad->SetBottomMargin(0.14); 
        gPad->SetTopMargin(0.18);
    }

    // Chip boundary lines (dashed) at x = 512, 1024, 1536
    auto drawChipBorders = [&]() {
        for (int x = 1; x < nChipsX; ++x)
        {
            TLine *ln = new TLine(x * pixPerChip, 0, x * pixPerChip, nBinsY);
            ln->SetLineColor(kGray + 2);
            ln->SetLineStyle(2);
            ln->Draw();
        }
    };

    // Secondary axes with the global scale in mm.
    auto drawMMAxes = [&]() {
        TGaxis *axTop = new TGaxis(gPad->GetUxmin(), gPad->GetUymax(),
                                   gPad->GetUxmax(), gPad->GetUymax(),
                                   xLow, xHigh, 510, "-");
        axTop->SetTitle("X (mm)");
        axTop->CenterTitle(true);
        axTop->SetTitleOffset(1.3); 
        axTop->SetTitleSize(0.045);
        axTop->SetLabelSize(0.032);
        axTop->SetLabelOffset(0.010);
        axTop->SetTextSize(0.032);
        axTop->Draw();

        TGaxis *axRight = new TGaxis(gPad->GetUxmax(), gPad->GetUymin(),
                                     gPad->GetUxmax(), gPad->GetUymax(),
                                     yLow, yHigh, 510, "+");
        axRight->SetTitle("Y (mm)");
        axRight->CenterTitle(true);
        axRight->SetTitleOffset(0.8);
        axRight->SetTitleSize(0.045);
        axRight->SetLabelSize(0.032);
        axRight->SetLabelOffset(0.018);
        axRight->SetTextSize(0.032);
        axRight->Draw();
    };

    // Push the palette into the expanded right margin
    auto movePalette = [&](TH2D* h) {
        gPad->Update();
        TPaletteAxis *palette = (TPaletteAxis*) h->GetListOfFunctions()->FindObject("palette");
        if (palette)
        {
            palette->SetX1NDC(0.86); 
            palette->SetX2NDC(0.91); 
            palette->SetY1NDC(gPad->GetBottomMargin());
            palette->SetY2NDC(1.0 - gPad->GetTopMargin());
            gPad->Modified();
            gPad->Update();
        }
    };

    c->cd(1);
    h2Bkg->Draw("COLZ");
    drawChipBorders();
    movePalette(h2Bkg); 
    drawMMAxes();

    c->cd(2);
    h2Sig->Draw("COLZ");
    drawChipBorders();
    movePalette(h2Sig);
    drawMMAxes();

    c->SaveAs(Form("Plots/truthVertexXY_mcFlag_run%04d.png", runNumber));
    c->SaveAs(Form("Plots/truthVertexXY_mcFlag_run%04d.pdf", runNumber));
    std::cout << "Saved Plots/truthVertexXY_mcFlag_run" << Form("%04d", runNumber) << ".png/pdf" << std::endl;
}
// Calorimetry2DTable.C
// Creates a 2D color-encoded table where:
//   - X axis = runFiles1 (e.g. different geometries / first parameter)
//   - Y axis = runFiles2 (e.g. different thresholds / second parameter)
//   - Cell color = Gaussian mean of "numSecondaries" from the fit
 
// TODO: Claude Generated. Please clean up
#include <iostream>
#include <TH2D.h>
#include <TFile.h>
#include <TTree.h>
#include <TH1D.h>
#include <TF1.h>
#include <TCanvas.h>
#include <TStyle.h>
#include <TROOT.h>
#include <TLatex.h>
#include <TExec.h>
#include <algorithm>
#include <vector>
#include <string>
#include <limits>
 
// ---------------------------------------------------------------------------
// Helper: open a CalorimetryThr*.root file, fit numSecondaries, return mean
// Returns -1 on failure.
// ---------------------------------------------------------------------------
double FitMean(const std::string &filePath)
{
    TFile *f = TFile::Open(filePath.c_str(), "READ");
    if (!f || f->IsZombie()) {
        std::cerr << "Could not open: " << filePath << std::endl;
        return -1.0;
    }
 
    TTree *tree = (TTree*) f->Get("CaloHits");
    if (!tree) {
        std::cerr << "No CaloHits tree in: " << filePath << std::endl;
        f->Close();
        return -1.0;
    }
 
    int numSecondaries = 0;
    tree->SetBranchAddress("numSecondaries", &numSecondaries);
 
    Long64_t n = tree->GetEntries();
    std::vector<double> vals;
    vals.reserve(n);
    for (Long64_t i = 0; i < n; i++) {
        tree->GetEntry(i);
        vals.push_back(numSecondaries);
    }
    f->Close();
 
    if (vals.empty()) return -1.0;
 
    double mn = *std::min_element(vals.begin(), vals.end());
    double mx = *std::max_element(vals.begin(), vals.end());
    if (mx <= mn) mx = mn + 1.0;
 
    TH1D *h = new TH1D("hTmp", "", 50, mn, mx);
    for (double v : vals) h->Fill(v);
 
    TF1 *gaus = new TF1("gausTmp", "gaus", mn, mx);
    h->Fit(gaus, "RQ"); // Q = quiet
 
    double mean = gaus->GetParameter(1);
    std::cout << "  -> mean = " << mean
              << " ± " << gaus->GetParError(1) << std::endl;
 
    delete h;
    delete gaus;
    return mean;
}
 
// ---------------------------------------------------------------------------
void Calorimetry_ParamScan()
{
    gStyle->SetCanvasPreferGL(kTRUE);
    gROOT->SetStyle("ATLAS");
 
    // -----------------------------------------------------------------------
    // USER CONFIGURATION
    // -----------------------------------------------------------------------
 
    // Single run number (each file contains exactly one energy point)
    // If your files are spread across multiple run numbers, extend
    // runNumbers1 / runNumbers2 to match the lengths of runFiles1 / runFiles2.
    const int runNumber = 212;   // <-- change as needed
    const int thr       = 200;   // <-- threshold
    const double expectedSecondaries = 4060;


    // --- Axis 1 (X axis): first parameter being varied ---
    //std::vector<std::string> runFiles1 = {"HWC_SecSize2", "HWC_SecSize3", "HWC_SecSize4", "HWC_SecSize5", "HWC_SecSize6", "HWC_SecSize7"};
    //std::vector<std::string> labels1 = {"Sec Size=2", "Sec Size=3", "Sec Size=4", "Sec Size=5", "Sec Size=6", "Sec Size=7"};

    //std::vector<std::string> runFiles1 = {"HWC_SecSize3_FIFOf6.25FIFOs128", "HWC_SecSize3_FIFOf12.5FIFOs256", "HWC_SecSize3_FIFOf18.75FIFOs384"};
    //std::vector<std::string> labels1 = {"1x128bFIFO", "2x128bFIFO", "3x128bFIFO"};
    //std::vector<std::string> runFiles1 = {"HWC_SecSize4_FIFOf6.25FIFOs128", "HWC_SecSize4_FIFOf12.5FIFOs256", "HWC_SecSize4_FIFOf18.75FIFOs384"};
    //std::vector<std::string> labels1 = {"1x128bFIFO", "2x128bFIFO", "3x128bFIFO"};
    //std::vector<std::string> runFiles1 = {"HWCFIXED_SectorSize7", "HWCFIXED_SectorSize11", "HWCFIXED_SectorSize18"};
    //std::vector<std::string> labels1 = {"SectorSize=7", "SectorSize=11", "SectorSize=18"};
    // Optimized
    //std::vector<std::string> runFiles1 = {"HWCFIXED_SectorSize18_SC2_SRAMd4_ALGORORO", "HWCFIXED_SectorSize18_SC2_SRAMd4_ALGOMOSTFILLED", "HWCFIXED_SectorSize18_SC2_SRAMd4_ALGOMOSTFULL"};
    //std::vector<std::string> labels1 = {"Round Robin", "Most Filled", "Most Full"};
    std::vector<std::string> runFiles1 = {"HWCFIXED_SectorSize18_SC2_SRAMd4", "HWCFIXED_SectorSize18_SC2_SRAMd8", "HWCFIXED_SectorSize18_SC2_SRAMd12"};
    std::vector<std::string> labels1 = {"SRAMd=4", "SRAMd=8", "SRAMd=12"};
 
    // --- Axis 2 (Y axis): second parameter being varied ---
    //std::vector<std::string> runFiles2 = {"SRAMf1.5", "SRAMf3.125", "SRAMf6.25"};
    //std::vector<std::string> labels2 = {"SRAMf=1.5", "SRAMf=3.125", "SRAMf=6.25"};
    //std::vector<std::string> runFiles2 = {"SRAMd4", "SRAMd8", "SRAMd12", "SRAMd16", "SRAMd20", "SRAMd24"};
    //std::vector<std::string> labels2 = {"SRAMd=4", "SRAMd=8", "SRAMd=12", "SRAMd=16", "SRAMd=20", "SRAMd=24"};
    //std::vector<std::string> runFiles2 = {"ALGORORO", "MOSTFILLED", "MOSTFULL"};
    //std::vector<std::string> labels2 = {"Round Robin", "Most Filled", "Most Full"};
    //std::vector<std::string> runFiles2 = {"SRAMd4", "SRAMd8", "SRAMd12", "SRAMd16", "SRAMd20", "SRAMd24"};
    //std::vector<std::string> labels2 = {"SRAMd=4", "SRAMd=8", "SRAMd=12", "SRAMd=16", "SRAMd=20", "SRAMd=24"};
    //std::vector<std::string> runFiles2 = {"FIFOf6.25FIFOs128", "FIFOf12.5FIFOs256", "FIFOf18.75FIFOs384", "FIFOf25FIFOs512"};
    //std::vector<std::string> labels2 = {"1x128W FIFO", "2x128W FIFO", "3x128W FIFO", "4x128W FIFO"};
    //std::vector<std::string> runFiles2 = {"FIFOs128", "FIFOs256", "FIFOs384", "FIFOs512"};
    //std::vector<std::string> labels2 = {"FIFOs=128W", "FIFOs=246W", "FIFOs=384W", "FIFOs=512W"};
    //std::vector<std::string> runFiles2 = {"SCDelay0.5", "SCDelay1", "SCDelay1.5", "SCDelay2"};
    //std::vector<std::string> labels2 = {"SlowControl Delay=0.5ns", "SlowControl Delay=1ns", "SlowControl Delay=1.5ns", "SlowControl Delay=2ns"};
    //std::vector<std::string> runFiles2 = {"BusMerg0.052", "BusMerg0.105", "BusMerg0.21"};
    //std::vector<std::string> labels2 = {"BusThr=0.052", "BusThr=0.105", "BusThr=0.21"};
    //Optimized
    //std::vector<std::string> runFiles2 = {"BusMerg0.105", "BusMerg0.21"};
    //std::vector<std::string> labels2 = {"BusThr=0.105", "BusThr=0.21"};
    std::vector<std::string> runFiles2 = {"ALGORORO_BusMerg0.105", "ALGOMOSTFILLED_BusMerg0.105", "ALGOMOSTFULL_BusMerg0.105"};
    std::vector<std::string> labels2 = {"Round Robin", "Most Filled", "Most Full"};
 
 
    // Optional: per-entry run numbers (must match length of runFiles1/2,
    // or leave as single value above and the code uses runNumber for all).
    // std::vector<int> runNumbers1 = {200,201,202,203,204,205};
    // std::vector<int> runNumbers2 = {200,201,202,203};
 
    // -----------------------------------------------------------------------
    // END USER CONFIGURATION
    // -----------------------------------------------------------------------
 
    const int nx = runFiles1.size();
    const int ny = runFiles2.size();
 
    // Build the 2D histogram with string-labelled bins
    TH2D *h2 = new TH2D("h2ColorTable",
                         ";X parameter;Y parameter",
                         nx, 0, nx,
                         ny, 0, ny);
 
    // Label the bins
    for (int ix = 0; ix < nx; ix++)
        h2->GetXaxis()->SetBinLabel(ix + 1, labels1[ix].c_str());
    for (int iy = 0; iy < ny; iy++)
        h2->GetYaxis()->SetBinLabel(iy + 1, labels2[iy].c_str());
 
    // Fill the 2D histogram
    for (int ix = 0; ix < nx; ix++) {
        for (int iy = 0; iy < ny; iy++) {
 
            // Adjust the path template to your directory structure
            std::string path = Form(
                "Results/local_%04d/%s_%s/CalorimetryThr%i.root",
                runNumber,
                runFiles1[ix].c_str(),
                runFiles2[iy].c_str(),
                thr
            );
 
            // --- ALTERNATIVE path templates (uncomment the one that fits): ---
            // One run number per X entry, fixed Y:
            // std::string path = Form("Results/local_%04d/%s/CalorimetryThr%i.root",
            //                         runNumbers1[ix], runFiles2[iy].c_str(), thr);
            //
            // Flat structure with combined tag:
            // std::string path = Form("Results/%s_%s/CalorimetryThr%i.root",
            //                         runFiles1[ix].c_str(), runFiles2[iy].c_str(), thr);
 
            std::cout << "Processing [" << ix << "," << iy << "]: "
                      << path << std::endl;
 
            double mean = FitMean(path);
            if (mean > 0)
                h2->SetBinContent(ix + 1, iy + 1, (mean / expectedSecondaries * 100 ));
            else
                h2->SetBinContent(ix + 1, iy + 1, 0); // mark failed cells as 0
        }
    }
 
    // -----------------------------------------------------------------------
    // Drawing
    // -----------------------------------------------------------------------
    TCanvas *c1 = new TCanvas("c1", "2D Color Table", 1300, 700);
    c1->SetLeftMargin(0.18);
    c1->SetBottomMargin(0.15);
    c1->SetRightMargin(0.18);
    c1->SetTopMargin(0.12);
 
    // Color palette: use kBird (cool-warm) or kTemperatureMap, kRainbow, etc.
    gStyle->SetPalette(kBird);
    gStyle->SetNumberContours(99);
 
    h2->SetStats(0);
    h2->GetXaxis()->SetLabelSize(0.045);
    h2->GetYaxis()->SetLabelSize(0.045);
    h2->GetZaxis()->SetTitle("Hit Survival Rate [%]");
    h2->GetZaxis()->SetTitleSize(0.04);
    h2->GetZaxis()->SetTitleOffset(1.4);
    h2->SetTitle("Mean Hits per Event;X parameter;Y parameter");
 
    h2->Draw("COLZ");
 
    // Overlay the numerical values in each cell
    TLatex *tex = new TLatex();
    tex->SetTextAlign(22);   // centred
    tex->SetTextSize(0.038);
    tex->SetTextColor(kWhite);
 
    double zmax = h2->GetMaximum();
    double zmid = zmax * 0.55; // switch to black text for bright cells
 
    for (int ix = 1; ix <= nx; ix++) {
        for (int iy = 1; iy <= ny; iy++) {
            double val = h2->GetBinContent(ix, iy);
            if (val <= 0) continue;
            // Use white text on dark cells, black on bright cells
            tex->SetTextColor(val > zmid ? kBlack : kWhite);
            tex->DrawLatex(h2->GetXaxis()->GetBinCenter(ix),
                           h2->GetYaxis()->GetBinCenter(iy),
                           Form("%.2f", val));
        }
    }
 
    // ATLAS label
    TLatex *tl = new TLatex();
    tl->SetNDC();
    tl->SetTextSize(0.042);
    //tl->DrawLatex(0.36, 0.91, "#bf{MALTA2 Simulation}, 30#mum EPI");
    tl->DrawLatex(0.2, 0.91, "#bf{MALTA2 Simulation, Bus thr=0.105, Sector Size=18}, 30#mum EPI");
 
    c1->SaveAs("PublicPlots/Calo2DColorTable.pdf");
    c1->SaveAs("PublicPlots/Calo2DColorTable.png");
 
    std::cout << "\nSaved: PublicPlots/Calo2DColorTable.pdf" << std::endl;
}
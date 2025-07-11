#include <TFile.h>
#include <TH2D.h>
#include <iostream>
#include <fstream>
#include <string>

int ExportTH2DToArray(const std::string& inputFileName, const std::string& outputFileName, const std::string& histName) {
    // Open the ROOT file
    TFile *file = TFile::Open(inputFileName.c_str());
    if (!file || file->IsZombie()) {
        std::cerr << "Error: Cannot open file." << std::endl;
        return 1;
    }

    // Get the histogram
    TH2D *hist = dynamic_cast<TH2D*>(file->Get(histName.c_str()));
    if (!hist) {
        std::cerr << "Error: Histogram not found." << std::endl;
        return 1;
    }

    int nBinsX = hist->GetNbinsX();
    int nBinsY = hist->GetNbinsY();
    double maxContent = hist->GetMaximum();

    // Get bin widths (assuming uniform binning)
    double xSpacing = hist->GetXaxis()->GetBinWidth(1); // spacing in X
    double ySpacing = hist->GetYaxis()->GetBinWidth(1); // spacing in Y
    std::cout << "Xspacing: " << xSpacing << std::endl;
    std::cout << "Yspacing: " << xSpacing << std::endl;

    if (maxContent <= 0.0) {
    std::cerr << "Error: Maximum bin content is zero or negative. Cannot normalize histogram." << std::endl;
    file->Close();
    return 1;
    }

    // Open output file
    std::ofstream outFile(outputFileName);
    outFile << "#include \"CorrectionData2D.hh\"\n\n";
    outFile << "const int nBinsX = " << nBinsX << ";\n";
    outFile << "const int nBinsY = " << nBinsY << ";\n";
    outFile << "const double spacingX = " << xSpacing << ";\n";
    outFile << "const double spacingY = " << ySpacing << ";\n";
    outFile << "double EffMap2D[nBinsX][nBinsY] = {\n";

    for (int i = 1; i <= nBinsX; ++i) {
        outFile << "    {";
        for (int j = 1; j <= nBinsY; ++j) {
            double scaledVal = hist->GetBinContent(i, j) / maxContent;
            outFile << scaledVal;
            if (j < nBinsY) outFile << ", ";
        }
        outFile << "}";
        if (i < nBinsX) outFile << ",";
        outFile << "\n";
    }

    outFile << "};\n";

    // Clean up
    outFile.close();
    file->Close();

    std::cout << "Histogram data written to "<< outputFileName << std::endl;
    return 0;
}


// Main function where you can set parameters
int main() {
    std::string inputFile = "250710_MPV_EPI_data/W5R23IBIAS03SUB06/TOT_PASSRes2/W5R23IBIAS03SUB06_2D_MPV_Res2_TOT_PASS_LandauCDF2Eff.root";
    std::string outputFile = "CorrectionData2D_EPI.cc";
    std::string histName = "MPV_2D";

    ExportTH2DToArray(inputFile, outputFile, histName);

    return 0;
}

main();

// Run this in root via ".L Create2DEffMap.cc"
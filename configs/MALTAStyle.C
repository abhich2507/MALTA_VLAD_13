
// Usage (from malta_simulation/), in any ROOT macro:
//
//     gROOT->LoadMacro("configs/MALTAStyle.C");
//     MALTAStyle();        // global gStyle only
//     MALTAStyle(c1);      // global gStyle + apply the pad margins to c1
//
// or from the command line:
//
//     root -l -b -q 'gROOT->LoadMacro("configs/MALTAStyle.C"); MALTAStyle(); <your macro>'
// ---------------------------------------------------------------------------

#include "TStyle.h"
#include "TROOT.h"
#include "TCanvas.h"

void MALTAStyle(TCanvas* c = nullptr)
{
    // Base style
    gROOT->SetStyle("ATLAS");

    // Canvas / pad defaults
    gStyle->SetCanvasPreferGL(kTRUE);
    gStyle->SetPadLeftMargin(0.17);
    gStyle->SetPadRightMargin(0.05);
    gStyle->SetPadTopMargin(0.10);
    gStyle->SetPadBottomMargin(0.12);

    // No statistics / fit box on histograms
    gStyle->SetOptStat(0);
    gStyle->SetOptFit(0);

    // Legend defaults: no border, no fill, moderate text
    gStyle->SetLegendBorderSize(0);
    gStyle->SetLegendTextSize(0.04);

    // Marker / line defaults (signal-efficiency plot convention)
    gStyle->SetMarkerStyle(20);
    gStyle->SetMarkerSize(2.0);
    gStyle->SetLineWidth(3);

    // Axis titles: standard offset for X, generous offset for Y
    gStyle->SetTitleOffset(1.2, "X");
    gStyle->SetTitleOffset(1.7, "Y");
    gStyle->SetLabelSize(0.04, "XYZ");
    gStyle->SetTitleSize(0.05, "XYZ");

    // Helvetica bold for text / TLatex titles
    gStyle->SetTextFont(42);
    gStyle->SetTextSize(0.04);

    // Histogram line width
    gStyle->SetHistLineWidth(3);

    // Apply the same margins to a specific canvas if provided
    if (c)
    {
        c->SetLeftMargin(gStyle->GetPadLeftMargin());
        c->SetRightMargin(gStyle->GetPadRightMargin());
        c->SetTopMargin(gStyle->GetPadTopMargin());
        c->SetBottomMargin(gStyle->GetPadBottomMargin());
    }
}

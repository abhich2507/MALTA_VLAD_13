#include <iostream>
#include <ROOT/RNTuple.hxx>
#include <TH3D.h>
#include <TFile.h>
#include <cmath>

void NegPosIntegral(TH1D *h1)
{
    double integralPositive = 0.;
    double integralNegative = 0.;

    int nbins = h1->GetNbinsX();
    for (int i = 1; i <= nbins; i++)
    {
        double c = h1->GetBinContent(i);
        if (c > 0)
            integralPositive += c;
        else
            integralNegative += c;
    }
    std::cout << "Positive: " << integralPositive << "; Negative: " << integralNegative << std::endl;
}


void BeamSpot()
{
    gStyle->SetCanvasPreferGL(kTRUE);
    gROOT->SetStyle("ATLAS");
    std::string pathMerger = "Plots/local_0047/Merg/histos.root";

    TFile *fileMerger = TFile::Open(pathMerger.c_str(), "READ");
    if (!fileMerger || fileMerger->IsZombie()) {
        std::cerr << "Could not open file: " << pathMerger << std::endl;
        return;
    }
    TDirectory *dirMerger = (TDirectory*)fileMerger->Get("Thr200");
    TH2D *h2DUTHitsMerger = (TH2D*) dirMerger->Get("h2DUTHits");


    std::string pathNoMerger = "Plots/local_0047/NoMerg/histos.root";

    TFile *fileNoMerger = TFile::Open(pathNoMerger.c_str(), "READ");
    if (!fileNoMerger || fileNoMerger->IsZombie()) {
        std::cerr << "Could not open file: " << pathNoMerger << std::endl;
        return;
    }
    TDirectory *dirNoMerger = (TDirectory*)fileNoMerger->Get("Thr200");
    TH2D *h2DUTHitsNoMerger = (TH2D*) dirNoMerger->Get("h2DUTHits");

    TCanvas *c1 = new TCanvas("c1","2D DUT Hits Merger ON",800,800);
    TCanvas *c2 = new TCanvas("c2","2D DUT Hits Merger OFF",800,800);
    TCanvas *c3 = new TCanvas("c3","2D Only Merged Hits",800,800);
    TCanvas *c4 = new TCanvas("c4","X Only Merged Hits",800,800);
    TCanvas *c5 = new TCanvas("c5","Y Only Merged Hits",800,800);


    h2DUTHitsMerger->Rebin2D(2, 2);
    h2DUTHitsMerger->GetYaxis()->SetRangeUser(0, 512);
    h2DUTHitsMerger->SetMinimum(0);
    h2DUTHitsMerger->SetMaximum(70);

    h2DUTHitsNoMerger->Rebin2D(2, 2);
    h2DUTHitsNoMerger->GetYaxis()->SetRangeUser(0, 512);
    h2DUTHitsNoMerger->SetMinimum(0);
    h2DUTHitsNoMerger->SetMaximum(70);

    TH2D *h2mergedSubtracted = (TH2D*)h2DUTHitsMerger->Clone("h2DUTHitsMergerCopy");
    h2mergedSubtracted->Add(h2DUTHitsNoMerger, -1.0);
    //h2mergedSubtracted->SetMinimum(0);

    TH1D* h1mergedSubtractedXProj = h2mergedSubtracted->ProjectionX();
    //h1mergedSubtractedXProj->SetMinimum(0);
    h1mergedSubtractedXProj->Scale(1.0 / h1mergedSubtractedXProj->GetMaximum());


    NegPosIntegral(h1mergedSubtractedXProj);


    TH1D* h1mergedSubtractedYProj = h2mergedSubtracted->ProjectionY();
    h1mergedSubtractedYProj->Scale(1.0 / h1mergedSubtractedYProj->GetMaximum());

    NegPosIntegral(h1mergedSubtractedYProj);


    c1->SetTopMargin(0.2);
    c1->SetRightMargin(0.2);
    c2->SetTopMargin(0.2);
    c2->SetRightMargin(0.2);
    c3->SetTopMargin(0.2);
    c3->SetRightMargin(0.2);
    c4->SetTopMargin(0.2);
    c4->SetRightMargin(0.2);
    c5->SetTopMargin(0.2);
    c5->SetRightMargin(0.2);

    c1->cd();
    h2DUTHitsMerger->Draw("COLZ");
    h2DUTHitsMerger->SetTitle(";Pix X;Pix Y");
    h2DUTHitsMerger->GetZaxis()->SetTitle("Merging Hits");
    TLatex *t1 = new TLatex();
    t1->SetTextSize(0.05);
    t1->SetNDC();
    t1->DrawLatex(0.27, 0.83, "#bf{MALTA2} 30#mum EPI");

    c2->cd();
    h2DUTHitsNoMerger->Draw("COLZ");
    h2DUTHitsNoMerger->SetTitle(";Pix X;Pix Y");
    h2DUTHitsNoMerger->GetZaxis()->SetTitle("No Merging Hits");
    TLatex *t2 = new TLatex();
    t2->SetTextSize(0.05);
    t2->SetNDC();
    t2->DrawLatex(0.27, 0.83, "#bf{MALTA2} 30#mum EPI");

    c3->cd();
    h2mergedSubtracted->Draw("COLZ");
    h2mergedSubtracted->SetTitle(";Pix X;Pix Y");
    h2mergedSubtracted->GetZaxis()->SetTitle("Merging - No Merging Hits");
    TLatex *t3 = new TLatex();
    t3->SetTextSize(0.05);
    t3->SetNDC();
    t3->DrawLatex(0.27, 0.83, "#bf{MALTA2} 30#mum EPI");

    c4->cd();
    h1mergedSubtractedXProj->Draw("HIST");
    h1mergedSubtractedXProj->SetTitle(";Pix X;Normalized Counts");

    c5->cd();
    h1mergedSubtractedYProj->Draw("HIST");
    h1mergedSubtractedYProj->SetTitle(";Pix Y;Normalized Counts");



    c1->SaveAs("PublicPlots/BeamSpotMerging.pdf");
    c1->SaveAs("PublicPlots/BeamSpotMerging.C");

    c2->SaveAs("PublicPlots/BeamSpotNoMerging.pdf");
    c2->SaveAs("PublicPlots/BeamSpotNoMerging.C");

    c3->SaveAs("PublicPlots/BeamSpotDiffernce.pdf");
    c3->SaveAs("PublicPlots/BeamSpotDifference.C");

    c4->SaveAs("PublicPlots/BeamSpotXProj.pdf");
    c4->SaveAs("PublicPlots/BeamSpotXProj.C");

    c5->SaveAs("PublicPlots/BeamSpotYProj.pdf");
    c5->SaveAs("PublicPlots/BeamSpotYProj.C");




}
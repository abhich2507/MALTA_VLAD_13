#include "DigitalProcessing.hh"
#include <iostream>
#include <ROOT/RNTuple.hxx>
#include <TH3D.h>
#include <TFile.h>
#include <cmath>


void DigitalTest()
{
    gStyle->SetCanvasPreferGL(kTRUE);
    gROOT->SetStyle("ATLAS");
    std::vector<std::pair<int,int>> hits = {{511,511}, {511,511}};
    digitalTest(hits);

    // Test the global position translation of pixel coordinates.
    //PixelPositionReconstruction();


    /*
    // Chech that encoding/ decoding is constistent across all pixels
    bool roundtrip_ok = true;
    for (int x = 0; x< 512; x++)
    {
        for (int y = 0; y< 512; y++)
        {
            auto word = encodeWord(x, y, false);
            auto pixels = decodedDigitalWord(word, 16, 5, 1, 8); 
            bool found = false;
            for (auto &p : pixels) 
            {
                if (p.first.first == x && p.first.second == y) { found = true; break; }
            }
            if (!found) 
            {
                std::cout << "Roundtrip fail for " << x << "," << y << std::endl;
                roundtrip_ok = false;
            }
        }
    }
    std::cout << "Roundtrip ok? " << (roundtrip_ok ? "YES" : "NO") << std::endl;
    */

    // Generate plots of merging
    int y = 100;
    std::vector<int> xMerged;
    std::vector<int> xPos;
    std::vector<int> yMerged;

    TH1D *XProjHisto    = new TH1D("XProjHisto", "XProjHisto", 512,0,512);
    TH1D *YProjHisto    = new TH1D("YProjHisto", "YProjHisto", 512,0,512);

    /*
    for (int x= 0; x< 511; x++)
    {
        //std::cout << "####################################" << std::endl;
        //std::cout << "X: " << x << "; Y: " << y << std::endl;

        auto word = encodeWord(x, y, true);

        //std::cout << std::bitset<30>(word) << std::endl;
        auto nextWord = encodeWord(x+1,y, false);

        //std::cout << std::bitset<30>(nextWord) << std::endl;
        nextWord |= word;

        //std::cout << std::bitset<30>(nextWord) << std::endl;
        //std::cout << "####################################" << std::endl;
        auto mergedHits = decodedDigitalWord(nextWord, 16, 5, 1, 8); 
        for (auto &hit: mergedHits)
        {
            int pixX = hit.first.first;
            int pixY = hit.first.second;

            xPos.push_back(x);
            xMerged.push_back(pixX - x);
            yMerged.push_back(pixY);

            if(pixX - x> 1) XProjHisto->Fill(pixX);

            //std::cout << "pixX: " << pixX << "; pixY: " << pixY << std::endl;  
        }
    }

    int x = 100;
    std::vector<int> yMergedY;
    std::vector<int> yPos;
    std::vector<int> xMergedY;

    for (y= 0; y< 511; y++)
    {
        //std::cout << "####################################" << std::endl;
        //std::cout << "X: " << x << "; Y: " << y << std::endl;

        auto word = encodeWord(x, y, true);

        //std::cout << std::bitset<30>(word) << std::endl;
        auto nextWord = encodeWord(x,y+1, false);

        //std::cout << std::bitset<30>(nextWord) << std::endl;
        nextWord |= word;

        //std::cout << std::bitset<30>(nextWord) << std::endl;
        //std::cout << "####################################" << std::endl;
        auto mergedHits = decodedDigitalWord(nextWord, 16, 5, 1, 8); 
        for (auto &hit: mergedHits)
        {
            int pixX = hit.first.first;
            int pixY = hit.first.second;

            yPos.push_back(y);
            xMergedY.push_back(pixX);
            yMergedY.push_back(pixY - y);
            YProjHisto->Fill(pixY);

            //std::cout << "pixX: " << pixX << "; pixY: " << pixY << std::endl;  
        }
    }


    TCanvas *c1 = new TCanvas("c1","c1",800,800);
    TCanvas *c2 = new TCanvas("c2","c2",800,800);
    TCanvas *c3 = new TCanvas("c3","c3",800,800);
    TCanvas *c4 = new TCanvas("c4","c4",800,800);
    TCanvas *c5 = new TCanvas("c5","c5",800,800);
    TCanvas *c6 = new TCanvas("c6","c6",800,800);

    TLine *diag = new TLine(0, 0, 512, 512);
    diag->SetLineStyle(3); // 3 = dotted (try 2 = dashed)
    diag->SetLineWidth(1);

    TGraph *gPixX = new TGraph(xPos.size(), xPos.data(), xMerged.data());
    gPixX->SetTitle("X Merging by 1; Left side X position; Merged hit X position");
    gPixX->SetLineColor(kBlue);
    gPixX->SetLineWidth(1.);
    TGraph *gPixY = new TGraph(xPos.size(), xPos.data(), yMerged.data());
    gPixY->SetTitle("X Merging by 1; Left side X position; Merged hit Y position");
    TGraph *gPixXY = new TGraph(yPos.size(), yPos.data(), xMergedY.data());
    gPixXY->SetTitle("Y Merging by 1; Bottom side Y position; Merged hit X position");
    TGraph *gPixYY = new TGraph(yPos.size(), yPos.data(), yMergedY.data());
    gPixYY->SetTitle("Neighbor merging; PixY; Merging offset");
    gPixYY->SetLineColor(kRed);
    gPixYY->SetLineWidth(2.);
    //gPixYY->GetXaxis()->SetRangeUser(0, 33);
    //gPixYY->GetYaxis()->SetRangeUser(0, 32);
                
    c1->cd();
    gPixX->Draw();
    //diag->Draw("same");
    c2->cd();
    gPixY->Draw();
    c3->cd();
    gPixXY->Draw();
    c4->cd();
    gPixYY->Draw();
    //diag->Draw("same");
    c5->cd();
    gPixYY->Draw();
    gPixX->Draw("SAME");
    TLegend *leg1 = new TLegend(0.15,0.1,0.5,0.5);
    leg1->SetTextSize(0.06);
    leg1->SetBorderSize(0);
    leg1->AddEntry(gPixX, "X merging");
    leg1->AddEntry(gPixYY, "Y merging");
    leg1->Draw();

    c6->cd();
    //XProjHisto->Rebin(2);
    YProjHisto->Draw("");
    
    int nbins = XProjHisto->GetNbinsX();
    for (int i=1; i<=nbins; i++) {
        double y1 = XProjHisto->GetBinLowEdge(i);
        double y2 = XProjHisto->GetBinLowEdge(i+1);
        double x1 = 0;
        double x2 = XProjHisto->GetBinContent(i);

        // Pick a color depending on value
        int color = kBlack;
        if (x2 > 12) color = kRed;
        else if (x2 > 10) color = kBlack;
        else if (x2 > 8) color = kMagenta;
        else if (x2 > 6) color = kOrange;
        else if (x2 > 4) color = kYellow;
        else if (x2 > 3) color = kBlue;
        else if (x2 > 1) color = kGreen+2;

        // Draw the colored bar
        TBox *b = new TBox(x1, y1, x2, y2);
        b->SetFillColor(color);
        b->SetLineColor(color);
        b->Draw("same");
    }
        

    */

}
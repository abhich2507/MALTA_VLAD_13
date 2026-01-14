#include <iostream>
#include <ROOT/RNTuple.hxx>
#include <TH3D.h>
#include <TFile.h>
#include <cmath>


double Get2DMean(TH2D *h2D)
{
    double sum = 0;
    int nbins = 0;

    std::cout << h2D->GetNbinsX() << " ; " <<h2D->GetNbinsY() << std::endl;

    for (int i = 1; i <= h2D->GetNbinsX(); ++i) 
    {
        for (int j = 1; j <= h2D->GetNbinsY(); ++j) 
        {

            sum += std::abs(h2D->GetBinContent(i, j));
            nbins++;
            //std::cout <<sum << " " << nbins <<std::endl;
            //std::cout << "i: " << i << "; j: " << j << "; Content: " << h2D->GetBinContent(i, j) << std::endl;
        }
    }
    return sum / nbins ;
}

double GetTimeDiff(TH2D *h2D)
{
    double sum = 0;
    int nbins = 0;

    //std::cout << h2D->GetNbinsX() << " ; " <<h2D->GetNbinsY() << std::endl;

    for (int i = 1; i <= h2D->GetNbinsX(); ++i) 
    {
        for (int j = 1; j <= h2D->GetNbinsY(); ++j) 
        {
            if (i>= 7 && i<=11 && j>= 7 && j<=11)
            {
                sum += h2D->GetBinContent(i, j);
                nbins++;
                //std::cout <<sum << " " << nbins <<std::endl;

                
            }
        }   
    }
    double center =  sum / nbins ;
    std::cout << "Center: " << center << std::endl;
    nbins = 0;
    sum = 0;
    for (int i = 1; i <= h2D->GetNbinsX(); ++i) 
    {
        for (int j = 1; j <= h2D->GetNbinsY(); ++j) 
        {
            if (i>= 15 && i<=18 && j>= 15 && j<=18)
            {
                sum += h2D->GetBinContent(i, j);
                nbins++;
                //std::cout <<sum << " " << nbins <<std::endl;
            }
        }   
    }
    double corner =  sum / nbins ;
    std::cout << "Corner: " << corner << std::endl;

    return corner - center;

}

void InPixelClSize()
{
    // Simulation in-pixel plots
    std::string path = "Plots/local_0011/Final/histos.root";
    //std::string path = "Plots/local_0011/offsetX-0.52Y+0.53mu/histos.root";

    TFile *simuThresholdFile = TFile::Open(path.c_str(), "READ");
    if (!simuThresholdFile || simuThresholdFile->IsZombie()) {
        std::cerr << "Could not open file: " << path << std::endl;
        return;
    }
    TDirectory *dir = (TDirectory*)simuThresholdFile->Get("Thr200");
    TH2D *h2PASSInPixel   = (TH2D*) dir->Get("h2PASSInPixel");
    TH2D *h2ClSizeInPixel = (TH2D*) dir->Get("h2ClSizeInPixel");
    TH2D *h2TimingInPixel = (TH2D*) dir->Get("h2TimingInPixel");

    double simuClSizeMean = Get2DMean(h2ClSizeInPixel);
    double simuEffMean = Get2DMean(h2PASSInPixel);
    double simuTimeMean = Get2DMean(h2TimingInPixel) ;
    double simuTimeMin = h2TimingInPixel->GetMinimum();

    for (int i = 1; i <= h2TimingInPixel->GetNbinsX(); ++i) 
    {
        for (int j = 1; j <= h2TimingInPixel->GetNbinsY(); ++j) 
        {
            double old_content = h2TimingInPixel->GetBinContent(i, j);
            h2TimingInPixel->SetBinContent(i, j, old_content - simuTimeMean);
        }
    }

    double simuTimeDiff = GetTimeDiff(h2TimingInPixel);

    // Data in-pixel plots
    //// 200 el thr
    path = "plotting_scripts/root_input/Eff_Clsize_W5R23__IDB120_ITHR015_SUB06.0_PWELL06.root";

    /// 1600 el thr
    //path = "plotting_scripts/root_input/W5R23_IBIAS05_rootdatafiles/Eff_Clsize_W5R23__IDB100_ITHR066_SUB06_IBIAS05_PWELL06.root";

    TFile *dataThresholdFile = TFile::Open(path.c_str(), "READ");
    if (!dataThresholdFile || dataThresholdFile->IsZombie()) {
        std::cerr << "Could not open file: " << path << std::endl;
        return;
    }
    TH2D *h2dataPASSInPixel   = (TH2D*) dataThresholdFile->Get("TOT_Eff");
    TH2D *h2dataClSizeInPixel = (TH2D*) dataThresholdFile->Get("TOT_ClSize");
    TH2D *h2dataTimingInPixel = (TH2D*) dataThresholdFile->Get("TOT_ClTime");
    double dataClSizeMean = Get2DMean(h2dataClSizeInPixel);
    double dataEffMean = Get2DMean(h2dataPASSInPixel);
    double dataTimeMean = Get2DMean(h2dataTimingInPixel);
    double dataTimeMin = h2dataTimingInPixel->GetMinimum();
    std::cout << "Simu mean time: " << simuTimeMean << "; Data mean time: " << dataTimeMean << std::endl;
    std::cout << "Simu mean cl size: " << simuClSizeMean << "; Data mean cl size: " << dataClSizeMean << std::endl;
    std::cout << "Simu mean eff: " << simuEffMean << "; Data mean eff: " << dataEffMean << std::endl;
    std::cout << "Simu time min: " << simuTimeMin << "; Data time min: " << dataTimeMin << std::endl;

    
    for (int i = 1; i <= h2dataTimingInPixel->GetNbinsX(); ++i) 
    {
        for (int j = 1; j <= h2dataTimingInPixel->GetNbinsY(); ++j) 
        {
            double old_content = h2dataTimingInPixel->GetBinContent(i, j);
            h2dataTimingInPixel->SetBinContent(i, j, old_content - dataTimeMean);
        }
    }
    double dataTimeDiff = GetTimeDiff(h2dataTimingInPixel);
    
    gStyle->SetCanvasPreferGL(kTRUE);
    gROOT->SetStyle("ATLAS");


    h2TimingInPixel->GetZaxis()->SetTitle("Cl time - <Cl time> [ns]");
    //h2TimingInPixel->SetMinimum(-2.2);
    //h2TimingInPixel->SetMaximum(3.5);
    h2dataTimingInPixel->GetZaxis()->SetTitle("Cl time - <Cl time> [ns]");
    h2dataTimingInPixel->SetMinimum(-2.2);
    h2dataTimingInPixel->SetMaximum(3.5);

    h2PASSInPixel->SetMinimum(80);
    h2PASSInPixel->SetMaximum(100);
    h2dataPASSInPixel->SetMinimum(80);
    h2dataPASSInPixel->SetMaximum(100);
    h2ClSizeInPixel->SetMinimum(1);
    h2ClSizeInPixel->SetMaximum(2.4);
    h2dataClSizeInPixel->SetMinimum(1);
    h2dataClSizeInPixel->SetMaximum(2.4);

    TH2D *h2dataPASSInPixelResidual = (TH2D*)h2dataPASSInPixel->Clone("h2dataPASSInPixelCopy");
    h2dataPASSInPixelResidual->Add(h2PASSInPixel, -1.0);
    h2dataPASSInPixelResidual->Divide(h2PASSInPixel);
    TH1D *h1dataPASSInPixelResidual = new TH1D("h1dataPASSInPixelResidual", "h1dataPASSInPixelResidual", 40, -5, 5);

    TH2D *h2dataClSizeInPixelResidual = (TH2D*)h2dataClSizeInPixel->Clone("h2dataClSizeInPixelCopy");
    h2dataClSizeInPixelResidual->Add(h2ClSizeInPixel, -1.0);
    h2dataClSizeInPixelResidual->Divide(h2ClSizeInPixel);
    TH1D *h1dataClSizeInPixelResidual = new TH1D("h1dataClSizeInPixelResidual", "h1dataClSizeInPixelResidual", 40, -15, 19);

    TH2D *h2dataTimingInPixelResidual = (TH2D*)h2dataTimingInPixel->Clone("h2dataTimingInPixelCopy");
    h2dataTimingInPixelResidual->Add(h2TimingInPixel, -1.0);
    //h2dataTimingInPixelResidual->Divide(h2TimingInPixel);    

    
    for (int i = 1; i <= h2dataTimingInPixelResidual->GetNbinsX(); ++i) 
    {
        for (int j = 1; j <= h2dataTimingInPixelResidual->GetNbinsY(); ++j) 
        {
            double old_content = h2dataTimingInPixelResidual->GetBinContent(i, j);
            double to_divide = h2TimingInPixel->GetBinContent(i, j);
            h2dataTimingInPixelResidual->SetBinContent(i, j, (old_content /to_divide + 1e-6 )*100);
            //std::cout << "binx: " << i << "; biny: " << j << "old_val: " << old_content << "; to_divide: " << to_divide << "; new_val: " << (old_content /to_divide + 1e-6 )*100 << std::endl;
        }
    }
    
    h2dataTimingInPixelResidual->SetMinimum(-300);
    h2dataTimingInPixelResidual->SetMaximum(300);
    h2dataTimingInPixelResidual->GetZaxis()->SetTitle("Rel diff [%]");
    

    for (int i = 1; i <= h2dataClSizeInPixelResidual->GetNbinsX(); ++i) 
    {
        for (int j = 1; j <= h2dataClSizeInPixelResidual->GetNbinsY(); ++j) 
        {
            double old_content = h2dataClSizeInPixelResidual->GetBinContent(i, j);
            h2dataClSizeInPixelResidual->SetBinContent(i, j, old_content *100);
            h1dataClSizeInPixelResidual->Fill(old_content *100);
        }
    }
    h1dataClSizeInPixelResidual->Scale(1/ h1dataClSizeInPixelResidual->Integral());

    // Attempt simple gaussian fit of 1D Cl Size distribution
    h1dataClSizeInPixelResidual->Fit("gaus");
    TF1 *fit = h1dataClSizeInPixelResidual->GetFunction("gaus");
    double mu1DClSizeResidual = fit->GetParameter(1);
    double sigma1DClSizeResidual = fit->GetParameter(2);
    double muerr1DClSizeResidual = fit->GetParError(1);
    double sigmaerr1DClSizeResidual = fit->GetParError(2);
    fit->SetLineColor(kRed);
    fit->SetLineWidth(4);
    h1dataClSizeInPixelResidual->SetTitle(";Cl. Size Rel diff [%];Normalized Counts");


    TH2D *h2dataClSizeInPixelResidual_rebinned = (TH2D*)h2dataClSizeInPixelResidual->Rebin2D(1, 1);

    h2dataClSizeInPixelResidual_rebinned->SetMinimum(-10);
    h2dataClSizeInPixelResidual_rebinned->SetMaximum(10);
    h2dataClSizeInPixelResidual_rebinned->GetZaxis()->SetTitle("Cl. Size Rel diff [%]");

    for (int i = 1; i <= h2dataPASSInPixelResidual->GetNbinsX(); ++i) 
    {
        for (int j = 1; j <= h2dataPASSInPixelResidual->GetNbinsY(); ++j) 
        {
            double old_content = h2dataPASSInPixelResidual->GetBinContent(i, j);
            h2dataPASSInPixelResidual->SetBinContent(i, j, old_content *100);
            h1dataPASSInPixelResidual->Fill(old_content *100);
        }
    }
    h1dataPASSInPixelResidual->Scale(1/ h1dataPASSInPixelResidual->Integral());
    
    // Attempt simple gaussian fit of 1D Cl Size distribution
    h1dataPASSInPixelResidual->Fit("gaus");
    TF1 *fitEff = h1dataPASSInPixelResidual->GetFunction("gaus");
    double mu1DEffResidual = fitEff->GetParameter(1);
    double sigma1DEffResidual = fitEff->GetParameter(2);
    double muerr1DEffResidual = fitEff->GetParError(1);
    double sigmaerr1DEffResidual = fitEff->GetParError(2);
    fitEff->SetLineColor(kRed);
    fitEff->SetLineWidth(4);
    h1dataPASSInPixelResidual->SetTitle(";Eff. Rel diff [%];Normalized Counts");


    double residualMean = Get2DMean(h2dataClSizeInPixelResidual_rebinned);
    std::cout << "residualMean: " << residualMean << std::endl;
    h2dataPASSInPixelResidual->SetMinimum(-5.5);
    h2dataPASSInPixelResidual->SetMaximum(5.5);
    h2dataPASSInPixelResidual->GetZaxis()->SetTitle("Eff. Rel diff [%]");

    TCanvas *c1 = new TCanvas("c1","SIMU In-pix Eff",800,800);
    c1->SetTopMargin(0.26);    // leave space for title
    c1->SetBottomMargin(0.12); // leave space for X axis labels
    c1->SetLeftMargin(0.2);   // leave space for Y axis labels
    c1->SetRightMargin(0.2);  // keep right side small for square plot
    TCanvas *c2 = new TCanvas("c2","SIMU In-pix ClSize",800,800);
    c2->SetTopMargin(0.26);    // leave space for title
    c2->SetBottomMargin(0.12); // leave space for X axis labels
    c2->SetLeftMargin(0.2);   // leave space for Y axis labels
    c2->SetRightMargin(0.2);  // keep right side small for square plot
    TCanvas *c3 = new TCanvas("c3","SIMU In-pix Timing",800,800);
    c3->SetTopMargin(0.26);    // leave space for title
    c3->SetBottomMargin(0.12); // leave space for X axis labels
    c3->SetLeftMargin(0.2);   // leave space for Y axis labels
    c3->SetRightMargin(0.2);  // keep right side small for square plot
    TCanvas *c4 = new TCanvas("c4","DATA In-pix Eff",800,800);
    c4->SetTopMargin(0.26);    // leave space for title
    c4->SetBottomMargin(0.12); // leave space for X axis labels
    c4->SetLeftMargin(0.2);   // leave space for Y axis labels
    c4->SetRightMargin(0.2);  // keep right side small for square plot
    TCanvas *c5 = new TCanvas("c5","DATA In-pix ClSize",800,800);
    c5->SetTopMargin(0.26);    // leave space for title
    c5->SetBottomMargin(0.12); // leave space for X axis labels
    c5->SetLeftMargin(0.2);   // leave space for Y axis labels
    c5->SetRightMargin(0.2);  // keep right side small for square plot
    TCanvas *c6 = new TCanvas("c6","DATA In-pix Timing",800,800);
    c6->SetTopMargin(0.26);    // leave space for title
    c6->SetBottomMargin(0.12); // leave space for X axis labels
    c6->SetLeftMargin(0.2);   // leave space for Y axis labels
    c6->SetRightMargin(0.2);  // keep right side small for square plot
    TCanvas *c7 = new TCanvas("c7","DATA - SIMU Cl Size",800,800);
    c7->SetTopMargin(0.26);    // leave space for title
    c7->SetBottomMargin(0.12); // leave space for X axis labels
    c7->SetLeftMargin(0.2);   // leave space for Y axis labels
    c7->SetRightMargin(0.2);  // keep right side small for square plot
    TCanvas *c8 = new TCanvas("c8","DATA - SIMU Timing",800,800);
    c8->SetTopMargin(0.10);    // leave space for title
    c8->SetBottomMargin(0.12); // leave space for X axis labels
    c8->SetLeftMargin(0.2);   // leave space for Y axis labels
    c8->SetRightMargin(0.2);  // keep right side small for square plot
    TCanvas *c9 = new TCanvas("c9","DATA - SIMU Eff",800,800);
    c9->SetTopMargin(0.26);    // leave space for title
    c9->SetBottomMargin(0.12); // leave space for X axis labels
    c9->SetLeftMargin(0.2);   // leave space for Y axis labels
    c9->SetRightMargin(0.2);  // keep right side small for square plot
    TCanvas *c10 = new TCanvas("c10","1D ClSizeResidual",800,800);
    c10->SetTopMargin(0.12);    // leave space for title
    c10->SetBottomMargin(0.19); // leave space for X axis labels
    c10->SetLeftMargin(0.18);   // leave space for Y axis labels
    c10->SetRightMargin(0.12);  // keep right side small for square plot
    TCanvas *c11 = new TCanvas("c11","1D EffResidual",800,800);
    c11->SetTopMargin(0.12);    // leave space for title
    c11->SetBottomMargin(0.19); // leave space for X axis labels
    c11->SetLeftMargin(0.18);   // leave space for Y axis labels
    c11->SetRightMargin(0.12);  // keep right side small for square plot

    gStyle->SetNumberContours(255);
    gStyle->SetPalette(kBird);

    double xMin = h2dataPASSInPixelResidual->GetXaxis()->GetXmin();
    double xMax = h2dataPASSInPixelResidual->GetXaxis()->GetXmax();
    double yMin = h2dataPASSInPixelResidual->GetYaxis()->GetXmin();
    double yMax = h2dataPASSInPixelResidual->GetYaxis()->GetXmax();
    double xMid = 0.5*(xMin + xMax);
    double yMid = 0.5*(yMin + yMax);

    c1->cd();
    h2PASSInPixel->Draw("COLZ");

    TLatex *t1 = new TLatex();
    t1->SetTextSize(0.043);
    t1->SetNDC();
    t1->DrawLatex(0.23, 0.78, Form("#splitline{#bf{MALTA2 Simulation}, 30#mum EPI}{             <Eff.> = %.2f %}", simuEffMean));

    // Draw vertical line at xMid
    TLine *lineX1 = new TLine(xMid, yMin, xMid, yMax);
    lineX1->SetLineColor(kBlack);
    lineX1->SetLineWidth(3);
    lineX1->Draw("same");
    // Draw horizontal line at yMid
    TLine *lineY1 = new TLine(xMin, yMid, xMax, yMid);
    lineY1->SetLineColor(kBlack);
    lineY1->SetLineWidth(3);
    lineY1->Draw("same");
    h2PASSInPixel->GetZaxis()->SetTitle("Tracking Efficiency [%]");
    h2PASSInPixel->GetZaxis()->SetTitleOffset(1.6);
    h2PASSInPixel->GetXaxis()->SetTitleOffset(1.1);

    gPad->Update();
    TPaletteAxis *palette1 = (TPaletteAxis*)h2PASSInPixel->GetListOfFunctions()->FindObject("palette");
    if (palette1) {
        // Shift the palette
        palette1->SetX1NDC(0.805);
        palette1->SetX2NDC(0.845);
        palette1->SetY1NDC(0.12);
        palette1->SetY2NDC(0.74);
        gPad->Modified();
        gPad->Update();
    }

    c2->cd();
    h2ClSizeInPixel->Draw("COLZ");
    TLatex *t2 = new TLatex();
    t2->SetTextSize(0.043);
    t2->SetNDC();
    t2->DrawLatex(0.23, 0.78, Form("#splitline{#bf{MALTA2 Simulation}, 30#mum EPI}{             <Cl. Size> = %.2f}", simuClSizeMean));

    // Draw vertical line at xMid
    TLine *lineX2 = new TLine(xMid, yMin, xMid, yMax);
    lineX2->SetLineColor(kBlack);
    lineX2->SetLineWidth(3);
    lineX2->Draw("same");
    // Draw horizontal line at yMid
    TLine *lineY2 = new TLine(xMin, yMid, xMax, yMid);
    lineY2->SetLineColor(kBlack);
    lineY2->SetLineWidth(3);
    lineY2->Draw("same");
    h2ClSizeInPixel->GetZaxis()->SetTitle("Cluster Size");
    h2ClSizeInPixel->GetZaxis()->SetTitleOffset(1.4);
    h2ClSizeInPixel->GetXaxis()->SetTitleOffset(1.1);
    

    c3->cd();
    h2TimingInPixel->Draw("COLZ");
    TLatex *t3 = new TLatex();
    t3->SetTextSize(0.043);
    t3->SetNDC();
    t3->DrawLatex(0.23, 0.78, Form("#splitline{#bf{MALTA2 Simulation}, 30#mum EPI}{Center - Corner Timing = %.2f ns}", simuTimeDiff));
    
    // Draw vertical line at xMid
    TLine *lineX3 = new TLine(xMid, yMin, xMid, yMax);
    lineX3->SetLineColor(kBlack);
    lineX3->SetLineWidth(3);
    lineX3->Draw("same");
    // Draw horizontal line at yMid
    TLine *lineY3 = new TLine(xMin, yMid, xMax, yMid);
    lineY3->SetLineColor(kBlack);
    lineY3->SetLineWidth(3);
    lineY3->Draw("same");
    h2TimingInPixel->GetZaxis()->SetTitle("Cl. Timing - <Cl. Timing> [ns]");
    h2TimingInPixel->GetZaxis()->SetTitleOffset(1.6);
    h2TimingInPixel->GetXaxis()->SetTitleOffset(1.1);
    //h2TimingInPixel->GetZaxis()->SetLabelFont(72);
    //h2TimingInPixel->GetZaxis()->SetLabelOffset(0.04); 

    std::vector<double> zAxisTicksSimu = {0,-0.6, -0.4, -0.2, 0., 0.2, 0.4, 0.6, 0.8, 1, 1.2, 1.4};  
    for (int i = 0; i <= 10; i++) 
    {
        double v = zAxisTicksSimu[i];

        TString label;
        label.Form("%s%.1f", (v < 0 ? "- " : ""), fabs(v));

        h2TimingInPixel->GetZaxis()->ChangeLabel(i, -1,-1,-1,-1,-1, label);
    }

    gPad->Update();
    TPaletteAxis *palette3 = (TPaletteAxis*)h2TimingInPixel->GetListOfFunctions()->FindObject("palette");
    if (palette3) {
        // Shift the palette
        palette3->SetX1NDC(0.805);
        palette3->SetX2NDC(0.845);
        palette3->SetY1NDC(0.12);
        palette3->SetY2NDC(0.74);
        /*
        double x = palette3->GetX1();  // left/right of color bar
        double ylow = palette3->GetY1();
        double yhigh = palette3->GetY2();

        int nTicks = 8; // number of ticks you want
        for (int i = 0; i <= nTicks; i++) 
        {
            double val = h2TimingInPixel->GetMinimum() + i*(h2TimingInPixel->GetMaximum() - h2TimingInPixel->GetMinimum())/nTicks;
            double ypos = ylow + i*(yhigh - ylow)/nTicks;

            TString label;
            label.Form("%s%.1f", (val < 0 ? "- " : ""), fabs(val));

            TLatex *tex = new TLatex(x, ypos, label);
            tex->SetTextColor(kBlack);
            tex->SetTextAlign(31); // right-aligned
            tex->SetTextFont(42);
            tex->SetTextSize(0.03);
            tex->Draw("same");
        }
        */



        gPad->Modified();
        gPad->Update();
    }

    TBox *box = new TBox(13.7, 13.7, 22.8, 22.8);
    box->SetLineColor(kRed);
    box->SetLineWidth(3);
    box->SetFillStyle(0); 
    //box->Draw("same");

    TBox *boxCorner = new TBox(31.9, 31.9, 41, 41);
    boxCorner->SetLineColor(kRed);
    boxCorner->SetLineWidth(3);
    boxCorner->SetFillStyle(0); 
    //boxCorner->Draw("same");

    TArrow *arrow = new TArrow(22.8, 22.8, 31.9, 31.9);
    arrow->SetLineColor(kRed);
    arrow->SetLineWidth(3);
    arrow->SetArrowSize(0.02);  // adjust arrow head size
    //arrow->Draw();



    c4->cd();
    h2dataPASSInPixel->Draw("COLZ");
    TLatex *t4 = new TLatex();
    t4->SetTextSize(0.043);
    t4->SetNDC();
    t4->DrawLatex(0.29, 0.78, Form("#splitline{#bf{MALTA2 Data}, 30#mum EPI}{        <Eff.> = %.2f %}", dataEffMean));

    // Draw vertical line at xMid
    TLine *lineX4 = new TLine(xMid, yMin, xMid, yMax);
    lineX4->SetLineColor(kBlack);
    lineX4->SetLineWidth(3);
    lineX4->Draw("same");
    // Draw horizontal line at yMid
    TLine *lineY4 = new TLine(xMin, yMid, xMax, yMid);
    lineY4->SetLineColor(kBlack);
    lineY4->SetLineWidth(3);
    lineY4->Draw("same");
    h2dataPASSInPixel->GetZaxis()->SetTitle("Tracking Efficiency [%]");
    h2dataPASSInPixel->GetZaxis()->SetTitleOffset(1.6);
    h2dataPASSInPixel->GetXaxis()->SetTitleOffset(1.1);

    gPad->Update();
    TPaletteAxis *palette4 = (TPaletteAxis*)h2dataPASSInPixel->GetListOfFunctions()->FindObject("palette");
    if (palette4) {
        // Shift the palette
        palette4->SetX1NDC(0.805);
        palette4->SetX2NDC(0.845);
        palette4->SetY1NDC(0.12);
        palette4->SetY2NDC(0.74);
        gPad->Modified();
        gPad->Update();
    }

    c5->cd();
    h2dataClSizeInPixel->Draw("COLZ");
    TLatex *t5 = new TLatex();
    t5->SetTextSize(0.043);
    t5->SetNDC();
    t5->DrawLatex(0.29, 0.78, Form("#splitline{#bf{MALTA2 Data}, 30#mum EPI}{       <Cl. Size> = %.2f}", dataClSizeMean));

    // Draw vertical line at xMid
    TLine *lineX5 = new TLine(xMid, yMin, xMid, yMax);
    lineX5->SetLineColor(kBlack);
    lineX5->SetLineWidth(3);
    lineX5->Draw("same");
    // Draw horizontal line at yMid
    TLine *lineY5 = new TLine(xMin, yMid, xMax, yMid);
    lineY5->SetLineColor(kBlack);
    lineY5->SetLineWidth(3);
    lineY5->Draw("same");
    h2dataClSizeInPixel->GetZaxis()->SetTitle("Cluster Size");
    h2dataClSizeInPixel->GetZaxis()->SetTitleOffset(1.6);
    h2dataClSizeInPixel->GetXaxis()->SetTitleOffset(1.1);

    gPad->Update();
    TPaletteAxis *palette5 = (TPaletteAxis*)h2dataClSizeInPixel->GetListOfFunctions()->FindObject("palette");
    if (palette5) {
        // Shift the palette
        palette5->SetX1NDC(0.805);
        palette5->SetX2NDC(0.845);
        palette5->SetY1NDC(0.12);
        palette5->SetY2NDC(0.74);
        gPad->Modified();
        gPad->Update();
    }

    c6->cd();
    h2dataTimingInPixel->Draw("COLZ");
    TLatex *t6 = new TLatex();
    t6->SetTextSize(0.043);
    t6->SetNDC();
    t6->DrawLatex(0.22, 0.78, Form("#splitline{     #bf{MALTA2 Data}, 30#mum EPI}{Center - Corner Timing = %.2f ns}", dataTimeDiff));

    // Draw vertical line at xMid
    TLine *lineX6 = new TLine(xMid, yMin, xMid, yMax);
    lineX6->SetLineColor(kBlack);
    lineX6->SetLineWidth(3);
    lineX6->Draw("same");
    // Draw horizontal line at yMid
    TLine *lineY6 = new TLine(xMin, yMid, xMax, yMid);
    lineY6->SetLineColor(kBlack);
    lineY6->SetLineWidth(3);
    lineY6->Draw("same");
    h2dataTimingInPixel->GetZaxis()->SetTitle("Cl. Timing - <Cl. Timing> [ns]");
    h2dataTimingInPixel->GetZaxis()->SetTitleOffset(1.2);
    h2dataTimingInPixel->GetXaxis()->SetTitleOffset(1.1);

    gPad->Update();
    TPaletteAxis *palette6 = (TPaletteAxis*)h2dataTimingInPixel->GetListOfFunctions()->FindObject("palette");
    if (palette6) {
        // Shift the palette
        palette6->SetX1NDC(0.805);
        palette6->SetX2NDC(0.845);
        palette6->SetY1NDC(0.12);
        palette6->SetY2NDC(0.74);
        gPad->Modified();
        gPad->Update();
    }    

    TBox *dataBox = new TBox(13.7, 13.7, 22.8, 22.8);
    dataBox->SetLineColor(kRed);
    dataBox->SetLineWidth(3);
    dataBox->SetFillStyle(0); 
    box->Draw("same");

    TBox *dataBoxCorner = new TBox(31.9, 31.9, 41, 41);
    dataBoxCorner->SetLineColor(kRed);
    dataBoxCorner->SetLineWidth(3);
    dataBoxCorner->SetFillStyle(0); 
    dataBoxCorner->Draw("same");

    TArrow *dataArrow = new TArrow(22.8, 22.8, 31.9, 31.9);
    dataArrow->SetLineColor(kRed);
    dataArrow->SetLineWidth(3);
    dataArrow->SetArrowSize(0.02);  // adjust arrow head size
    //dataArrow->Draw();

    c7->cd();
    h2dataClSizeInPixelResidual_rebinned->Draw("COLZ");
    h2dataClSizeInPixelResidual_rebinned->GetZaxis()->SetTitleOffset(1.2);
    h2dataClSizeInPixelResidual_rebinned->GetXaxis()->SetTitleOffset(1.1);

    // Draw vertical line at xMid
    TLine *lineX7 = new TLine(xMid, yMin, xMid, yMax);
    lineX7->SetLineColor(kBlack);
    lineX7->SetLineWidth(3);
    lineX7->Draw("same");
    // Draw horizontal line at yMid
    TLine *lineY7 = new TLine(xMin, yMid, xMax, yMid);
    lineY7->SetLineColor(kBlack);
    lineY7->SetLineWidth(3);
    lineY7->Draw("same");

    TLatex *t7 = new TLatex();
    t7->SetTextSize(0.043);
    t7->SetNDC();
    t7->DrawLatex(0.24, 0.77, "#bf{MALTA2 Simulation}, 30#mum EPI");

    gPad->Update();
    TPaletteAxis *palette7 = (TPaletteAxis*)h2dataClSizeInPixelResidual_rebinned->GetListOfFunctions()->FindObject("palette");
    if (palette7) {
        // Shift the palette
        palette7->SetX1NDC(0.805);
        palette7->SetX2NDC(0.845);
        palette7->SetY1NDC(0.12);
        palette7->SetY2NDC(0.74);
        gPad->Modified();
        gPad->Update();
    }

    c8->cd();
    h2dataTimingInPixelResidual->Draw("COLZ");

    // Draw vertical line at xMid
    TLine *lineX8 = new TLine(xMid, yMin, xMid, yMax);
    lineX8->SetLineColor(kBlack);
    lineX8->SetLineWidth(3);
    lineX8->Draw("same");
    // Draw horizontal line at yMid
    TLine *lineY8 = new TLine(xMin, yMid, xMax, yMid);
    lineY8->SetLineColor(kBlack);
    lineY8->SetLineWidth(3);
    lineY8->Draw("same");

    TLatex *t8 = new TLatex();
    t8->SetTextSize(0.043);
    t8->SetNDC();
    t8->DrawLatex(0.22, 0.96, "#bf{Residual} In pixel timing. #bf{MALTA2} 30#mum EPI.");


    c9->cd();
    h2dataPASSInPixelResidual->Draw("COLZ");
    h2dataPASSInPixelResidual->GetZaxis()->SetTitleOffset(1.2);
    h2dataPASSInPixelResidual->GetXaxis()->SetTitleOffset(1.1);
    // Draw vertical line at xMid
    TLine *lineX = new TLine(xMid, yMin, xMid, yMax);
    lineX->SetLineColor(kBlack);
    lineX->SetLineWidth(3);
    lineX->Draw("same");

    // Draw horizontal line at yMid
    TLine *lineY = new TLine(xMin, yMid, xMax, yMid);
    lineY->SetLineColor(kBlack);
    lineY->SetLineWidth(3);
    lineY->Draw("same");

    TLatex *t9 = new TLatex();
    t9->SetTextSize(0.043);
    t9->SetNDC();
    t9->DrawLatex(0.24, 0.77, "#bf{MALTA2 Simulation} 30#mum EPI");

    gPad->Update();
    TPaletteAxis *palette9 = (TPaletteAxis*)h2dataPASSInPixelResidual->GetListOfFunctions()->FindObject("palette");
    if (palette9) {
        // Shift the palette
        palette9->SetX1NDC(0.805);
        palette9->SetX2NDC(0.845);
        palette9->SetY1NDC(0.12);
        palette9->SetY2NDC(0.74);
        gPad->Modified();
        gPad->Update();
    }

    c10->cd();
    h1dataClSizeInPixelResidual->Draw("HIST F");
    h1dataClSizeInPixelResidual->GetYaxis()->SetTitleOffset(1.7);
    h1dataClSizeInPixelResidual->GetXaxis()->SetTitleOffset(1.2);
    h1dataClSizeInPixelResidual->GetXaxis()->SetLabelSize(0.045);
    h1dataClSizeInPixelResidual->GetYaxis()->SetLabelSize(0.045);
    h1dataClSizeInPixelResidual->GetYaxis()->SetLabelOffset(0.012);
    h1dataClSizeInPixelResidual->GetXaxis()->SetNdivisions(707); 
    fit->Draw("SAME");
    TLatex *t10 = new TLatex();
    t10->SetTextSize(0.043);
    t10->SetNDC();
    t10->DrawLatex(0.24, 0.9, "#bf{MALTA2 Simulation}, 30#mum EPI");
    //Form("#bf{MALTA2} 30#mum EPI. Mean: %.2f, sigma: %.2f", mu1DClSizeResidual, sigma1DClSizeResidual)

    TLegend *leg1 = new TLegend(0.19,0.79,0.45,0.83);
    leg1->SetTextSize(0.06);
    leg1->SetBorderSize(0);
    leg1->AddEntry(fit, " ", "l");
    leg1->Draw();

    TLatex *fitInfo = new TLatex();
    fitInfo->SetTextSize(0.035);
    fitInfo->SetTextColor(2);
    fitInfo->SetNDC();
    fitInfo->DrawLatex(0.26, 0.8, Form("#splitline{#mu= %.2f #pm %.2f %}{#sigma= %.2f #pm %.2f %}", mu1DClSizeResidual, muerr1DClSizeResidual,  sigma1DClSizeResidual, sigmaerr1DClSizeResidual));

    c11->cd();
    h1dataPASSInPixelResidual->Draw("HIST F");
    h1dataPASSInPixelResidual->GetYaxis()->SetTitleOffset(1.7);
    h1dataPASSInPixelResidual->GetXaxis()->SetTitleOffset(1.2);
    h1dataPASSInPixelResidual->GetXaxis()->SetLabelSize(0.045);
    h1dataPASSInPixelResidual->GetYaxis()->SetLabelSize(0.045);
    h1dataPASSInPixelResidual->GetYaxis()->SetLabelOffset(0.012);
    h1dataPASSInPixelResidual->GetXaxis()->SetNdivisions(707); 
    fitEff->Draw("SAME");
    TLatex *t11 = new TLatex();
    t11->SetTextSize(0.05);
    t11->SetNDC();
    t11->DrawLatex(0.21, 0.9, "#bf{MALTA2 Simulation}, 30#mum EPI");
    //Form("#bf{MALTA2} 30#mum EPI. Mean: %.2f, sigma: %.2f", mu1DClSizeResidual, sigma1DClSizeResidual)

    TLegend *leg2 = new TLegend(0.2,0.68,0.45,0.75);
    leg2->SetTextSize(0.06);
    leg2->SetBorderSize(0);
    leg2->AddEntry(fitEff, " ", "l");
    leg2->Draw();

    TLatex *fitInfoEff = new TLatex();
    fitInfoEff->SetTextSize(0.035);
    fitInfoEff->SetTextColor(2);
    fitInfoEff->SetNDC();
    fitInfoEff->DrawLatex(0.27, 0.707, Form("#splitline{#mu= %.2f #pm %.2f %}{#sigma= %.2f #pm %.2f %}", mu1DEffResidual, muerr1DEffResidual , sigma1DEffResidual, sigmaerr1DEffResidual));

    c1->SaveAs("PublicPlots/Simu_2DEff.pdf");
    c1->SaveAs("PublicPlots/Simu_2DEff.C");

    c2->SaveAs("PublicPlots/Simu_2DClSize.pdf");
    c2->SaveAs("PublicPlots/Simu_2DClSize.C");

    c3->SaveAs("PublicPlots/Simu_2DTiming.pdf");
    c3->SaveAs("PublicPlots/Simu_2DTiming.C");

    c4->SaveAs("PublicPlots/Data_2DEff.pdf");
    c4->SaveAs("PublicPlots/Data_2DEff.C");

    c5->SaveAs("PublicPlots/Data_2DClSize.pdf");
    c5->SaveAs("PublicPlots/Data_2DClSize.C");

    c6->SaveAs("PublicPlots/Data_2DTiming.pdf");
    c6->SaveAs("PublicPlots/Data_2DTiming.C");

    c7->SaveAs("PublicPlots/Residual_2DClSize.pdf");
    c7->SaveAs("PublicPlots/Residual_2DClSize.C");

    c9->SaveAs("PublicPlots/Residual_2DEff.pdf");
    c9->SaveAs("PublicPlots/Residual_2DEff.C");

    c10->SaveAs("PublicPlots/Residual_1DClSize.pdf");
    c10->SaveAs("PublicPlots/Residual_1DClSize.C");

    c11->SaveAs("PublicPlots/Residual_1DEff.pdf");
    c11->SaveAs("PublicPlots/Residual_1DEff.C");
}
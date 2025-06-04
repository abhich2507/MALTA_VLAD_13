void visualize_TCT_data()
{
    TFile *file = TFile::Open("/home/vlad/Documents/DECAL/TCT_DATA/wave_SUBV10PDscan53_5perc.root");
    TTree *tree = (TTree*)file->Get("ntuple");

    float pitch, depth, ampl;

    tree->SetBranchAddress("Pitch", &pitch);
    tree->SetBranchAddress("Depth", &depth);
    tree->SetBranchAddress("Mean_ampl", &ampl);

    std::set<float> pitchVals, depthVals;
    for (Long64_t i = 0; i < tree->GetEntries(); ++i) 
    {
        tree->GetEntry(i);
        pitchVals.insert(pitch);
        depthVals.insert(depth);
    }

    std::vector<double> xBins(pitchVals.begin(), pitchVals.end());
    std::vector<double> yBins(depthVals.begin(), depthVals.end());

    // Add one more bin edge by extrapolating
    double lastStepX = xBins.back() - *(--(--xBins.end()));
    xBins.push_back(xBins.back() + lastStepX);

    double lastStepY = yBins.back() - *(--(--yBins.end()));
    yBins.push_back(yBins.back() + lastStepY);

    // Create the histogram
    TH2D *T2D_data = new TH2D("Pitch_Depth_data", "Pitch vs Depth",
                            xBins.size() - 1, xBins.data(),
                            yBins.size() - 1, yBins.data());
    TH3D *approx3D = new TH3D("Approx_3D_data", "Approx_3D_data",
                            41,0,41,
                            41,0,41,
                            41,0,41);
    Long64_t nEntries = tree->GetEntries();

    for (Long64_t i = 0; i < nEntries; ++i) 
    {
        tree->GetEntry(i);

        T2D_data->Fill(pitch, depth, ampl);
    }
    //T2D_data->Draw("COLZ");

    int nx = T2D_data->GetNbinsX();
    int ny = T2D_data->GetNbinsY();

    TH2D *T2D_flipped = (TH2D*)T2D_data->Clone("T2D_flipped");
    T2D_flipped->Reset(); // clear contents

    for (int ix = 1; ix <= nx; ++ix) 
    {
        for (int iy = 1; iy <= ny; ++iy) 
        {
            double val = T2D_data->GetBinContent(ix, iy);
            // Flip: new bin = (nx - ix + 1, ny - iy + 1)
            T2D_flipped->SetBinContent(nx - ix + 1, ny - iy + 1, val);
        }
    }
    TCanvas *c0 = new TCanvas("c0", "c0", 1000, 800);
    T2D_flipped->Draw();
    TH1D *projX = T2D_flipped->ProjectionX("projX"); 
    TH1D *projY = T2D_flipped->ProjectionY("projY"); 
    TCanvas *c1 = new TCanvas("c1", "c1", 1000, 800);
    projX->Draw();
    TCanvas *c2 = new TCanvas("c2", "c2", 1000, 800);
    projY->Draw();    


    TH2D *approx2D = (TH2D*)T2D_data->Clone("approx2D");
    approx2D->Reset();

    for (int ix = 1; ix <= projX->GetNbinsX(); ++ix) {
        for (int iy = 1; iy <= projY->GetNbinsX(); ++iy) {
            double val = projX->GetBinContent(ix) * projY->GetBinContent(iy);
            approx2D->SetBinContent(ix, iy, val);
        }
    }
    TCanvas *c3 = new TCanvas("c3", "c3", 1000, 800);
    approx2D->Draw();  


    double xmin = T2D_data->GetXaxis()->GetXmin();
    double xmax = 0;
    double ymin = T2D_data->GetYaxis()->GetXmin();
    double ymax = 0;
    double zmin = 0;
    double zmax = 0;

    for (int ix1 = 1; ix1 <= projX->GetNbinsX(); ++ix1) 
    {
        for (int ix2 = 1; ix2 <= projX->GetNbinsX(); ++ix2) 
        {
            for (int iy = 1; iy <= projY->GetNbinsX(); ++iy) 
            {
                // Normalize x,y,z axese
                double x1Center = T2D_flipped->GetXaxis()->GetBinCenter(ix1);
                double x1Norm = (x1Center - xmin) * 1000;
                double x2Center = T2D_flipped->GetXaxis()->GetBinCenter(ix2);
                double x2Norm = (x2Center - xmin) * 1000;
                double yCenter = T2D_flipped->GetYaxis()->GetBinCenter(iy);
                double yNorm = (yCenter - ymin) * 1000;                
                // Extrapolate 3D data
                double val = projX->GetBinContent(ix1) *projX->GetBinContent(ix2) * projY->GetBinContent(iy);
                approx3D->SetBinContent(x1Norm,x2Norm, yNorm, val);
                //cout << x1Norm << "; " << x2Norm << "; " << yNorm<< "\n";
            }
        }
    }
    TCanvas *c4 = new TCanvas("c4", "c4", 1000, 800);
    // Normalize COLZ axis
    double maxBin = approx3D->GetMaximum();
    if (maxBin > 0) 
    {
        approx3D->Scale(1.0 / maxBin);
    }
    approx3D->Draw("COLZ");  
    TCanvas *c5 = new TCanvas("c5", "c5", 1000, 800);
    TH2D* proj3DX = (TH2D*) approx3D->Project3D("xz");
    proj3DX->Draw("COLZ");

    double phys_pitch = 36.4;
    double phys_depth = 30;

    xmin = approx3D->GetXaxis()->GetXmin();
    xmax = approx3D->GetXaxis()->GetXmax();
    ymin = approx3D->GetYaxis()->GetXmin();
    ymax = approx3D->GetYaxis()->GetXmax();
    zmin = approx3D->GetZaxis()->GetXmin();
    zmax = approx3D->GetZaxis()->GetXmax();

    double x_correction = (xmax - phys_pitch) /2;
    double y_correction = (ymax - phys_pitch) /2;
    double z_correction = (zmax - phys_depth) /2;
    
    double core_xmin_bin = xmin + x_correction;
    double core_xmax_bin = xmax - x_correction;
    double core_ymin_bin = ymin + y_correction;
    double core_ymax_bin = ymax - y_correction;
    double core_zmin_bin = zmin + z_correction;
    double core_zmax_bin = zmax - z_correction;



    // Make the histogram core that corresponds to the physical pixel
    TH3D* h3_core = new TH3D("h3_core", "Core of Histogram",
                         core_xmax_bin - core_xmin_bin, 0, core_xmax_bin - x_correction,
                         core_ymax_bin - core_ymin_bin, 0, core_ymax_bin - y_correction,
                         core_zmax_bin - core_zmin_bin, 0, core_zmax_bin - z_correction);

    for (int ix = core_xmin_bin; ix <= core_xmax_bin; ++ix) 
    {
        for (int iy = core_ymin_bin; iy <= core_ymax_bin; ++iy) 
        {
            for (int iz = core_zmin_bin; iz <= core_zmax_bin; ++iz) 
            {
                double val = approx3D->GetBinContent(ix, iy, iz);

                // Get bin centers in old histogram
                double x = approx3D->GetXaxis()->GetBinCenter(ix);
                double y = approx3D->GetYaxis()->GetBinCenter(iy);
                double z = approx3D->GetZaxis()->GetBinCenter(iz);

                // Fill into new histogram
                h3_core->Fill(x - x_correction, y - y_correction, z - z_correction, val);
            }
        }
    }
    TCanvas *c6 = new TCanvas("c6", "c6", 1000, 800);
    h3_core->Draw("COLZ");

}
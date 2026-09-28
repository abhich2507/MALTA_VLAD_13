// plot_bkg_mom.C
void plot_bkg_mom() {
    TChain* c = new TChain("TruthVertex");
    for (int t = 0; t < 6; ++t)                    // match numThreadsLocal
        c->Add(Form("Results/local_%04d/output0_t%d.root", 1, t));
    // background only
    c->Draw("trueMomX>>hx(200,-10,10)",  "mcFlag==1");
    c->Draw("trueMomY>>hy(200,-5,5)",    "mcFlag==1");
    c->Draw("trueMomZ>>hz(300,0,15)",    "mcFlag==1");
    c->Draw("trueEnergy>>he(300,0,15)",  "mcFlag==1");
    
}
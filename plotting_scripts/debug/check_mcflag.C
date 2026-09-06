void check_mcflag(int run = 1) {
    TChain* c = new TChain("analyzedHits_planeZ0");
    c->Add(Form("./Results/local_%04d/analysis_results_MP/analysisThr100.root", run));
    int mcFlag = -99, clSize = -1;
    c->SetBranchAddress("mcFlag", &mcFlag);
    c->SetBranchAddress("clSize", &clSize);
    Long64_t nSig = 0, nSigPass = 0, nBkg = 0;
    for (Long64_t i = 0; i < c->GetEntries(); ++i) {
        c->GetEntry(i);
        if (mcFlag == 0) { nSig++; if (clSize > 0) nSigPass++; }
        else nBkg++;
    }
    printf("signal rows = %lld  pass = %lld  eff = %.2f%%   bkg rows = %lld\n",
           nSig, nSigPass, nSig ? 100.0*nSigPass/nSig : 0.0, nBkg);
}
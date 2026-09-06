void check_raw(int run = 1) {
    TChain* c = new TChain("RawPixelHits");
    c->Add(Form("Results/local_%04d/output0_t*.root", run));
    int iPlane;
    c->SetBranchAddress("iPlane", &iPlane);
    std::map<int,Long64_t> cnt;
    for (Long64_t i=0;i<c->GetEntries();++i){ c->GetEntry(i); cnt[iPlane]++; }
    printf("run %d raw: total=%lld\n", run, c->GetEntries());
    for (auto& kv : cnt) printf("  iPlane %d : %lld\n", kv.first, kv.second);
}
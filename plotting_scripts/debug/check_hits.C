void check_hits(int run = 1) {
    TChain* c = new TChain("ReconstructedHits");
    c->Add(Form("Results/local_%04d/analysis_results_MP/ReconstructedHitsThr100.root", run));
    int planeID;
    c->SetBranchAddress("planeID", &planeID);
    std::map<int,Long64_t> cnt;
    for (Long64_t i=0;i<c->GetEntries();++i){ c->GetEntry(i); cnt[planeID]++; }
    printf("run %d: total=%lld\n", run, c->GetEntries());
    for (auto& kv : cnt) printf("  planeID %d : %lld hits\n", kv.first, kv.second);
}
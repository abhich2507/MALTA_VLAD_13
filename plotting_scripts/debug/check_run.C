void check_run(int run = 1) {
    // (a) signal beam width
    TChain* tv = new TChain("TruthVertex");
    tv->Add(Form("Results/local_%04d/output0_t*.root", run));
    int mcFlag; float vx, vy;
    tv->SetBranchAddress("trueVertexX", &vx);
    tv->SetBranchAddress("trueVertexY", &vy);
    tv->SetBranchAddress("mcFlag", &mcFlag);
    double sx=0, sy=0, sx2=0; Long64_t n=0;
    for (Long64_t i=0;i<tv->GetEntries();++i){ tv->GetEntry(i); if(mcFlag==0){sx+=vx; sy+=vy; sx2+=vx*vx; n++;} }
    double mx=sx/n, my=sy/n, rms=sqrt(sx2/n - mx*mx);
    printf("beam: n=%lld meanX=%.3f mm  rmsX=%.3f mm  meanY=%.3f mm\n", n, mx, rms, my);

    // (b) tracking sentinels (unmatched signal tracks)
    TChain* th = new TChain("TrackedHits_planeZ0");
    th->Add(Form("Results/local_%04d/analysis_results_MP/LocalTrackedHitsThr100.root", run));
    int tmc, tid, px, nh;
    th->SetBranchAddress("mcFlag", &tmc);
    th->SetBranchAddress("trackID", &tid);
    th->SetBranchAddress("DUTPixX", &px);
    th->SetBranchAddress("DUTnHits", &nh);
    std::set<int> ids, un;
    Long64_t srows=0, sent=0;
    for (Long64_t i=0;i<th->GetEntries();++i){ th->GetEntry(i); if(tmc!=0) continue; srows++; ids.insert(tid); if(px==-1){sent++; un.insert(tid);} }
    printf("tracking: signal rows=%lld unique=%zu  sentinel=%lld unmatched_tracks=%zu\n",
           srows, ids.size(), sent, un.size());
    
    // (c) background beam (mcFlag==1)
    TChain* tb = new TChain("TruthVertex");
    tb->Add(Form("Results/local_%04d/output0_t*.root", run));
    int bmc; float bvx, bvy;
    tb->SetBranchAddress("trueVertexX", &bvx);
    tb->SetBranchAddress("trueVertexY", &bvy);
    tb->SetBranchAddress("mcFlag", &bmc);
    double bsx=0, bsx2=0; Long64_t bn=0;
    for (Long64_t i=0;i<tb->GetEntries();++i){ tb->GetEntry(i); if(bmc==1){bsx+=bvx; bsx2+=bvx*bvx; bn++;} }
    printf("bkg beam: n=%lld meanX=%.3f mm  rmsX=%.3f mm\n", bn, bn?bsx/bn:0, bn?sqrt(bsx2/bn-(bsx/bn)*(bsx/bn)):0);
}
# Prompt for optimal signal-only hit-efficiency design (MALTA Geant4 simulation)

Copy everything below the line into Gemini/Opus/GPT as a single prompt. It is self-contained.

-------------------------------------------------------------------------------

You are designing a modification to a Geant4 test-beam simulation + ROOT analysis chain. Your deliverable is a rigorous, staged, line-by-line implementation plan (no code should be written blindly). Read the code first, verify every claim below, and answer every question in Section 7.

## 1. Objective

Measure hit efficiency ONLY for the signal particle in a pile-up setup (EIC FMT), after the ENTIRE reconstruction chain (digital processing -> tracking -> clustering -> analysis):

- N_generated = number of signal particles generated
- N_passed    = number of those signal particles that survive every stage (end with a final cluster, clSize > 0)
- signalEff   = N_passed / N_generated * 100, with binomial error, per plane and overall

This is a SECOND efficiency, in addition to the existing one which mixes signal + background and must stay unchanged.

## 2. Simulation setup

- Repo: malta_simulation/ (CMake project; executables in build/).
- One Geant4 event contains `particleCount` primaries: exactly 1 SIGNAL (pi+, config energy) and 10 BACKGROUND (e-, fixed energy). The signal is at a RANDOM index among the primaries (not necessarily first). Config: configs/flags_MP_EIC.cfg (largeScaleFlag = EIC_FMT, particleCount = 11, particleType = pi+, bkgparticleType = e-, itkEnable = false).
- src/PrimaryGenerator.cc, GeneratePrimaries(): fills the TruthVertex ntuple per primary with: iEvent, trueVertexX/Y/Z, trueGlobalTime, trackID (counter 1..N per event, incremented per generated primary; equals the Geant4 track ID because primaries are generated sequentially), mcFlag (0 = signal, 1 = background).
- Sensor: 512x512 pixels, MALTA2. Charge sharing splits each deposit over 4 pixels with analytical efficiency effAn (src/SensitiveDetector.cc). Deposits are converted to electrons (MeV * 1e6 / 3.66 eV) and accumulated per pixel in src/EventAction.cc (map key = plane, pixX, pixY, so deposits of ALL particles are merged per pixel). A hit row is stored only if the pixel has >= 50 electrons; hitTime is the first time the pixel crossed 50 e-.

## 3. Geant4 output files (Results/local_NNNN/output0_t0..5.root, one per thread)

- Tree RawPixelHits: iEvent, iPlane, iModule, PixX, PixY, hitTime, hitEnergy. NO track identity.
- Tree TruthVertex:  iEvent, trueVertexX, trueVertexY, trueVertexZ, trueGlobalTime, trackID, mcFlag. This tree ALREADY has trackID and mcFlag.

## 4. Analysis chain (ONLY analysis_multiPlane/ is built by CMake; the old analysis/ folder is legacy and must NOT be touched)

Each stage is a separate executable (build/run_DigitalProcessing, run_Tracking, run_Clustering, run_Analysis) that reads the previous stage's ROOT files. Key structs:

- TrackEntry    {x, y, z, t}                                     (include/Tracking_multiPlane.hh)
- FullTrackInfo {planeID, trackID, vertexX, vertexY, vertexTime, dutX, dutY, dutNHits, dutTime}
- ClusteredHit  {planeID, x, y, clSize, timing, corrTiming}      (include/Clustering_multiPlane.hh)
- ClusterState  {cluster, currentX, currentY, currentPlaneID, currentTrackID, currentPixY}
- AnalysisHits  {planeID, x, y, clSize, timing, correctedTiming} (include/Analysis_multiPlane.hh)
- ProcessedHit  {planeID, x, y, time, nHit}
- RawHit        {HitKey{plane,event,x,y}, energy, time}

Stage 1 - DigitalProcessing_multiPlane (src/DigitalProcessing_multiPlane.cc, src/DigitalUtils.cc):
  GetRawHits (src/RootIO.cc) reads RawPixelHits -> RawHit. Digitization: threshold map (MALTA2), word building, merging -> ProcessedHit.
  Writes {run}/<save>/ReconstructedHitsThr{thr}.root: planeID, PixX, PixY, timing, NHits. NO track identity.

Stage 2 - Tracking_multiPlane (src/Tracking_multiPlane.cc, src/TrackingUtils.cc):
  GetVertex (src/RootIO.cc) reads TruthVertex but ONLY x,y,z,t into TrackEntry - it DROPS trackID and mcFlag - then sorts tracks by time.
  MatchHits(tracks, hits, cfg, runNumber): assigns trackID = index i in the time-sorted truth list; sliding time window (cfg.timeCut) over reconstructed hits; for each hit computes residual between reconstructed pixel position (PixelPositionReconstruction) and the track extrapolated intercept (IntersectTrackPlane: vertex + fixed momentum from flags.cfg, geometry from geoFile); applies position cut (cfg.distCut); saves residuals; EVERY truth track gets at least one FullTrackInfo row (sentinel DUTPixX = -1, DUTnHits = 0 if nothing matched; multiple rows if multiple hits matched).
  Writes {run}/<save>/LocalTrackedHitsThr{thr}.root, trees TrackedHits_planeZ%d: planeID, trackID, vertexX, vertexY, vertexTime, DUTPixX, DUTPixY, DUTnHits, DUTLocalTime. (trackID present, mcFlag absent.)

Stage 3 - Clustering_multiPlane (src/Clustering_multiPlane.cc, src/ClusteringUtils.cc):
  GetMatchedHits reads tracked rows -> FullTrackInfo. GroupHitsByTrack groups by consecutive equal trackID. BuildClusterState / ResetClusterState fill ClusterState (it already carries currentTrackID). ValidateCluster erases sentinel (-1) pixels and diagonal-only pixels. GetValidCluster produces EXACTLY ONE ClusteredHit per truth track (clSize = 0 when nothing survives).
  Writes {run}/<save>/analysisThr{thr}.root, trees analyzedHits_planeZ%d: planeID, analysisVertexX, analysisVertexY, clSize, timing, correctedTiming. NO trackID/mcFlag.

Stage 4 - Analysis_multiPlane (src/Analysis_multiPlane.cc, src/AnalysisUtils.cc):
  GetAnalysisHits reads analyzed rows -> AnalysisHits. FillHistograms folds positions into 2x2 in-pixel maps (with optional Gaussian smearing) and fills kALL (weight 1 per row), kPASS (clSize > 0), kClSize, kTiming and their InPixel variants. GetStatistics: avgEff = getEff(Integral(kPASSInPixel), Integral(kALLInPixel)) - this is the existing efficiency and it MIXES signal + background in both numerator and denominator. SaveSummaryRoot writes Plots/local_NNNN/<save>/summary.root with tree summaryTree (threshold, efficiency, effError, clSize, clSizeError, timing).

## 5. Proposed direction already established (verify, then follow unless you find a real flaw)

- Signal identity can flow ENTIRELY on the truth side: TruthVertex -> TrackEntry -> FullTrackInfo -> ClusteredHit -> AnalysisHits. NO Geant4 changes required.
- Every truth track is guaranteed exactly one row in analyzedHits_planeZ%d (verified sentinel + clSize=0 code paths), so "mcFlag == 0 rows" at the end = signals generated and "mcFlag == 0 && clSize > 0" = signals that passed everything.
- Keep the existing index-based trackID (used by GroupHitsByTrack, which relies on time-ordering) UNCHANGED for grouping; carry the Geant4 truth trackID as a NEW field (e.g., mcTrackID), because truth trackIDs repeat across events (1..N per event).
- Structs may be extended by appending fields at the end; existing aggregate initializers remain valid (missing members are value-initialized).
- Workflow preference: one step at a time, line-by-line diffs, validation after each stage.

## 6. Open question about RawPixelHits truth (analyze, recommend, but do NOT assume)

RawPixelHits has no trackID and EventAction merges all tracks per pixel. Raw-level trackID would enable per-event validation of the tracking match ("did the signal's own deposits cross threshold near its predicted pixel?") and estimation of background contamination faking a pass. However the digitizer (merging/bus merging) destroys hit identity anyway. Decide whether adding trackID to RawPixelHits (requires splitting the per-pixel merge in EventAction, i.e., Geant4 change + new ntuple column) is worth it, or whether residual/timing distributions (already saved as h1ResidualX/Y per plane) are sufficient statistical validation. Give a clear recommendation and the exact extra edits if recommended.

## 7. Questions you MUST answer rigorously

1. Exact file-by-file, function-by-function edit list with insertion points and before/after logic, covering: TrackEntry, FullTrackInfo, ClusteredHit, ClusterState, AnalysisHits; GetVertex, MatchHits, FillTrackedTree (both overloads), GetMatchedHits, ResetClusterState, BuildClusterState, GetValidCluster, GetAnalysisHits; FillHistograms, Create2DHistograms (signal-only maps), GetStatistics, SaveSummaryRoot. State precisely which fields each function must copy and which branches each tree gains.
2. Edge cases: sentinel rows, tracks with 0 / 1 / many matched hits, mcFlag semantics per row, events with zero primaries, per-plane denominator when a signal does not cross a plane, multi-thread file concatenation, run-to-run merging.
3. Validation protocol after EACH stage: e.g., count of mcFlag == 0 rows must be identical in TruthVertex, TrackedHits_planeZ%d, analyzedHits_planeZ%d; residual and timing distributions for signal tracks only (peak at 0, width ~ tracking resolution) vs background (flat); expected value of signalEff for a known-good run (e.g., MIP efficiency should be ~ high, close to the mixed efficiency since background e- hits are mostly random); cross-check N_generated against TruthVertex directly.
4. Contamination: derive how to estimate the probability that a background hit inside the distCut window fakes a signal pass, and how to fold this into a systematic uncertainty on signalEff.
5. Statistics: binomial errors per plane, overall, and across multiple runs; weighting when runs have different signal counts.
6. Test plan using the existing integration tests (tests/integration/test_fullDigitalProcessing/: test_fullTracking.cc, test_fullClustering.cc, test_fullAnalysis.cc) and a small end-to-end run with configs/flags_MP_EIC.cfg + configs/analysis_flags_MP_EIC_Vlad.cfg.

## 8. Required output format

A numbered, staged implementation plan with:
(a) for each stage: the exact code change (diff-style) in each file,
(b) the build + run + validation command for that stage,
(c) the exact expected output that proves correctness,
(d) the final summary format (new summary.root branches and console output), and
(e) a short risk list with mitigations.

#!/bin/bash
# =============================================================================
# run_single.sh - run ONE simulation run and the full analysis chain.
#
# Steps (same as run_efficiency.sh, but for a single run):
#   1. SIMULATION    run_script_local.sh     -> Results/local_NNNN/ (raw Geant4)
#   2. ANALYSIS      run_mult.sh             -> Results/local_NNNN/<SAVE>/,
#                                               Plots/local_NNNN/<SAVE>/histos.root
#   3. COINCIDENCE   run_coin_eff.sh         -> Results/coin_eff.csv
#   4. EFFICIENCY    save_bkg_eff.C          -> Results/bkg_eff.csv
#   5. PLOTS         BkgEff.c                -> Plots/bkg_eff.png / .pdf
#
# Usage (from malta_simulation/):
#   ./run_single.sh              full chain for one fresh run
#   ./run_single.sh --no-sim     skip simulation, reuse the latest Results/local_NNNN
#
# Settings are overridable via env:
#   SIM_FLAGS_CFG  (default flags_MP_EIC.cfg)
#   ANALYSIS_CFG   (default analysis_flags_MP_EIC_Vlad.cfg)
#   SAVE           (default analysis_results_MP)
#   THRESHOLD      (default 100)
#   WINDOW_NS      (default 8)
# =============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

SIM_FLAGS_CFG="${SIM_FLAGS_CFG:-flags_MP_EIC.cfg}"
ANALYSIS_CFG="${ANALYSIS_CFG:-analysis_flags_MP_EIC_Vlad.cfg}"
SAVE="${SAVE:-analysis_results_MP}"
THRESHOLD="${THRESHOLD:-100}"
WINDOW_NS="${WINDOW_NS:-8}"

DO_SIM=true
for arg in "$@"; do
    case "$arg" in
        --no-sim) DO_SIM=false ;;
        -h|--help) sed -n '2,21p' "$0"; exit 0 ;;
        *) echo "Unknown option: $arg (use --no-sim or -h)" >&2; exit 1 ;;
    esac
done

# 1. Simulation (single run) ---------------------------------------------------
if $DO_SIM; then
    echo
    echo "== [1/5] SIMULATION (single run, cfg=$SIM_FLAGS_CFG) =="
    pushd build >/dev/null
    source run_script_local.sh "$SIM_FLAGS_CFG"
    popd >/dev/null
else
    echo
    echo "== [1/5] SIMULATION skipped (--no-sim) =="
fi

# Resolve the latest run directory --------------------------------------------
RUN_DIR=$(ls -d Results/local_* 2>/dev/null | sort -V | tail -n 1)
if [ -z "$RUN_DIR" ]; then
    echo "ERROR: no Results/local_* directory found" >&2
    exit 1
fi
RUN=$((10#$(basename "$RUN_DIR" | sed 's/local_//')))
echo "Using run $RUN ($RUN_DIR)"

# 2. Analysis ------------------------------------------------------------------
echo
echo "== [2/5] ANALYSIS (digitize + tracking + clustering + analysis) =="
SAVE="$SAVE" CONFIG="$ANALYSIS_CFG" THRESHOLD="$THRESHOLD" ./run_mult.sh "$RUN"

# 3. Coincidence efficiency ----------------------------------------------------
echo
echo "== [3/5] COINCIDENCE EFFICIENCY -> Results/coin_eff.csv =="
SAVE="$SAVE" THRESHOLD="$THRESHOLD" WINDOW_NS="$WINDOW_NS" \
    bash plotting_scripts/run_coin_eff.sh "$RUN"

# 4. Signal efficiency ---------------------------------------------------------
echo
echo "== [4/5] SIGNAL EFFICIENCY -> Results/bkg_eff.csv =="
root -l -b -q "save_bkg_eff.C(\"$SAVE\", $THRESHOLD)"

# 5. Plots ---------------------------------------------------------------------
echo
echo "== [5/5] PLOTS -> Plots/bkg_eff.png / bkg_eff.pdf =="
root -l -b -q 'plotting_scripts/BkgEff.c(1)'

echo
echo "============================================================================="
echo " Done (run $RUN)."
echo "   per-plane efficiency : Results/bkg_eff.csv"
echo "   coincidence efficiency: Results/coin_eff.csv"
echo "   plots                : Plots/bkg_eff.png / Plots/bkg_eff.pdf"
echo "============================================================================="

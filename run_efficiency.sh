#!/bin/bash
# =============================================================================
# run_efficiency.sh - one-command efficiency scan pipeline
#
# Runs the chain in the correct order:
#   1. SIMULATION    bkg_scan.sh                -> Results/local_NNNN/ (raw Geant4)
#   2. ANALYSIS      run_mult.sh (per run)      -> Results/local_NNNN/<SAVE>/,
#                                                  Plots/local_NNNN/<SAVE>/histos.root
#   3. COINCIDENCE   Coincidence.cc (per run)   -> Results/coin_eff.csv
#   4. EFFICIENCY    save_bkg_eff.C             -> Results/bkg_eff.csv
#   5. PLOTS         BkgEff.c                   -> Plots/bkg_eff.png / .pdf
#
# Usage (from malta_simulation/):
#   ./run_efficiency.sh               full chain (simulation + analysis + extraction)
#   ./run_efficiency.sh --no-sim      skip the Geant4 scan (Results/local_NNNN exist)
#   ./run_efficiency.sh --only-extract  only re-run steps 3-5 (CSVs + plots)
#
# The run list always comes from configs/bkg_count.csv (run,count columns).
# =============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# ---- configuration (single place to edit) -----------------------------------
SIM_FLAGS_CFG="flags_MP_EIC.cfg"              # passed to bkg_scan.sh
ANALYSIS_CFG="analysis_flags_MP_EIC_Vlad.cfg" # passed to run_mult.sh
SAVE="analysis_results_MP"                    # analysis save name (all steps)
THRESHOLD=100                                 # analysis threshold in e-
WINDOW_NS=8                                   # coincidence time window in ns
CSV="$SCRIPT_DIR/configs/bkg_count.csv"
# ------------------------------------------------------------------------------

DO_SIM=true
DO_ANALYSIS=true

for arg in "$@"; do
    case "$arg" in
        --no-sim)        DO_SIM=false ;;
        --only-extract)  DO_SIM=false; DO_ANALYSIS=false ;;
        -h|--help)
            sed -n '2,18p' "$0"; exit 0 ;;
        *)
            echo "Unknown option: $arg (use --no-sim, --only-extract or -h)" >&2
            exit 1 ;;
    esac
done

# Read the run list from configs/bkg_count.csv (column 1)
runs=()
while IFS=',' read -r run rest; do
    case "$run" in ''|run) continue ;; esac
    [[ "$run" =~ ^[0-9]+$ ]] && runs+=("$run")
done < "$CSV"
[ "${#runs[@]}" -gt 0 ] || { echo "ERROR: no runs found in $CSV" >&2; exit 1; }

echo "============================================================================="
echo " Efficiency scan: ${#runs[@]} run(s) | SAVE=$SAVE | thr=$THRESHOLD e- |"
echo "                  window=$WINDOW_NS ns"
echo "============================================================================="

# 1. Simulation scan -----------------------------------------------------------
if $DO_SIM; then
    echo
    echo "== [1/5] SIMULATION (bkg_scan.sh, cfg=$SIM_FLAGS_CFG) =="
    ./bkg_scan.sh
else
    echo
    echo "== [1/5] SIMULATION skipped (--no-sim) =="
fi

# 2. Analysis per run -----------------------------------------------------------
if $DO_ANALYSIS; then
    echo
    echo "== [2/5] ANALYSIS (digitize + tracking + clustering + analysis) =="
    for run in "${runs[@]}"; do
        echo "----- run $run -----"
        SAVE="$SAVE" CONFIG="$ANALYSIS_CFG" THRESHOLD="$THRESHOLD" ./run_mult.sh "$run"
    done
else
    echo
    echo "== [2/5] ANALYSIS skipped (--only-extract) =="
fi

# 3. Coincidence efficiency ----------------------------------------------------
echo
echo "== [3/5] COINCIDENCE EFFICIENCY -> Results/coin_eff.csv =="
SAVE="$SAVE" THRESHOLD="$THRESHOLD" WINDOW_NS="$WINDOW_NS" \
    bash plotting_scripts/run_coin_eff.sh "${runs[@]}"

# 4. Per-plane signal efficiency vs background --------------------------------
echo
echo "== [4/5] SIGNAL EFFICIENCY -> Results/bkg_eff.csv =="
root -l -b -q "save_bkg_eff.C(\"$SAVE\", $THRESHOLD)"

# 5. Plots ---------------------------------------------------------------------
echo
echo "== [5/5] PLOTS -> Plots/bkg_eff.png / bkg_eff.pdf =="
root -l -b -q 'plotting_scripts/BkgEff.c(1)'

echo
echo "============================================================================="
echo " Done."
echo "   per-plane efficiency : Results/bkg_eff.csv"
echo "   coincidence efficiency: Results/coin_eff.csv"
echo "   plots                : Plots/bkg_eff.png / Plots/bkg_eff.pdf"
echo "============================================================================="

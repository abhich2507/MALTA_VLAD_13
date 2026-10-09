#!/bin/bash
# =============================================================================
# run_efficiency.sh - one-command efficiency scan pipeline
#
# Runs the chain in the correct order:
#   1. SIMULATION    bkg_scan.sh                -> Results/local_NNNN/ (raw Geant4)
#   2. ANALYSIS      run_mult.sh (per run)      -> Results/local_NNNN/<SAVE>/,
#                                                  Plots/local_NNNN/<SAVE>/histos.root
#   3. COINCIDENCE   Coincidence.cc (per run)   -> $RES_DIR/coin_eff.csv
#   4. EFFICIENCY    save_bkg_eff.C             -> $RES_DIR/bkg_eff.csv
#   5. PLOTS         BkgEff.c                   -> $OUT_DIR/bkg_eff.png / .pdf
#   6. ARCHIVE       move per-run outputs       -> $RES_DIR/local_NNNN, $OUT_DIR/local_NNNN
#
# Usage (from malta_simulation/):
#   ./run_efficiency.sh               full chain (simulation + analysis + extraction)
#   ./run_efficiency.sh --no-sim      skip the Geant4 scan (Results/local_NNNN exist)
#   ./run_efficiency.sh --only-extract  only re-run steps 3-5 (CSVs + plots)
#
# The run list always comes from configs/bkg_rate.csv (run,bkgRateMean columns).
# =============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# ---- configuration (single place to edit) -----------------------------------
SIM_FLAGS_CFG="flags_MP_EIC.cfg"              # passed to bkg_scan.sh
ANALYSIS_CFG="analysis_flags_MP_EIC_Vlad.cfg" # passed to run_mult.sh
SAVE="analysis_results_MP"                    # analysis save name (all steps)
THRESHOLD=100                                 # analysis threshold in e-
WINDOW_NS=36                                # coincidence time window in ns
CSV="$SCRIPT_DIR/configs/bkg_rate.csv"
RES_DIR="Results_pi1GeV_eEIC_mp_coin"                             # final results dir: CSVs + per-run trees/raw
OUT_DIR="Plots_pi1GeV_eEIC_mp_coin"                               # final plots dir: bkg_eff.png/pdf + per-run histos
# ------------------------------------------------------------------------------

DO_SIM=true
DO_ANALYSIS=true
SEL_RUNS=()

for arg in "$@"; do
    case "$arg" in
        --no-sim)        DO_SIM=false ;;
        --only-extract)  DO_SIM=false; DO_ANALYSIS=false ;;
        -h|--help)
            sed -n '2,18p' "$0"; exit 0 ;;
        *)
            if [[ "$arg" =~ ^[0-9]+$ ]]; then
                SEL_RUNS+=("$arg")
            else
                echo "Unknown option: $arg (use --no-sim, --only-extract, -h, or a run number)" >&2
                exit 1
            fi
            ;;
    esac
done

# Read the run list from configs/bkg_rate.csv (column 1)
runs=()
while IFS=',' read -r run rest; do
    case "$run" in ''|run) continue ;; esac
    [[ "$run" =~ ^[0-9]+$ ]] && runs+=("$run")
done < "$CSV"
[ "${#runs[@]}" -gt 0 ] || { echo "ERROR: no runs found in $CSV" >&2; exit 1; }

# Restrict to the selected runs, if any were given
if [ "${#SEL_RUNS[@]}" -gt 0 ]; then
    runs=("${SEL_RUNS[@]}")
fi

echo "============================================================================="
echo " Efficiency scan: ${#runs[@]} run(s) | SAVE=$SAVE | thr=$THRESHOLD e- |"
echo "                  window=$WINDOW_NS ns"
echo "============================================================================="

# 1. Simulation scan -----------------------------------------------------------
if $DO_SIM; then
    echo
    echo "== [1/6] SIMULATION (bkg_scan.sh, cfg=$SIM_FLAGS_CFG) =="
    if [ "${#SEL_RUNS[@]}" -gt 0 ]; then
        ./bkg_scan.sh "${SEL_RUNS[@]}"
    else
        ./bkg_scan.sh
    fi
else
    echo
    echo "== [1/6] SIMULATION skipped (--no-sim) =="
fi

# 2. Analysis per run -----------------------------------------------------------
if $DO_ANALYSIS; then
    echo
    echo "== [2/6] ANALYSIS (digitize + tracking + clustering + analysis) =="
    for run in "${runs[@]}"; do
        echo "----- run $run -----"
        SAVE="$SAVE" CONFIG="$ANALYSIS_CFG" THRESHOLD="$THRESHOLD" ./run_mult.sh "$run"
    done
else
    echo
    echo "== [2/6] ANALYSIS skipped (--only-extract) =="
fi

# 3. Coincidence efficiency ----------------------------------------------------
echo
echo "== [3/6] COINCIDENCE EFFICIENCY -> $RES_DIR/coin_eff.csv =="
SAVE="$SAVE" THRESHOLD="$THRESHOLD" WINDOW_NS="$WINDOW_NS" RES_DIR="$RES_DIR" \
    bash plotting_scripts/run_coin_eff.sh "${runs[@]}"

# 4. Per-plane signal efficiency vs background --------------------------------
echo
echo "== [4/6] SIGNAL EFFICIENCY -> $RES_DIR/bkg_eff.csv =="
root -l -b -q "save_bkg_eff.C(\"$SAVE\", $THRESHOLD, \"$RES_DIR\")"

# 5. Plots ---------------------------------------------------------------------
echo
echo "== [5/6] PLOTS -> $OUT_DIR/bkg_eff.png / bkg_eff.pdf =="
root -l -b -q "plotting_scripts/BkgEff.c(1, \"$RES_DIR\", \"$OUT_DIR\")"

# 6. Archive per-run outputs into the final dirs -------------------------------
echo
echo "== [6/6] ARCHIVE per-run outputs -> $RES_DIR / $OUT_DIR =="
mkdir -p "$RES_DIR" "$OUT_DIR"
{
    echo "# Scan finalized on $(date '+%Y-%m-%d %H:%M:%S')"
    echo "SAVE=$SAVE"
    echo "THRESHOLD=$THRESHOLD"
    echo "WINDOW_NS=$WINDOW_NS"
    echo "SIM_FLAGS_CFG=$SIM_FLAGS_CFG"
    echo "ANALYSIS_CFG=$ANALYSIS_CFG"
} > "$RES_DIR/scan_settings.txt"
cp "$RES_DIR/scan_settings.txt" "$OUT_DIR/scan_settings.txt"
for run in "${runs[@]}"; do
    n=$(printf %04d "$run")
    if [ -d "Results/local_$n" ]; then
        rm -rf "$RES_DIR/local_$n"
        mv "Results/local_$n" "$RES_DIR/local_$n"
    fi
    if [ -d "Plots/local_$n" ]; then
        rm -rf "$OUT_DIR/local_$n"
        mv "Plots/local_$n" "$OUT_DIR/local_$n"
    fi
done
echo "   archived ${#runs[@]} run(s) into $RES_DIR and $OUT_DIR"

echo
echo "============================================================================="
echo " Done."
echo "   per-plane efficiency : $RES_DIR/bkg_eff.csv"
echo "   coincidence efficiency: $RES_DIR/coin_eff.csv"
echo "   plots                : $OUT_DIR/bkg_eff.png / $OUT_DIR/bkg_eff.pdf"
echo "   per-run outputs      : $RES_DIR/local_NNNN, $OUT_DIR/local_NNNN"
echo "============================================================================="

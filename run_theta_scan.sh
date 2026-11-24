#!/bin/bash
# run_theta_scan.sh - one-command theta-scan pipeline (efficiency vs incidence angle)
#   1. SIM+ANA   theta_scan_X.sh         -> Results/local_NNNN, Plots/local_NNNN
#   2. COIN      run_coin_eff.sh         -> $RES_DIR/coin_eff.csv
#   3. EFF       save_theta_eff.C        -> $RES_DIR/theta_eff.csv
#   4. PLOT      ThetaEff.c              -> $OUT_DIR/theta_eff.png / .pdf
#   5. ARCHIVE   move per-run outputs    -> $RES_DIR, $OUT_DIR
# Usage:
#   ./run_theta_scan.sh                  full chain
#   ./run_theta_scan.sh --no-sim         skip simulation (analysis + extraction)
#   ./run_theta_scan.sh --only-extract   only steps 2-4 (CSVs + plots)
#   ./run_theta_scan.sh 406              single run
# Run list comes from configs/theta_scan_X.csv (run, theta_deg).
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# ---- configuration (single place to edit) -----------------------------------
SCAN_SCRIPT="theta_scan_X.sh"                        # theta_scan.sh = Y scan
SIM_FLAGS_CFG="flags_MP_EIC_224x512_thetaX.cfg"
ANALYSIS_CFG="analysis_flags_MP_EIC_224x512.cfg"
SAVE="analysis_results_224x512_thetaX"
THRESHOLD=100
WINDOW_NS=36
CSV="$SCRIPT_DIR/configs/theta_scan_X.csv"
RES_DIR="Results_thetaX"                             # <-- set your dir name here
OUT_DIR="Plots_thetaX"                               # <-- and here
# ------------------------------------------------------------------------------

DO_SIM=true
DO_ANALYSIS=true
SEL_RUNS=()

for arg in "$@"; do
    case "$arg" in
        --no-sim)        DO_SIM=false ;;
        --only-extract)  DO_SIM=false; DO_ANALYSIS=false ;;
        -h|--help)       sed -n '2,18p' "$0"; exit 0 ;;
        *) if [[ "$arg" =~ ^[0-9]+$ ]]; then SEL_RUNS+=("$arg"); else
               echo "Unknown option: $arg" >&2; exit 1; fi ;;
    esac
done

runs=()
while IFS=',' read -r run rest || [ -n "$run" ]; do
    case "$run" in ''|run) continue ;; esac
    [[ "$run" =~ ^[0-9]+$ ]] && runs+=("$run")
done < "$CSV"
[ "${#runs[@]}" -gt 0 ] || { echo "ERROR: no runs found in $CSV" >&2; exit 1; }

if [ "${#SEL_RUNS[@]}" -gt 0 ]; then
    runs=("${SEL_RUNS[@]}")
fi

echo "============================================================================="
echo " Theta scan: ${#runs[@]} run(s) | SAVE=$SAVE | thr=$THRESHOLD e- | window=$WINDOW_NS ns"
echo "============================================================================="

# 1. Simulation + analysis (theta_scan_X.sh does both) -------------------------
if $DO_SIM; then
    echo; echo "== [1/5] SIM + ANALYSIS ($SCAN_SCRIPT) =="
    if [ "${#SEL_RUNS[@]}" -gt 0 ]; then
        SAVE="$SAVE" ./$SCAN_SCRIPT "${SEL_RUNS[@]}"
    else
        SAVE="$SAVE" ./$SCAN_SCRIPT
    fi
elif $DO_ANALYSIS; then
    echo; echo "== [1/5] ANALYSIS only (--no-sim) =="
    if [ "${#SEL_RUNS[@]}" -gt 0 ]; then
        SAVE="$SAVE" ./$SCAN_SCRIPT --no-sim "${SEL_RUNS[@]}"
    else
        SAVE="$SAVE" ./$SCAN_SCRIPT --no-sim
    fi
else
    echo; echo "== [1/5] SIM + ANALYSIS skipped (--only-extract) =="
fi

# 2. Coincidence efficiency ----------------------------------------------------
echo; echo "== [2/5] COINCIDENCE -> $RES_DIR/coin_eff.csv =="
SAVE="$SAVE" THRESHOLD="$THRESHOLD" WINDOW_NS="$WINDOW_NS" RES_DIR="$RES_DIR" \
    bash plotting_scripts/run_coin_eff.sh "${runs[@]}"

# 3. Per-plane signal efficiency vs theta --------------------------------------
echo; echo "== [3/5] SIGNAL EFFICIENCY -> $RES_DIR/theta_eff.csv =="
root -l -b -q "save_theta_eff.C(\"$SAVE\", $THRESHOLD, \"$RES_DIR\", \"$OUT_DIR\", \"$CSV\")"

# 4. Plots ---------------------------------------------------------------------
echo; echo "== [4/5] PLOTS -> $OUT_DIR/theta_eff.png / .pdf =="
root -l -b -q "plotting_scripts/ThetaEff.c(1, \"$RES_DIR\", \"$OUT_DIR\", \"$CSV\")"

# 5. Archive -------------------------------------------------------------------
echo; echo "== [5/5] ARCHIVE -> $RES_DIR / $OUT_DIR =="
mkdir -p "$RES_DIR" "$OUT_DIR"
{
    echo "# Scan finalized on $(date '+%Y-%m-%d %H:%M:%S')"
    echo "SAVE=$SAVE"; echo "THRESHOLD=$THRESHOLD"; echo "WINDOW_NS=$WINDOW_NS"
    echo "SIM_FLAGS_CFG=$SIM_FLAGS_CFG"; echo "ANALYSIS_CFG=$ANALYSIS_CFG"
} > "$RES_DIR/scan_settings.txt"
cp "$RES_DIR/scan_settings.txt" "$OUT_DIR/scan_settings.txt"
for run in "${runs[@]}"; do
    n=$(printf %04d "$run")
    [ -d "Results/local_$n" ] && { rm -rf "$RES_DIR/local_$n"; mv "Results/local_$n" "$RES_DIR/local_$n"; }
    [ -d "Plots/local_$n"   ] && { rm -rf "$OUT_DIR/local_$n";  mv "Plots/local_$n"   "$OUT_DIR/local_$n";  }
done
echo "   archived ${#runs[@]} run(s)"

echo; echo "============================================================================="
echo " Done."
echo "   per-plane efficiency : $RES_DIR/theta_eff.csv"
echo "   coincidence efficiency: $RES_DIR/coin_eff.csv"
echo "   plots                : $OUT_DIR/theta_eff.png / .pdf"
echo "============================================================================="
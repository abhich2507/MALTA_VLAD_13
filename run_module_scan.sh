#!/bin/bash
# run_module_scan.sh - one-command perpendicular module-timing scan pipeline.
#
#   1. SIM + ANALYSIS   module_scan.sh           -> Results/local_NNNN, Plots/local_NNNN
#   2. TIMING           QuadModule_time_plane0.C -> $OUT_DIR/ModuleTiming_plane0_runNNNN.png/pdf
#   3. OVERLAY          ModuleTiming_overlay.C    -> $OUT_DIR/ModuleTiming_all4.png/pdf
#   4. ARCHIVE          move per-run outputs     -> $RES_DIR, $OUT_DIR
#
# Usage:
#   ./run_module_scan.sh               full chain (all 4 sensors)
#   ./run_module_scan.sh 502           single run (S1)
#   ./run_module_scan.sh --no-sim      skip simulation (analysis + timing + archive)
#   ./run_module_scan.sh --only-extract  only timing + overlay from archived runs
#
# Run list comes from configs/module_scan.csv (run,sensor,x_center_cm).
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# ---- configuration (single place to edit) -----------------------------------
SCAN_SCRIPT="module_scan.sh"
ANALYSIS_CFG="analysis_flags_MP_EIC_224x512.cfg"
SAVE="analysis_results_224x512_module"
THRESHOLD=100
CSV="$SCRIPT_DIR/configs/module_scan.csv"
RES_DIR="Results_module_pencil"                             # <-- set your results dir here
OUT_DIR="Plots_module_pencil"                               # <-- set your plots dir here
# ------------------------------------------------------------------------------

DO_SIM=true
DO_ANALYSIS=true
SEL_RUNS=()

for arg in "$@"; do
    case "$arg" in
        --no-sim)        DO_SIM=false ;;
        --only-extract)  DO_SIM=false; DO_ANALYSIS=false ;;
        -h|--help)       sed -n '2,17p' "$0"; exit 0 ;;
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
echo " Module scan: ${#runs[@]} run(s) | SAVE=$SAVE | thr=$THRESHOLD e-"
echo "============================================================================="

# 1. Simulation + analysis -----------------------------------------------------
if $DO_SIM; then
    echo; echo "== [1/4] SIM + ANALYSIS ($SCAN_SCRIPT) =="
    if [ "${#SEL_RUNS[@]}" -gt 0 ]; then
        SAVE="$SAVE" ./$SCAN_SCRIPT "${SEL_RUNS[@]}"
    else
        SAVE="$SAVE" ./$SCAN_SCRIPT
    fi
elif $DO_ANALYSIS; then
    echo; echo "== [1/4] ANALYSIS only (--no-sim) =="
    if [ "${#SEL_RUNS[@]}" -gt 0 ]; then
        SAVE="$SAVE" ./$SCAN_SCRIPT --no-sim "${SEL_RUNS[@]}"
    else
        SAVE="$SAVE" ./$SCAN_SCRIPT --no-sim
    fi
else
    echo; echo "== [1/4] SIM + ANALYSIS skipped (--only-extract) =="
fi

# 2. Per-run module timing -----------------------------------------------------
echo; echo "== [2/4] PER-RUN MODULE TIMING -> $OUT_DIR/ =="
for run in "${runs[@]}"; do
    echo "----- run $run -----"
    root -l -b -q "plotting_scripts/QuadModule_time_plane0.C($run, \"$RES_DIR\", \"$SAVE\", $THRESHOLD, \"$OUT_DIR\")"
done

# 3. Combined overlay of all 4 sensors ------------------------------------------
echo; echo "== [3/4] OVERLAY all 4 sensors -> $OUT_DIR/ModuleTiming_all4.png/pdf =="
root -l -b -q "plotting_scripts/ModuleTiming_overlay.C(\"$RES_DIR\", \"$SAVE\", $THRESHOLD, \"$OUT_DIR\", \"$CSV\")"

# 4. Archive -------------------------------------------------------------------
echo; echo "== [4/4] ARCHIVE -> $RES_DIR / $OUT_DIR =="
mkdir -p "$RES_DIR" "$OUT_DIR"
{
    echo "# Scan finalized on $(date '+%Y-%m-%d %H:%M:%S')"
    echo "SAVE=$SAVE"; echo "THRESHOLD=$THRESHOLD"
    echo "ANALYSIS_CFG=$ANALYSIS_CFG"
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
echo "   per-run timing : $OUT_DIR/ModuleTiming_plane0_runNNNN.png/pdf"
echo "   all-4 overlay  : $OUT_DIR/ModuleTiming_all4.png/pdf"
echo "   per-run outputs: $RES_DIR/local_NNNN, $OUT_DIR/local_NNNN"
echo "============================================================================="

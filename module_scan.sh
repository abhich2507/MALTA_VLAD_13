#!/bin/bash
# module_scan.sh - perpendicular-beam scan over the 4 sensor columns.
# The beam (theta = 0, normal incidence) is offset to the centre of each
# sensor S0..S3 on plane 0; the same column index is hit on plane 1
# (S0->S100, S1->S101, ...). One run per sensor => per-sensor timing.
#
# For each (run, sensor, x_center_cm) in configs/module_scan.csv:
#   [SIM] beamXOffset = x_center_cm, runDirNumber = run -> local Geant4 sim
#   [ANA] run_mult.sh per run (digitize + track + cluster + analyze)
#
# Usage:
#   ./module_scan.sh               # all 4 runs
#   ./module_scan.sh 502           # single run (S1)
#   ./module_scan.sh --no-sim      # skip simulation (Results/local_NNNN exist)
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

# ---- configuration -----------------------------------------------------------
SIM_FLAGS_CFG="flags_MP_EIC_224x512_module.cfg"
ANALYSIS_CFG="analysis_flags_MP_EIC_224x512.cfg"
SAVE="${SAVE:-analysis_results_224x512_module}"
THRESHOLD=100
CSV="$SCRIPT_DIR/configs/module_scan.csv"
FLAGS="$SCRIPT_DIR/configs/$SIM_FLAGS_CFG"
BUILD_DIR="$SCRIPT_DIR/build"
# ------------------------------------------------------------------------------

DO_SIM=true
SEL_RUNS=()
for arg in "$@"; do
    case "$arg" in
        --no-sim) DO_SIM=false ;;
        -h|--help) sed -n '2,16p' "$0"; exit 0 ;;
        *) if [[ "$arg" =~ ^[0-9]+$ ]]; then SEL_RUNS+=("$arg"); else
               echo "Unknown option: $arg" >&2; exit 1; fi ;;
    esac
done

[ -f "$CSV" ]  || { echo "Missing $CSV" >&2; exit 1; }
[ -f "$FLAGS" ] || { echo "Missing $FLAGS" >&2; exit 1; }

runs=()
while IFS=',' read -r run sensor xc || [ -n "$run" ]; do
    case "$run" in ''|run) continue ;; esac
    if [ "${#SEL_RUNS[@]}" -gt 0 ]; then
        for s in "${SEL_RUNS[@]}"; do
            [ "$s" = "$run" ] && runs+=("$run") && break
        done
    else
        runs+=("$run")
    fi
done < "$CSV"
[ "${#runs[@]}" -gt 0 ] || { echo "ERROR: no selected runs found in $CSV" >&2; exit 1; }

in_runs() { local r="$1" s; for s in "${runs[@]}"; do [ "$s" = "$r" ] && return 0; done; return 1; }

echo "============================================================================="
echo " Module scan: ${#runs[@]} run(s): ${runs[*]} | SAVE=$SAVE | thr=$THRESHOLD e-"
echo "============================================================================="

# 1. Simulation ----------------------------------------------------------------
if $DO_SIM; then
    cp "$FLAGS" "$FLAGS.bak"
    trap 'mv "$FLAGS.bak" "$FLAGS"; echo "Restored $FLAGS"' EXIT
    cd "$BUILD_DIR"
    while IFS=',' read -r run sensor xc || [ -n "$run" ]; do
        case "$run" in ''|run) continue ;; esac
        in_runs "$run" || continue

        echo "== [1/2 SIM] run=$run sensor=$sensor beamXOffset=$xc cm (perpendicular)"

        sed -i '' "s/^beamXOffset = .*/beamXOffset = $xc/" "$FLAGS"
        sed -i '' "s/^runDirNumber = .*/runDirNumber = $run/" "$FLAGS"
        grep -q "^beamXOffset = $xc$" "$FLAGS" || { echo "FAILED to set beamXOffset=$xc" >&2; exit 1; }
        source run_script_local.sh "$SIM_FLAGS_CFG" "$run"
    done < <(tail -n +2 "$CSV")
    cd "$SCRIPT_DIR"
else
    echo "== [1/2 SIM] skipped (--no-sim)"
fi

# 2. Analysis ------------------------------------------------------------------
echo "== [2/2 ANA] digitize + track + cluster + analyze =="
for run in "${runs[@]}"; do
    echo "----- run $run -----"
    SAVE="$SAVE" CONFIG="$ANALYSIS_CFG" THRESHOLD="$THRESHOLD" ./run_mult.sh "$run"
done

echo " Done. Timing: root -l -b -q 'plotting_scripts/QuadModule_time_plane0.C(\$run, \"\$RES_DIR\", \"\$SAVE\", 100, \"\$OUT_DIR\")'"

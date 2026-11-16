#!/bin/bash
# theta_scan.sh - oblique-beam angle scan (Y tilt) + per-run analysis.
# For each (run, theta_deg) in configs/theta_scan.csv:
#   [SIM] particleMomentumY = tan(theta), beamYOffset = -105*tan(theta) cm,
#         runDirNumber = run  -> run local Geant4 sim
#   [ANA] run_mult.sh per run (digitize + track + cluster + analyze)
# Residual extraction is a separate step (plotting_scripts/thetaResiduals.C).
#
# Usage:
#   ./theta_scan.sh               # all runs in the CSV
#   ./theta_scan.sh 303           # single run (theta = 1 deg)
#   ./theta_scan.sh 302 303 304   # selected runs
#   ./theta_scan.sh --no-sim      # skip simulation (Results/local_NNNN exist)
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

# ---- configuration -----------------------------------------------------------
SIM_FLAGS_CFG="flags_MP_EIC_224x512_theta.cfg"
ANALYSIS_CFG="analysis_flags_MP_EIC_224x512.cfg"
SAVE="analysis_results_224x512_theta"
THRESHOLD=100
CSV="$SCRIPT_DIR/configs/theta_scan.csv"
FLAGS="$SCRIPT_DIR/configs/$SIM_FLAGS_CFG"
BUILD_DIR="$SCRIPT_DIR/build"
# ------------------------------------------------------------------------------

DO_SIM=true
SEL_RUNS=()
for arg in "$@"; do
    case "$arg" in
        --no-sim) DO_SIM=false ;;
        -h|--help) sed -n '2,17p' "$0"; exit 0 ;;
        *) if [[ "$arg" =~ ^[0-9]+$ ]]; then SEL_RUNS+=("$arg"); else
               echo "Unknown option: $arg" >&2; exit 1; fi ;;
    esac
done

[ -f "$CSV" ]  || { echo "Missing $CSV" >&2; exit 1; }
[ -f "$FLAGS" ] || { echo "Missing $FLAGS" >&2; exit 1; }

# Selected runs (CSV order preserved)
runs=()
while IFS=',' read -r run theta || [ -n "$run" ]; do
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
echo " Theta scan: ${#runs[@]} run(s): ${runs[*]} | SAVE=$SAVE | thr=$THRESHOLD e-"
echo "============================================================================="

# 1. Simulation ----------------------------------------------------------------
if $DO_SIM; then
    cp "$FLAGS" "$FLAGS.bak"
    trap 'mv "$FLAGS.bak" "$FLAGS"; echo "Restored $FLAGS"' EXIT
    cd "$BUILD_DIR"
    while IFS=',' read -r run theta || [ -n "$run" ]; do
        case "$run" in ''|run) continue ;; esac
        in_runs "$run" || continue

        TAN=$(python3 -c "import math; print(math.tan(math.radians($theta)))")
        YOFF=$(python3 -c "import math; print(-105.0*math.tan(math.radians($theta)))")
        echo "== [1/2 SIM] run=$run theta=$theta deg  tan=$TAN  beamYOffset=$YOFF cm"

        sed -i '' "s/^particleMomentumY = .*/particleMomentumY = $TAN/" "$FLAGS"
        sed -i '' "s/^beamYOffset = .*/beamYOffset = $YOFF/" "$FLAGS"
        sed -i '' "s/^runDirNumber = .*/runDirNumber = $run/" "$FLAGS"
        grep -q "^particleMomentumY = $TAN$" "$FLAGS" || { echo "FAILED to set particleMomentumY=$TAN" >&2; exit 1; }

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

echo "============================================================================="
echo " Done. Residuals: root -l -b -q 'plotting_scripts/thetaResiduals.C(\"$SAVE\",\"configs/theta_scan.csv\",\"Plots_thetaScan\")'"
echo "============================================================================="
#!/bin/bash
# Scan particleMomentum in flags_MP_EIC_momentum.cfg over the (run, momentum) values in
# configs/momentum_scan.csv, at a FIXED EIC background rate (bkgRateMean = 1.584).
# Each scan point launches the local Geant4 simulation via run_script_local.sh, and
# Results/local_NNNN directories are written to the run number given in the CSV.
#
# This script is fully independent of bkg_scan.sh / run_efficiency.sh: it edits
# configs/flags_MP_EIC_momentum.cfg (NOT flags_MP_EIC.cfg) and reads
# configs/momentum_scan.csv (NOT bkg_rate.csv).
#
# Usage:
#   ./momentum_scan.sh              # all points in configs/momentum_scan.csv
#   ./momentum_scan.sh 102          # only run 102
#   ./momentum_scan.sh 101 103 105  # runs 101, 103 and 105
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
CSV="$SCRIPT_DIR/configs/momentum_scan.csv"
FLAGS="$SCRIPT_DIR/configs/flags_MP_EIC_momentum.cfg"
BUILD_DIR="$SCRIPT_DIR/build"
EIC_BKG_RATE=1.584   # fixed EIC background rate (0.198 hits/sensor/event x 8 sensors)

[ -f "$CSV" ]  || { echo "Missing $CSV"; exit 1; }
[ -f "$FLAGS" ] || { echo "Missing $FLAGS"; exit 1; }

# --- Optional run selection ----------------------------------------------------
# SEL_RUNS is a space-separated list of run numbers to process; empty = all runs.
SEL_RUNS=""
for arg in "$@"; do
    IFS=',' read -ra parts <<< "$arg"
    for p in "${parts[@]}"; do
        [ -z "$p" ] && continue
        if [[ "$p" == *-* ]]; then
            start="${p%%-*}"
            end="${p##*-}"
            if [ "$start" -gt "$end" ] 2>/dev/null; then
                tmp="$start"; start="$end"; end="$tmp"
            fi
            for ((r=start; r<=end; r++)); do
                SEL_RUNS="$SEL_RUNS $r"
            done
        else
            SEL_RUNS="$SEL_RUNS $p"
        fi
    done
done

is_selected() {
    local run="$1"
    local s
    for s in $SEL_RUNS; do
        [ "$s" = "$run" ] && return 0
    done
    return 1
}
# ------------------------------------------------------------------------------

# Keep a backup of the momentum flags and restore them no matter how the scan ends
cp "$FLAGS" "$FLAGS.bak"
trap 'mv "$FLAGS.bak" "$FLAGS"; echo "Restored $FLAGS"' EXIT

cd "$BUILD_DIR"

# Skip the header line; process substitution keeps the loop in this shell
while IFS=',' read -r run momentum || [ -n "$run" ]; do
    [ -z "$run" ] && continue

    # Skip runs that were not selected (only when a selection was provided)
    if [ -n "$SEL_RUNS" ] && ! is_selected "$run"; then
        echo "Skipping run=$run (not selected)"
        continue
    fi

    echo "=================================================="
    echo "Scan point: run=$run  particleMomentum=$momentum GeV/c (bkgRateMean=$EIC_BKG_RATE fixed)"

    sed -i '' "s/^particleMomentum = .*/particleMomentum = $momentum/" "$FLAGS"
    grep -q "^particleMomentum = $momentum$" "$FLAGS" || { echo "Failed to set particleMomentum=$momentum in $FLAGS"; exit 1; }

    sed -i '' "s/^bkgRateMean = .*/bkgRateMean = $EIC_BKG_RATE/" "$FLAGS"
    grep -q "^bkgRateMean = $EIC_BKG_RATE$" "$FLAGS" || { echo "Failed to set bkgRateMean=$EIC_BKG_RATE in $FLAGS"; exit 1; }

    # Force the sim to write into Results/local_%04d matching this run number
    if grep -q "^runDirNumber = " "$FLAGS"; then
        sed -i '' "s/^runDirNumber = .*/runDirNumber = $run/" "$FLAGS"
    else
        sed -i '' "/^outputPathNAF = /a\\
runDirNumber = $run" "$FLAGS"
    fi
    grep -q "^runDirNumber = $run$" "$FLAGS" || { echo "Failed to set runDirNumber=$run in $FLAGS"; exit 1; }

    source run_script_local.sh flags_MP_EIC_momentum.cfg "$run"
done < <(tail -n +2 "$CSV")

echo "=================================================="
echo "Momentum scan complete."

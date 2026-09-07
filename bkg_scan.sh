#!/bin/bash
# Scan particleCount in flags_MP_EIC.cfg over selected (run, count) values in configs/bkg_count.csv.
# Each scan point launches the local Geant4 simulation via run_script_local.sh,
# and Results/local_NNNN directories increment automatically per run.
#
# Usage:
#   ./bkg_scan.sh              # all runs in configs/bkg_count.csv
#   ./bkg_scan.sh 2            # only run 2
#   ./bkg_scan.sh 2 4 6        # runs 2, 4 and 6
#   ./bkg_scan.sh 2-5          # runs 2 through 5
#   ./bkg_scan.sh 1,3-5        # runs 1 and 3..5 (comma- and/or space-separated)
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
CSV="$SCRIPT_DIR/configs/bkg_count.csv"
FLAGS="$SCRIPT_DIR/configs/flags_MP_EIC.cfg"
BUILD_DIR="$SCRIPT_DIR/build"

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
        if [ "$s" = "$run" ]; then
            return 0
        fi
    done
    return 1
}
# ------------------------------------------------------------------------------

# Keep a backup of the original flags and restore it no matter how the scan ends
cp "$FLAGS" "$FLAGS.bak"
trap 'mv "$FLAGS.bak" "$FLAGS"; echo "Restored $FLAGS"' EXIT

cd "$BUILD_DIR"

# Skip the header line; process substitution keeps the loop in this shell
while IFS=',' read -r run count; do
    [ -z "$run" ] && continue

    # Skip runs that were not selected (only when a selection was provided)
    if [ -n "$SEL_RUNS" ] && ! is_selected "$run"; then
        echo "Skipping run=$run (not selected)"
        continue
    fi

    echo "=================================================="
    echo "Scan point: run=$run  particleCount=$count"

    sed -i '' "s/^particleCount = .*/particleCount = $count/" "$FLAGS"
    grep -q "^particleCount = $count$" "$FLAGS" || { echo "Failed to set particleCount=$count in $FLAGS"; exit 1; }

    source run_script_local.sh flags_MP_EIC.cfg
done < <(tail -n +2 "$CSV")

echo "=================================================="
echo "Scan complete."
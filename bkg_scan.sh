#!/bin/bash
# Scan particleCount in flags_MP_EIC.cfg over the (run, count) values in configs/bkg_count.csv.
# Each scan point launches the local Geant4 simulation via run_script_local.sh,
# and Results/local_NNNN directories increment automatically per run.
# Usage: ./bkg_scan.sh
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
CSV="$SCRIPT_DIR/configs/bkg_count.csv"
FLAGS="$SCRIPT_DIR/configs/flags_MP_EIC.cfg"
BUILD_DIR="$SCRIPT_DIR/build"

[ -f "$CSV" ]  || { echo "Missing $CSV"; exit 1; }
[ -f "$FLAGS" ] || { echo "Missing $FLAGS"; exit 1; }

# Keep a backup of the original flags and restore it no matter how the scan ends
cp "$FLAGS" "$FLAGS.bak"
trap 'mv "$FLAGS.bak" "$FLAGS"; echo "Restored $FLAGS"' EXIT

cd "$BUILD_DIR"

# Skip the header line; process substitution keeps the loop in this shell
while IFS=',' read -r run count; do
    [ -z "$run" ] && continue

    echo "=================================================="
    echo "Scan point: run=$run  particleCount=$count"

    sed -i '' "s/^particleCount = .*/particleCount = $count/" "$FLAGS"
    grep -q "^particleCount = $count$" "$FLAGS" || { echo "Failed to set particleCount=$count in $FLAGS"; exit 1; }

    source run_script_local.sh flags_MP_EIC.cfg
done < <(tail -n +2 "$CSV")

echo "=================================================="
echo "Scan complete."

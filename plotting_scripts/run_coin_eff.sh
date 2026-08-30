#!/bin/bash
# run_coin_eff.sh
# Drive analysis_multiPlane/src/Coincidence.cc over a list of run numbers and
# write the coincidence efficiency results to Results/coin_eff.csv.
#
# CSV columns: run, window_ns, nGen, coinCount, eff_percent
#   nGen      : generated signal tracks (mcFlag==0 rows in plane Z0)
#   coinCount : tracks with clSize>0 in both planes AND |ctZ0-ctZ1| <= window
#   eff       : 100 * coinCount / nGen
#
# Usage (run from malta_simulation/ or anywhere):
#   ./plotting_scripts/run_coin_eff.sh 1 2 3     # specific runs
#   ./plotting_scripts/run_coin_eff.sh           # all runs in configs/bkg_count.csv
#
# Note: the file Results/coin_eff.csv is rewritten by each invocation.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BASE_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$BASE_DIR"

COIN_CC="analysis_multiPlane/src/Coincidence.cc"
OUT_CSV="Results/coin_eff.csv"
WINDOW_NS=8          # must match coincidenceWindow in Coincidence.cc

runs=("$@")
if [ "${#runs[@]}" -eq 0 ]; then
    # fallback: take the run column of configs/bkg_count.csv
    while IFS=',' read -r run rest; do
        case "$run" in ''|'run') continue ;; esac
        if [[ "$run" =~ ^[0-9]+$ ]]; then runs+=("$run"); fi
    done < configs/bkg_count.csv
fi
if [ "${#runs[@]}" -eq 0 ]; then
    echo "ERROR: no run numbers given and configs/bkg_count.csv has none" >&2
    exit 1
fi

mkdir -p Results
echo "run,window_ns,nGen,coinCount,eff_percent" > "$OUT_CSV"

for run in "${runs[@]}"; do
    echo "== processing run $run =="
    out="$(root -l -b -q "${COIN_CC}(${run})" 2>&1 || true)"

    coin="$(printf '%s\n' "$out" | sed -n 's/.*Total coincidences found: *\([0-9]*\).*/\1/p' | tail -1)"
    ngen="$(printf '%s\n' "$out" | sed -n 's/.*Signal generated tracks: *\([0-9]*\).*/\1/p' | tail -1)"
    eff="$(printf '%s\n' "$out"  | sed -n 's/.*Coincidence efficiency: *\([0-9.]*\).*/\1/p' | tail -1)"

    if [ -z "${coin}" ] || [ -z "${ngen}" ] || [ -z "${eff}" ]; then
        echo "WARNING: could not parse run $run output; skipping" >&2
        printf '%s\n' "$out" | tail -25 >&2
        continue
    fi

    echo "$run,$WINDOW_NS,$ngen,$coin,$eff" >> "$OUT_CSV"
    echo "   nGen=$ngen coin=$coin eff=${eff}%"
done

echo "done. wrote $OUT_CSV"

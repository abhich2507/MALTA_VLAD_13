#!/bin/bash
# Wrapper around run_analysis_multiPlane.py
# Usage: ./run_mult.sh <startRun> [endRun]   (also accepts a range like 1-12)
# Override analysis settings via env: SAVE, CONFIG, THRESHOLD
set -e

SAVE="${SAVE:-analysis_results_MP}"
CONFIG="${CONFIG:-analysis_flags_MP_EIC_Vlad.cfg}"
THRESHOLD="${THRESHOLD:-100}"

START="${1:?Usage: $0 <startRun> [endRun]}"
END="${2:-}"

if [ -n "$END" ]; then
    RUNS="$START-$END"
else
    RUNS="$START"
fi

cd "$(dirname "$0")/"

python run_analysis_multiPlane.py \
    -s "$SAVE" \
    -r "$RUNS" \
    -i "$CONFIG" \
    -thr "$THRESHOLD" \
    -d -t -c -a
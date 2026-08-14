#!/bin/bash
# Wrapper around run_analysis_multiPlane.py
# Usage: ./run_mult.sh <runNumber>
set -e

SAVE="analysis_results_MP"
CONFIG="analysis_flags_MP.cfg"
THRESHOLD="100"

RUN="${1:?Usage: $0 <runNumber>}"

cd "$(dirname "$0")/"

python run_analysis_multiPlane.py \
    -s "$SAVE" \
    -r "$RUN" \
    -i "$CONFIG" \
    -thr "$THRESHOLD" \
    -d -t -c -a
#!/bin/bash
source ../config.sh
set -e

source "$LOCAL_GEANT"
source "$LOCAL_ROOT"
export $EXTRA_LOCAL

export SIMU_CONFIG=$LOCAL_PATH/flags.cfg
cmake .. 2>&1 | tee -a sim_preflight.log
make 2>&1 | tee -a sim_preflight.log
$LOCAL_PATH/build/sim --test > sim_preflight.log 2>&1 
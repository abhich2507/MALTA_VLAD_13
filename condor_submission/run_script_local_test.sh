#!/bin/bash
source ../config.sh

source "$LOCAL_GEANT"
source "$LOCAL_ROOT"
export $EXTRA_LOCAL

export SIMU_CONFIG=$LOCAL_PATH/flags.cfg
cmake -DEXTRA_LIBS=uuid .. > sim_preflight.log 2>&1 
make > sim_preflight.log 2>&1 
$LOCAL_PATH/build/sim --test > sim_preflight.log 2>&1 
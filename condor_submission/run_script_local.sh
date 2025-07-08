#!/bin/bash
source ../config.sh

source "$LOCAL_GEANT"
source "$LOCAL_ROOT"
export $EXTRA_LOCAL

export SIMU_CONFIG=$LOCAL_PATH/flags.cfg
cmake .. 2>&1 | tee -a sim.log
make 2>&1 | tee -a sim.log
$LOCAL_PATH/build/sim 2>&1 | tee -a sim.log
latest_dir=$(ls -d ../Results/local_* | sort -V | tail -n 1)
mv sim.log "$latest_dir/"
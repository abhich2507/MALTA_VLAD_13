#!/bin/bash
source ../config.sh

if [ $HOME = "/home/vlad" ]; then
    source "$LOCAL_GEANT"
    source "$LOCAL_ROOT"
    export $EXTRA_LOCAL
fi
FLAG=$1

export SIMU_CONFIG=$LOCAL_PATH/configs/$FLAG
if [ $HOME = "/home/vlad" ]; then 
    cmake -DEXTRA_LIBS=uuid .. 2>&1 | tee -a sim_preflight.log

else 
    cmake .. 2>&1 | tee -a sim_preflight.log 
fi
make > sim_preflight.log 2>&1 
$LOCAL_PATH/build/sim --test > sim_preflight.log 2>&1 

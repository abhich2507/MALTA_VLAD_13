#!/bin/bash

if [ $HOME = "/home/vlad" ]; then
    source ../config.sh
    source "$LOCAL_GEANT"
    source "$LOCAL_ROOT"
    export $EXTRA_LOCAL
else
    source ../config_lucian.sh
fi
FLAG=$1

export SIMU_CONFIG=$LOCAL_PATH/configs/$FLAG
if [ $HOME = "/home/vlad" ]; then 
    cmake -DEXTRA_LIBS=uuid .. 2>&1 | tee -a sim.log 

else 
    cmake .. 2>&1 | tee -a sim.log 
fi

make 2>&1 | tee -a sim.log
$LOCAL_PATH/build/sim 2>&1 | tee -a sim.log
latest_dir=$(ls -d ../Results/local_* | sort -V | tail -n 1)
mv sim.log "$latest_dir/"

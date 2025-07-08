#!/bin/bash

source /afs/desy.de/user/b/berleavl/private/decal_sw_cern/config.sh
set -e

echo "NAF_USER=$NAF_USER"
echo "NAF_PATH=$NAF_PATH"
echo "NAF_GEANT=$NAF_GEANT"

echo "SIM_PATH=/afs/desy.de/user/${NAF_USER:0:1}/$NAF_USER/$NAF_PATH/build/sim"
source "$NAF_GEANT"
export SIMU_CONFIG=/afs/desy.de/user/${NAF_USER:0:1}/$NAF_USER/$NAF_PATH/flags.cfg
/afs/desy.de/user/${NAF_USER:0:1}/$NAF_USER/$NAF_PATH/build/sim


#source /afs/desy.de/user/b/berleavl/private/geant4-install/bin/geant4.sh
#export NUM_CORES=2
#export SIMU_CONFIG=/afs/desy.de/user/b/berleavl/private/decal_sw_cern/flags.cfg
#/afs/desy.de/user/b/berleavl/private/decal_sw_cern/build/sim 

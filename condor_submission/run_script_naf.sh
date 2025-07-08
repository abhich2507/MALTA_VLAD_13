#!/bin/bash

source config.sh
source "$NAF_GEANT"
export SIMU_CONFIG=/afs/desy.de/user/${NAF_USER:0:1}/$NAF_USER/$NAF_PATH/configs/__FLAG__
/afs/desy.de/user/${NAF_USER:0:1}/$NAF_USER/$NAF_PATH/build/sim

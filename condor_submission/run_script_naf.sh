source /afs/desy.de/user/b/berleavl/private/decal_sw_cern/config.sh

source "$NAF_GEANT"
#export NUM_CORES=2
export SIMU_CONFIG=/afs/desy.de/user/${NAF_USER:0:1}/$NAF_USER/$NAF_PATH/flags.cfg
/afs/desy.de/user/${NAF_USER:0:1}/$NAF_USER/$NAF_PATH/build/sim

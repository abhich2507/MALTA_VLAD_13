source /afs/desy.de/user/b/berleavl/private/geant4-install/bin/geant4.sh
#export NUM_CORES=2
export SIMU_CONFIG=/afs/desy.de/user/b/berleavl/private/decal_sw_cern/flags.cfg
/afs/desy.de/user/b/berleavl/private/decal_sw_cern/build/sim 

latest_dir=$(ls -d /afs/desy.de/user/b/berleavl/private/decal_sw_cern/Results/naf_* | sort -V | tail -n 1)
for f in /afs/desy.de/user/b/berleavl/private/decal_sw_cern/condor_log/job.err \
         /afs/desy.de/user/b/berleavl/private/decal_sw_cern/condor_log/job.log \
         /afs/desy.de/user/b/berleavl/private/decal_sw_cern/condor_log/job.out
do
    [ -f "$f" ] && mv "$f" "$latest_dir/"
done
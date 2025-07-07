source /home/vlad/Documents/Geant4/geant4-v11.3.0/install/bin/geant4.sh
source /home/vlad/Documents/root/root-install/bin/thisroot.sh
LD_LIBRARY_PATH=/usr/lib/x86_64-linux-gnu

export SIMU_CONFIG=/home/vlad/Documents/Simu/Geant4/DECAL_REPO/flags.cfg
cmake .. 2>&1 | tee -a sim.log
make 2>&1 | tee -a sim.log
/home/vlad/Documents/Simu/Geant4/DECAL_REPO/build/sim 2>&1 | tee -a sim.log
latest_dir=$(ls -d ../Results/local_* | sort -V | tail -n 1)
mv sim.log "$latest_dir/"
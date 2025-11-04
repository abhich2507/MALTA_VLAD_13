#!/bin/bash
source /cvmfs/atlas.cern.ch/repo/sw/tdaq/tdaq-common/tdaq-common-12-00-00/installed/share/cmake_tdaq/bin/setup.sh
source /cvmfs/atlas.cern.ch/repo/sw/tdaq/tdaq/tdaq-12-00-00/installed/setup.sh
#setup Root
export PATH=$ROOTSYS/bin:$PATH
export LD_LIBRARY_PATH=$ROOTSYS/lib:$LD_LIBRARY_PATH

#setup GEANT4
source /cvmfs/sft.cern.ch/lcg/releases/Geant4/11.1.2-8a022/x86_64-el9-gcc11-opt/bin/geant4.sh
# --------------------------------------------------------------
# Geant4 / ROOT build dependencies
# --------------------------------------------------------------

# CLHEP
export CLHEP_DIR=/cvmfs/sft.cern.ch/lcg/releases/clhep/2.4.6.4-2ef70/x86_64-el9-gcc11-opt/lib/CLHEP-2.4.6.4

# XercesC
export XercesC_LIBRARY=/cvmfs/sft.cern.ch/lcg/releases/XercesC/3.2.4-9e637/x86_64-el9-gcc11-opt/lib/libxerces-c.so
export XercesC_INCLUDE_DIR=/cvmfs/sft.cern.ch/lcg/releases/XercesC/3.2.4-9e637/x86_64-el9-gcc11-opt/include

# Qt5
export CMAKE_PREFIX_PATH=/cvmfs/sft.cern.ch/lcg/releases/qt5/5.15.9-50dd0/x86_64-el9-gcc11-opt/lib/cmake

#Inject arguments in CMAKE
export CMAKE_ARGS="-DCLHEP_DIR=$CLHEP_DIR \
-DXercesC_LIBRARY=$XercesC_LIBRARY \
-DXercesC_INCLUDE_DIR=$XercesC_INCLUDE_DIR \
-DCMAKE_PREFIX_PATH=$CMAKE_PREFIX_PATH"

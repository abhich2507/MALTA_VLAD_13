[[_TOC_]]

# MALTA_SIMULATION
The MALTA2 sensor is the second full scale implementation of the MALTA sensor family. An accurate simulation of the sensor performance in a test beam environment is desired for both test beam data validation but also future sensor redesign work. One such activity is the use of a MALTA-like sensor for digital calorimetry applications.

## Description
This repository contains a pure GEANT4 simulation of a DMAPS sensor, MALTA2. An accurate charge collection and charge sharing model is implemented via a parametrization of the charge collection model gained via digital scans of MALTA2 Epi samples in the SPS test beam. These measurements have been separately checked with an Edge Transient Current Technique (E-TCT) measurements. Compatible depletion depth measurements have been gained via both measurements.

The project is built with the help of a Cmake file: CMakeLists.txt. The simulation is divided into several directories described below: 

- /analysis. Directory contains all the C++ ROOT scripts for data processing, reconstruction and analysis
- /build. Directory contains all the built files via CMake
- /configs. Directory contains all the configuration files used in the simultaion and analysis.
- /include. GEANT4 header files
- /macros. GEANT4 run and vis macros. They are called via config flags
- /plotting_scripts. Directory contains all plotting scripts which can be used to quickly generate plots
- /run_files. Directory contains all run files needed to run both locally and on the cloud. Includes CONDOR job submission templating
- /src. GEANT4 source files

# Local Installation

```bash
# Clone Repository
git clone https://gitlab.cern.ch/dberlea/malta_simulation.git
cd malta_simulation
# Setup environment
source setup.sh
# Create the build directory
mkdir build 
cd build
# Build the project
cmake $CMAKE_ARGS ..
make
```

# Quick usage

## GEANT4 Usage
```bash
cd build/
source ../run_sim.sh local flag.cfg
```
## Analysis Usage
```bash
cd malta_simulation/
python run_analysis.py -r runNumber -i analysis_flags.cfg -thr thrValue -s saveName -d digitizer Flag -t Tracking flag -c Clustering flag -a Analysis flag
```

# GEANT4 Simulation structure
The GEANT4 simulation is structured based on the usual file format. The functionality of each class is described further below:

## ActionInitialization
This class inherit the GEANT4 class G4VUserActionInitialization. It is used to to intialize all the actions at the different GEANT4 levels in the package. The following actions are currently defined in the package: PrimaryGenerator, RunAction, EventAction, SteppingAction, TrackingAction. All initializations are done at the thread level, except for the run action which is additionally defined at the master thread level.

## Config
This class implements all the methods for importing and saving configurations and also creating run directories and handling path managements

    - DumpConfigToFile() copies the used flag file to the run directory
    - CreateNextRunDirectory() creates the next run directory based on the last value of the incremented local or naf directory structure
    - LoadSimFlagFromFile() loads all the simulation configuration flags
    - GetRequestCpusFromSubmitFile() Used for checking the number of requested CPUs is comaptible with the number of GEANT4 threads for remote job submission
    - trowWarning() Formatted warning trowing
    - trowError() Formatted error trowing
Detailed explanation of the simulation flag functionality is described in another subsection below. [TODO: Link?]

## CorrectionData2D
[Lucian Please fill]

## Create2DEffMap
[Lucian Please fill]

## DetectorConstruction
This class inherits the GEANT4 class G4VUserDetectorConstruction. It is used to define the solid, logic and physical volumes associated to the elements of the simulation. The aim of this class is to create an accurate simulation of the real MALTA2 sensor and fixtures but at the same time keep the geometry of the active detector simple enough in order to keep the computation times low.

### World Volume
The world volume is simulated as vacuum, defined with the help of the pre-defined nist G4_Galactic. This is in order to limit the particle interactions outside of the sensitive volume and further optimize the computation time. The world volume is designed as a G4Box of size 2 *m x 2 *m x 2 *m, centered at the origin. Any further geometry implementation needs to respect the following X, Y, Z boundaries: [-100 *m, 100 *cm]. This boundary check is enforced via a FATAL ERROR handling.

### Sensor Volume
The sensor volume is simulated as a monolithic Silicon block using the pre-defined nist G4_Si. Concerning the sensitive material, this simulation does not take into account the following elements:

- the non-sensitive SiO2 layer on top of the Si bulk
- the Cu metal layers above the PMD 
- the metal for the transistor gates of the N, P -MOSFETs
- the dopants and geometry of surface or deep implants
- any Si wafer post-processing procedure e.g. back metallization, conductive glue, etc.

The solid volume is defined as a G4Box with customizable values. Default values are implemented in the sample configs/flags.cfg file.

Currently 7 MALTA planes are implemted where the z positions were taken from the .toml MALTA analysis files with the following planeID naming:
    - DUT (center) planeID 0
    - first 3 tracking planes planeID 1-3
    - last 3 tracking planes planeID 4 -6

### PCB Volume
The Printed Circuit Board (PCB) to which the MALTA samples are glued to is simulated. Currently, the PCB implementation is implemented separatelly to the sensor simulation, allowing a switch between the 2 via a flag. A unitary implementation might be desired in the future, however the PCB stack is subject to change between diferent iterations.

The PCB is implemented as Copper and dielectric planes with the help of the edms files of the E-TCT PCB. Two materials are used in the simulation. The pre-defined nist G4_Cu and a custom defined material: FR4. The FR4 material is defined as a mixture of O, C, Si, H, Na, B. The component percentage of each element was computed taking into account the atomic formula of a typical fire retardant PCB material found at reference: https://www.physics.smu.edu/web/research/preprints/SMU-HEP-08-11.pdf and the molar mass of each element. All Copper and FR4 planes are defined as rectangular 12.7 x 12.7 cm^2 but with various thicknesses as defined in the design file. (TODO: The current geometrical implementation does not match the exact physical dimensions of the board. For a more accurate implementation, define custom volume and not a G4Box). All Copper planes have the same thickness of 18 microns. Three FR4 thickness planes are defined depending on their position in the stack: Outer plane (20 microns), Middle plane (100 microns), Inner plane (200 microns). All planes are arranged side by side with no gaps between them. 

## EventAction
This class inherits the GEANT4 class G4UserEventAction and provides the control over GEANT4 processes at the event level. Currently, the implementation allows for handling actions at the beginning and end of an event via 2 methods: PhMattEventAction::BeginOfEventAction(const G4Event* event), PhMattEventAction::EndOfEventAction(const G4Event* event)

### EndofEvent Action
Progress bar is implemented in order to output the expected total simulation time and elapsed time. The elapsed time is updated in a pre-defined step size, relative to the total number of events: totalEvents / 100. The progress bar print out is performed in a thread-safe manner with the help of the G4AutoLock lock(&g4CounterMutex) object. 

## PhysicsList
This class inherits the GEANT4 class G4VModularPhysicsList and defines the physics to be considered in the simulation. Two physics cases are envisioned for the simulation package:

1. The energy deposition of hadrons in thin Silicon layers is accurately simulated by G4EmStandardPhysics(). 
2. The hadron interaction in several planes of a telescope require further considerations for other effects, such as hadronic interactions. Preliminary physics list for hadron interactions: G4DecayPhysics; G4HadronElasticPhysics; G4HadronPhysicsFTFP_BERT; G4StoppingPhysics; G4IonPhysics. 

Currently, physics lists are switched with the help of flags.

The SetCuts() method is implemented to modify the production cuts of e-, e+ and gammas. This has an impact on secondary particle propagation in thin sensors, such is the case of the MALTA2 sensor. A very low production cut has an impact on the size of the stored data, however it improves accuracy in simulating secondar propagation. A value of 1 um was found to yield reasonable results for the tracking of a single MALTA2 sensor.

## PrimaryGenerator
This class inherits the GEANT4 class G4VUserPrimaryGeneratorAction and defines the particle generator. All particles are generated via the G4ParticleGun(1) method. This defines the number of primaries (1) that GEANT4 will shoot at the generator level of the class. The number of primaries that GEANT4 shoots at the same time can however be customized with the help of the particleCount flag which is implemented at the GeneratePrimaries level in order to accound for random sampling of desired parameters: position, momentum, energy, particle type.

Currently, continuous distributions of the primary position is implmented via the following functions:

1. Pencil beam: define a fixed position in PhMattPrimaryGenerator() as a G4ThreeVector
2. Circular beam: defined with the GetRandomPointOnCircle() method in GeneratePrimaries()
3. Rectangular beam: defined with GetRandomPointOnRectangle() method in GeneratePrimaries()

Currently, no energy distribution, beam straggling, beam spread or mixed particle type is implemented. 

The time structure of the beam can be customized with the help of the beamVeto flag.

## RunAction
This class inherits the GEANT4 class G4UserRunAction and gives access to run level GEANT4 actions.

In the class constructor the following Ntuples are defined:

1. "ScatAngle" Ntuple ID 0 is populated in SteppingAction::UserSteppingAction. This Ntuple is not yet maintained and might be deprecated. It has the following columns:
    - "iEvent" stores the EventID
    - "fX" stores the X coordinate of a GEANT4 event: preStepPoint->GetPosition()[0]
    - "fY" stores the Y coordinate of a GEANT4 event: preStepPoint->GetPosition()[1]
    - "fZ" stores the Z coordinate of a GEANT4 event: preStepPoint->GetPosition()[2]
    - "ScateringAngle" stores the angle between the G4ThreeVector momenta predefined as entering/ exiting hardcoded volumes. Needs to be generalized for more functionality. Value converted to angle from radian
    - "MomentumVal" stores the magnitude of the G4ThreeVector momentum emerging from the defined volume.
    Ntuple used for the scattering angle and the emerging momentum for particles traveling through a predefined volume/ several volumes. It is used for measuring the expected beam deflection by different materials.

2. "RawPixelHits" Ntuple ID 1 is populated in SensitiveDetector::ProcessHits. It has the following columns:
    - "iEvent" stores the Event ID from G4RunManager::GetRunManager()->GetCurrentEvent()->GetEventID()
    - "iPlane" stores the Plane ID
        Example:
        if(planeName == "physSensor") 
        {
            planeID = 0;
        }
    - "iHit" stores the Hit ID. A hit is defined as an ionization step in the sensor 
    - "PixX" stores the X coordinate of the hit from: pixelCluster[i][0]
    - "PiXY" stores the Y coordinate of the hit from: pixelCluster[i][1]
    - "hitTime" stores the global timing from: preStepPoint->GetGlobalTime()
    - "hitEnergy" stores the energy deposit of the GEANT4 hit defined from: GetEfficiencyAnalytical(InPixPos)* energy*100000/3.66
    Ntuple used for storing the raw Silicon hits

3. "TruthVertex" Ntuple ID 2 is populated in PrimaryGenerator::GeneratePrimaries. It has the following columns:
    - "iEvent" stores the Event ID from oneEvent->GetEventID()
    - "truthVertexX" store X coordinate of the primary vertex from G4ThreeVector(x, y, z)
    - "truthVertexY" store Y coordinate of the primary vertex from G4ThreeVector(x, y, z)
    - "truthVertexZ" store Z coordinate of the primary vertex from G4ThreeVector(x, y, z)
    - "trueGlobalTime" stores the global time stamp derived from the event ID via evtID * fFlag->beamVeto *ns + offSet *ns
    - "trueEnergy" stores the true energy of the primary particle defined by the particleEnergy flag

4. "ScatteringAngle" H1 ID 0 is populated in SteppingAction::UserSteppingAction. Contains the same information as the "ScatAngle" Ntuple. 

5. "MomentumDistribution" H1 ID 0 is populated in SteppingAction::UserSteppingAction. Contains the same information as the "ScatAngle" Ntuple.

The class additionally handles the IO calls for opening and closeing the output root files in the BeginofAction() and EndofAction() methods respectively.

## SensitiveDetector
This class inherits the GEANT4 G4VSensitiveDetector class and allows GEANT4 actions limited to volumes defined as sensitive in `DetectorConstruction::ConstructSDandField()`.


### Charge collection efficiency

Each energy deposit is scaled by a charge collection efficiency that depends on the in-pixel location. The energy deposit is distributed among the seed and its 3 nearest neighboring pixels (the three neighbors of the nearest pixel corner). The analytical scaling is derived from testbeam data and based on error-functions: `SensitiveDetector::GetEfficiencyAnalytical()`.

### Timing

The `SensitiveDetector::GetTimingOffset()` method calculates the timewalk based on amplitude and threshold. The parameterization was measured at a threshold of 150e-. Consequently, this threshold is used for reference. The timewalk for other thresholds is obtained by scaling the x-axis (charge-axis) accordingly. This is based on the assumption that the waveform of an n-times larger signal is the same at an n-times larger threshold. In data this is only valid if the front-end gain is changed through a different bias current. The same assumption can be phrased as: The gain scales with 1/threshold. This scaling is chosen because at the same gain setting (fixed front-end) only a limited threshold range can be covered. For example at "normal" gain a range of 200-700 e- can be covered. Larger threshold ranges can only be reached by lowering the gain. The threshold scaling is illustrated in [plotting_scripts/plot_timewalk_curves.py](plotting_scripts/plot_timewalk_curves.py)

## SteppingAction
This class inherits the GEANT4 G4UserSteppingAction class and implements GEANT4 actions at the step level. It populates the "ScatAngle" Ntuple and "ScatteringAngle" and "MomentumDistribution" histograms.

## SubmissionTests
Implementation of tests to run before Condor Job submission. 

## TrackingAction
This class inherits the GEANT4 G4UserTrackingAction class and implements GEANT4 actions at the track level. Currently, has no implementation. Can be used in the future to kill early unwanted tracks for increasing computation efficiency.

# Analysis chain. analysis/
This directory provides all the tools for performing the tracking, clustering and analysis of the raw simulation files. All functions are written as root functions. At each analysis step a debugging/monitoring root file is created in the Results/ directory together with monitoring histograms in the Plots/ directory. 

## DigitalProcessing
Provides the digital processing of the hits. The main steps of this analysis step are:
1. Summ up all hits based on eventID and hit pixel coordinates. 
2. Create threshold dispersion map
3. Apply threshold to all 4 or more hits associated to 1 event and discard all hits below threshold
4. Compute the timing of the hit based on 3 effects: global time stamp, timeWalk calculated with GetTimingOffset() and rowPropagation calculated via 7ns/ 512 rows
5. Encode the hits into digital words, following the MALTA bit description
6. Sort all digital words into time buckets of programmable size
7. Merge all words inside the same time bucket, applying a simple OR operation to them
8. Decode all hits inside a word and encode this value in NHits

The digital processing outputs the following data:

    Results/SaveName/PlaneIDReconstructedHitsThrvalue.root/ReconstructedHits: 
       PixX
       PixY
       Timing
       NHits
    Plots/SaveName/histos.root/h1Dthreshold - 1D dsitribution of the threshold smearing
    Plots/SaveName/histos.root/h2Dthreshold - 2D dsitribution of the threshold smearing

## Tracking
Performs the matching between tracks reconstructed from MONTE CARLO truth information and reconstructed hits given spatial and timing cuts
1. Time sort all the tracks, based on the MONTE CARLO truth information provided by GEANT4. 
2. Perform a rolling window look-up between DUT hits and tracks in a programmable time window and match time adjacent hits to tracks
3. Perform a distance cut between each hit and the track based on a programmable distance cut
4. Save all the valid hits and give any invalids sentinel values (-1)

The tracking outputs the following data:

    Results/SaveName/LocalTrackedHitsThrValue.root/TrackedHits:
       trackID
       reconstructedVertexX
       reconstructedVertexY
       reconstructedGolbalTime
       PixX
       PixY
       nHits
       reconstructedLocalTime
    Plots/SaveName/histos.root/h2DUTHits - Hits reconstructed but not yet tracked on the DUT
    Plots/SaveName/histos.root/h1ResidualX - X tracking residual distribution
    Plots/SaveName/histos.root/h1ResidualY - Y tracking residual distribution

## Clustering
Provides the clustering of hits that were found to be time and space adjacent to a track in the previous tracking processing
1. The -1 sentinel values are identified as non-efficient hits
2. Cluster candidates are formed from valid coordinates
3. A cluster filtering algorithm is applied to check for valid clusters. Valid cluster criterion: Only spatially adjacent hits can form a cluster. Diagonally adjacent hits are cut from the final cluster formation.

The clustering outputs the following data:

    Results/SaveName/analysisThrValue.root/analyzedHits:
       analysisVertexX
       analysisVertexY
       clSize
       timing

## Analysis
Provides the final analysis step which computes the per bin efficiency, cluster size and timing. 

The analysis outputs the following files:

    Plots/SaveName/histos.root/h2PASS - 2D sensor efficiency
    Plots/SaveName/histos.root/h2ClSize - 2D sensor Cluster Size
    Plots/SaveName/histos.root/h2Timing - 2D sensor Timing
    Plots/SaveName/histos.root/h2PASSInPixel - 2D 2x2 in pixel projection efficiency
    Plots/SaveName/histos.root/h2ClSizeInPixel - 2D 2x2 in pixel projection cluster size
    Plots/SaveName/histos.root/h2TimingInPixel - 2D 2x2 in pixel projection timing
    Plots/SaveName/histos.root/h2PASSInPixel - debugging histogram. passed hits 2x2 in pixel projection
    Plots/SaveName/histos.root/h2ALLInPixel - debugging histogram. all hits 2x2 in pixel projection

Summary data of average efficiency, cluster size, timing, threshold is saved in the following nTuple:

    Plots/SaveName/summary.root/sumarryTree:
       threshold
       efficiency
       effError
       clSize
       clSizeError
       timing

## SimulationToProteus
Optional/ Alternative analysis path. Formats the 7 plane data (1 DUT + 6 tracking planes) to the usual Proteus MALTA format. This allows for analysing the simulated data with the help of the standard test beam framework.

## MALTAClustering 
Alternative clustering algorithm designed to mimic the MALTA TB algorithm. In this case, the clustering of time adjacent hits is performed first. The same cluster filtering is performed like in the case of the Clustering.cc

## MALTATracking
Alternative tracking algorithm designed to mimic the MALTA TB analysis. In this case, the tracking is performed on the reconstructed clusters. A cluster is matched to a track as long as any of the in-cluster hits are within a distance cut to it.

# Plotting scripts. plotting_scripts/

## compare_data_sim
[Lucian Please fill]

## plot_timewalk_curves
[Lucian Please fill]

## ratio_data_sim
[Lucian Please fill]

## ROOTTHelperFunctions
[Lucian Please fill]

## simpleAnalysis
Root macro that takes as input the thread-wise root output files. The analysis assumes the Geant4 events as independent and performs a tracking and timing cut of the MALTA hits relative to the MCtrue primary vertex. (TODO: Currently the path is hardcoded. Find smarter way of passing the path)

## summary_plots
Root macro that takes as input the average tracking figures of merit from summary.root files generated by the analysis chain in order to create comparison and/or data trend plots.

Currently the data input is manually passed  via the following variables:
std::vector<int> runNumbers= {} - the run number denoted by different GEANT4 simulation run number
std::vector<std::string> runFiles = {} - the analysis name denoted by different analysis chain of the same simulation run
Additionally th labels variable passes the desired plotting labels
std::vector<std::string> labels = {} - legend labels should have the size of runNubers.size() * runFiles.size()

The plots are just displayed but not saved. Manually saving required. If no X server is available for remote operation, a change go pdf saving of the canvases can be implemented.

## visualize_TCT_data
Root macro that takes as input E-TCT data as analog waveforms. Outputs pixel charge collection distributions.

# Config files. configs/
The user is encouraged to create their own configuration file as desired. The simulation (e.g. flags.cfg) and analysis (e.g. analysis_flags.cfg) flags can be easily passed via the argument line. If a foreign local installation of the package is desired, additional path configuration might be needed. The default ALMA9 path configuration is implemented via config.sh. Customized examples of local path sourcing can be seen in the config_vlad.sh and config_lucian.sh. A manual pointing towards the GEANT4, ROOT and any other non intialized libraries is needed for other local implementations. The passing of the correct config file is explained in the run_files section. 

## analysis_flags

Time walk calibration scaling default values:

    -T = 390, Tdiv = 200, TrefThr = 150, x0 = 149.8, n = 0.65, t0 = 0

MALTA2 digital encoding information default values:

    -groupSize = 16, groupLeng = 5, parityLeng = 1, dColLeng = 8

MALTA2 meging time bucket size default value:

    -wordSpacing = 1.6

Number of thread-wise data files to analyze default value:

    -numThreads = 6

Charge loss scaling of the deposited charge default value:

    -chLoss = 1

Threshold smearing in the mean and column default values:

    -meanSmearing = 0.08, colSmearing = 0.02

Tracking and clustering distance and time cut default values:

    -distCut = 100, timeCut = 500

Tracking uncertainty enable/disable default value:

    -trkUnc = true
    
Verbose flags for each analysis step default values:

    -verboseDigital = false, verboseTracking = false, verboseClustering = false, verboseAnalysis = false

## flags.cfg

Geometry flag used for switching between the MALTA geometry implementation and the PCB geometry implementation

    -preDefinedGeometryFlag = MALTA / PCB

Detector X,Y,Z offsets relative to the GEANT4 origin in [cm]. Default values:

    -detectorXOffset = 5., detectorYOffset = 5. ,detectorZOffset = 5.

Pixel size default value [mm]:

    -pixelSize = 0.0364 

Sensor dimension default values X, Y[cm], sensitive Depth [um]:

    -detectorSizeX = 1.86368, detectorSizeY = 1.86368, detectorDepth = 29.1

Charge sharing model parametrization in X and Y default values:

    -CCModelSigmaX = 4.3, CCModelSigmaY = 4.3   
    
World volume material. Accepted values are G4_GALACTIC (vacuum) and G4_AIR:

    -outsideMaterial = G4_Galactic / G4_AIR

Particle source geometry. Available presets: pencil, circle, rectangle, granularBeam = 1 particle every 2 pixels in X (simulate out of group hits)

    -beamGeometry = pencil / circle / rectangle / granularBeam

Beam offset in X, Y, Z default values [cm]:

    -beamXOffset = 5., beamYOffset = 5., beamZOffset = - 100.

Beam size, assuming only symmetrical beam geometry implementation, default value [mm]:

    -sourceRadius = 18.6368

Spill size, or number of particles fired in a coincidence time window, default value:

    -particleCount = 1

Number of events in a GEANT4 run, default value:

    -numEvents = 100000

Intra spill offset that can simulate the delays introduced by cabling in real devices, default value:

    -intraSpillOffset = 0

Period of the beam, default value [ns]:

    -beamVeto = 1000

Primary particle type, default:

    -particleType = proton

Primary particle energy, default value [GeV]:

    -particleEnergy = 120

Momentum of the primary particle, default values:

    -particleMomentumX = 0., particleMomentumY = 0., particleMomentumZ = 1.

Physics list flags. Enable/disable different physics processes, default values:

    -EMPhysics = true, hadronPhysics = true

Distance cut values for e-,e+ and photons, default value:

    -GEANT4CutValue = 1

Raw data storage path for local and naf running, default values:

    -outputPathLocal = Results, outputPathNAF = Results

GEANT4 macro files for local and naf running, default values (run - batch mode, vis - gui mode):

    -macroFileLocal = run.mac, macroFileNAF = run.mac

Number of Threads. Should match the number of usable cores. Optimization between physical/ virtual cores. Default values:

    -numThreadsLocal = 6, numThreadsNAF = 12

Verbose flags for different parts of the simulation, default values:

    -verbosePL = false, verbosePG = false, verboseSD = false, verboseSA = false

## config.sh
For expert users only. If a local GEANT4 installation is available and desired, the following path format should be replicated:

    -export LOCAL_PATH=/home/path/to/repo 
    -export LOCAL_GEANT=/home/path/to/geant4/geant4.sh 
    -export LOCAL_ROOT=/home/path/to/root/thisroot.sh 
    -export EXTRA_LOCAL=NEEDED_LIBRARY_HERE=/path/to/lib 
    -export NAF_GEANT=/cloud/path/to/geant/geant4.sh 
    -export NAF_PATH=cloud/path/to/repo
    -export NAF_USER=user

# Run macros. macros/
Three GEANT4 macros are implemented:

    - run.mac. The main batch mode run macro. Supresses all GEANT4 verbosity. Can be enable by setting the fields to 1
    - run_test.mac. The run macro for the dry runnin before a job submission.
    - vis.mac The visualization macro. Particle colour scheme or other visualization values possible.

# Run files. run_files/
Three shell scripts are implemented in this directory that schedule the running of the simulation package:

    - run_script_local.sh handles the running locally. 
    The term locally also applies for running a simulation on a remote machine as long as no job submission to a computer cluster is involved.
    
    - run_script_naf.sh handles the cluster computing job submission. 
    This assumes access to the naf computing infrastructure or any other compatible infrastructure. However, path compatibility needs to be resolved. WARNING! such a run mode only works with a Condor job submission format.
    
    - run_script_local_test.sh handles the running of a local dry run
     before the submission of a job to the cluster, in order to ensure correct compilation of the package before requesting computing resources.

Instructions for passing a customized config.sh file:

1. Ensure that the file has a custom naming scheme and is present in the /configs folder. 
2. Identify the local HOME directory path with the help of
    echo $HOME
3. Edit the run_script_local.sh file, following the format:

    elif [$HOME = "/Users/lucianfasselt"]; then
        echo "Running as user lucian"
        source ../configs/config_lucian.sh


## Visuals
TODO

# NAF Installation

```bash
# Log in to NAF
ssh username@naf-atlas.desy.de

# Create a working directory in your private area
mkdir -p private/directory_name
cd private/directory_name

# Initialize Git repository
git init
git remote set-url origin git@gitlab.cern.ch:7999/dberlea/malta_simulation.git
git pull origin master

# Create build and results directories
mkdir build Results condor_log
```

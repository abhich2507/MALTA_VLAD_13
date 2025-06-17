# MALTA_SIMULATION
The MALTA2 sensor is the second full scale implementation of the MALTA sensor family. An accurate simulation of the sensor performance in a test beam enviornment is desired for both test beam data validation but also future sensor redesign work. One such activity is the use of a MALTA-like sensor for digital calorimetry applications.

## Description
This repository contains a "pure" GEANT4 simulation of a DMAPS sensor, MALTA2. An accurate charge collection and charge sharing model is implemented with the help of previusly acquired Edge Transient Current Technique (E-TCT) measurements. The simulation is divided into several source (.cc) files and associated header files (.hh). The project is subsequently built with the help of a Cmake file: CMakeLists.txt. The files are sorted into the following folder structure:

- /analysis. contains all the python/ C++ scripts for data analysis and plotting
- /build. needs to be manually created and is the path from which the project is built and compiled
- /include. contains the header files
- /macros. contains the GEANT4 run and visualization macros
- /root_macros. contains the root macros for visualization and mainupalation of GEANT4 output
- /src. contains the source files

## ActionInitialization
This class inherit the GEANT4 class G4VUserActionInitialization. It is used to to intialize all the actions at the different GEANT4 levels in the package. The following actions are currently defined in the package: PrimaryGenerator, RunAction, EventAction, SteppingAction, TrackingAction. All initializations are done at the thread level, except for the run action which is additionally defined at the master thread level.

## DetectorConstruction
This class inherits the GEANT4 class G4VUserDetectorConstruction. It is used to define the solid, logic and physical volumes associated to the elements of the simulation. The aim of this class is to create an accurate simulation of the real MALTA2 sensor and fixtures but at the same time keep the geometry of the active detector simple enough in order to keep the computation times low.

### World Volume
The world volume is simmulated as vacuum, defined with the help of the pre-defined nist G4_Galactic. This is in order to limit the particle interactions outside of the sensitive volume and further optimize the computation time. The world volume is designed as a G4Box of size 1 *m x 1 *m x 1 *m, centered at the origin. Any further geometry implementation needs to respect the following X, Y, Z boundaries: [-50 *cm, 50 *cm]. 

### Sensor Volume
The sensor volume is simmulated as pure Si using the pre-defined nist G4_Si. Concerning the sensitive material, this simulation does not take into account the following elements:

- the non-sensitive SiO2 layer on top of the Si bulk
- the Cu metal layers above the PMD 
- the metal for the transistor gates of the N, P -MOSFETs
- the dopants and geometry of surface or deep implants
- any Si wafer post-processing procedure e.g. back metallization, conductive glue, etc.

The Sensor simulation is implemented in two distinct ways (1. and 2. currently 1. is commented out and is not taken into account):

1. Pixelated geometry implementation. The solid volume is defined as a G4Box with the following dimensions: 36.4 *um x 36.4 um (pixel pitch) x 30 *um (pixel depth). The logic volume created as mentioned above is then repeated to create a 512 x 512 pixel array, where all pixels are congruent. 

2. Monolithic geometry implementation. The solid volume is defined as a G4Box with the following dimennions: 18.6368 *mm x 18.6368 *mm x 30 *um.

Currently the simulation uses the monolithic geometry implementation for a believed decreased simmulation time. 

### PCB Volume
The Printed Circuit Board (PCB) to which the MALTA samples are glued to is simulated. Currently this is divorced from the sensor implementation and simulation, but can be later used for a complete telescope simulation. 

The PCB is implemented as Copper and dielectric planes with the help of the edms files of the E-TCT PCB. Two materials are used in the simulation. The pre-defined nist G4_Cu and a custom defined material: FR4. The FR4 material is defined as a mixture of O, C, Si, H, Na, B. The component percentage of each element was computed taking into account the atomic formula of a typical fire retardant PCB material found at reference: https://www.physics.smu.edu/web/research/preprints/SMU-HEP-08-11.pdf and the molar mass of each element. All Copper and FR4 planes are defined as rectangular 12.7 x 12.7 cm^2 but with various thicknesses as defined in the design file. (TODO: The current geometrical implementation does not match the exact physical dimensions of the board. For a more accurate implementation, define custom volume and not a G4Box). All Copper planes have the same thickness of 18 microns. Three FR4 thickness planes are defined depending on their position in the stack: Outer plane (20 microns), Middle plane (100 microns), Inner plane (200 microns). All planes are arranged side by side with no gaps between them. 

## EventAction
This class inherits the GEANT4 class G4UserEventAction and provides the control over GEANT4 processes at the event level. Currently, the implementation allows for handling actions at the beggining and end of an event via 2 methods: PhMattEventAction::BeginOfEventAction(const G4Event* event), PhMattEventAction::EndOfEventAction(const G4Event* event)

### EndofEvent Action
Progress bar is implemented in order to output the expected total simulation time and elapsed time. The elapsed time is updated in a pre-defined step size, relative to the total number of events: totalEvents / 100. The progress bar print out is performed in a thread-safe manner with the help of the G4AutoLock lock(&g4CounterMutex) object. 

## PhysicsList
This class inherits the GEANT4 class G4VModularPhysicsList and defines the physics to be considered in the simulation. Two physics cases are envisioned for the simulation package:

1. The energy deposition of hadrons in thin Silicon layers is accuratley simmulated by G4EmStandardPhysics(). 
2. The hadron interaction in several planes of a telescope require further considerations for other effects, such as hadronic interactions. Preliminary physics list for hadron interactions: G4DecayPhysics; G4HadronElasticPhysics; G4HadronPhysicsFTFP_BERT; G4StoppingPhysics; G4IonPhysics. 

Currently physics lists are switched by commenting/ uncommenting. TODO: implement smart flag for switching between EM to hadronic interactions

The SetCuts() method is implemented to modify the production cuts of e-, e+ and gammas to 1 micron currently. TODO: Characterize the impact of the production cut value on computation time and perform a study on the centroid of the secondaries path length in silicon for various particle energies.


## PrimaryGenerator
This class inherits the GEANT4 class G4VUserPrimaryGeneratorAction and defines the particle generator. All particles are generated via the G4ParticleGun(1) method. One event being associated with a generated particle. Currently, the particle type and energy is hardcoded as string, double values with the help of the respective GEANT4 methods. Additionally, three beam types can be defines, currently by commenting/ uncommenting code:

1. Pencil beam: define a fixed position in PhMattPrimaryGenerator() as a G4ThreeVector
2. Circular beam: defined with the GetRandomPointOnCircle() method in GeneratePrimaries()
3. Rectangular beam: defined with GetRandomPointOnRectangle() method in GeneratePrimaries()

Currently no energy distribution, beam straggling, beam spread is implemented. TODO: Search for Key4HEP SPS test beam implementation/ develop from scratch a more realistic beam condition. At the very least, more development on the beam energy spread needs to be done.

## RunAction
This class inherits the GEANT4 class G4UserRunAction and gives access to run level GEANT4 actions.

In the class constructuor the following Ntuples are defined:

1. "EnDeposited" Ntuple ID 0 is populated in SensitiveDetector::ProcessHits. It has the following columns:
    - "iEvent" stores the EventID
    - "fX" stores the X coordinate of a GEANT4 event: preStepPoint->GetPosition()[0]
    - "fY" stores the Y coordinate of a GEANT4 event: preStepPoint->GetPosition()[1]
    - "fZ" stores the Z coordinate of a GEANT4 event: preStepPoint->GetPosition()[2]
    - "fGlobalTime" stores the time stamp of a GEANT4 event: preStepPoint->GetGlobalTime();
    - "Energy" stores the energy of a GEANT4 event: aStep->GetTotalEnergyDeposit();

    Ntuple used for the energy deposited in the sensitive volume, currently defined as the silicon detector volume.


2. "ScatAngle" Ntuple ID 1 is populated in SteppingAction::UserSteppingAction. It has the following columns:
    - "iEvent" stores the EventID
    - "fX" stores the X coordinate of a GEANT4 event: preStepPoint->GetPosition()[0]
    - "fY" stores the Y coordinate of a GEANT4 event: preStepPoint->GetPosition()[1]
    - "fZ" stores the Z coordinate of a GEANT4 event: preStepPoint->GetPosition()[2]
    - "ScateringAngle" stores the angle between the G4ThreeVector momenta predefined as entering/ exiting hardcoded volumes. Needs to be generalized for more functionality. Value converted to angle from radian
    - "MomentumVal" stores the magnitude of the G4ThreeVector momentum emerging from the defined volume.
    Ntuple used for the scattering angle and the emerging momemntum for particles traveling through a predefined volume/ several volumes. It is used for measuring the expected beam deflection by different materials.

3. "DebuggingInfo" Ntuple ID 2 is populated in SensitiveDetector::ProcessHits. It has the following columns:
    - "iEvent" stores the EventID
    - "TravelLength" stores the step length of an interaction for secondaries in microns.
    Ntuple used currently only for secondary particles. Can raise errors in further extension of the package. 

4. "ScatteringAngle" H1 ID 0 is populated in SteppingAction::UserSteppingAction. Contains the same information as the "ScatAngle" Ntuple. TODO: probably remove implementation.

5. "MomentumDistribution" H1 ID 0 is populated in SteppingAction::UserSteppingAction. Contains the same information as the "ScatAngle" Ntuple. TODO: probably remove implementation.

Additionally, it opens and closes the output files in the BeginofAction() and EndofAction() methods respectively.
## SensitiveDetector
This class inherits the GEANT4 G4VSensitiveDetector class and allows GEANT4 actions limited to volumes defined as sensitive in DetectorConstruction::ConstructSDandField().

The SensitiveDetector::ProcessHits() method populates the "EnDeposited" and "DebuggingInfo" ntuples.

## SteppingAction
This class inherits the GEANT4 G4UserSteppingAction class and implements GEANT4 actions at the step level. It populates the "ScatAngle" Ntuple and "ScatteringAngle" and "MomentumDistribution" histograms.

## TrackingAction
This class inherits the GEANT4 G4UserTrackingAction class and implements GEANT4 actions at the track level. Currently has no implementation. Can be used in the future to kill early unwanted tracks for increasing computation efficiency.

## In_pixel_plots
Root macro that takes as input the thread-wise root output files. (TODO: Currently the path is hardcoded. Find smarter way of passing the path)

## visualize_TCT_data
TODO

## run macro
TODO

## Visuals
TODO

## Installation
TODO

## Usage
TODO

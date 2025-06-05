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
TODO

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

## EventAction
TODO

## PhysicsList
TODO

## PrimaryGenerator
TODO

## RunAction
TODO

## SensitiveDetector
TODO

## SteppingAction
TODO

## TrackingAction
TODO

## In_pixel_plots
TODO

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

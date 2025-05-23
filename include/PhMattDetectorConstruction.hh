#ifndef PHMATTDETECTORCONSTRUCTION_HH
#define PHMATTDETECTORCONSTRUCTION_HH
// Main class
#include "G4VUserDetectorConstruction.hh"
//Solid Volume e.g. Box
#include "G4Box.hh"
#include "G4Sphere.hh"
#include "G4Tubs.hh"
#include "G4SubtractionSolid.hh"
#include "G4UnionSolid.hh"
// Logical Volume
#include "G4LogicalVolume.hh"
// Physical Volume
#include "G4VPhysicalVolume.hh"
// Placement in world
#include "G4PVPlacement.hh"
// Material definition
#include "G4Material.hh"

#include "G4NistManager.hh"

#include "G4SystemOfUnits.hh"

#include "G4UnitsTable.hh"
// Visualization attribute
#include "G4VisAttributes.hh"
// Color just for visualization
#include "G4Color.hh"
// Sensitive Detector Manager
#include "G4SDManager.hh"

#include "PhMattSensitiveDetector.hh"
// Optical surface coupling imports
#include "G4OpticalSurface.hh"
#include "G4LogicalBorderSurface.hh"
#include "G4LogicalSkinSurface.hh"

class PhMattDetectorConstruction : public G4VUserDetectorConstruction
{
public:
    PhMattDetectorConstruction();
    // virtual because overwrite allready defined in G4VUserDetectorConstruction
    virtual ~PhMattDetectorConstruction();
    virtual G4VPhysicalVolume *Construct();

private:
    G4LogicalVolume *logicPixel;
    // Method constructs any sensitive detector or additional field
    virtual void ConstructSDandField();
};

#endif

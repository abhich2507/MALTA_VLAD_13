#ifndef DETECTORCONSTRUCTION_H
#define DETECTORCONSTRUCTION_H
// Main class
#include "G4VUserDetectorConstruction.hh"
#include "G4LogicalVolume.hh"

class SimFlags;

class DetectorConstruction : public G4VUserDetectorConstruction
{
public:
    explicit DetectorConstruction(const SimFlags* flags);
    ~DetectorConstruction() override = default;
    G4VPhysicalVolume *Construct() override;

private:
    const SimFlags* m_flag{};
    G4LogicalVolume* m_logicSensor{};
    G4LogicalVolume* m_logicPlane1{};
    G4LogicalVolume* m_logicPlane2{};
    G4LogicalVolume* m_logicPlane3{}; 
    G4LogicalVolume* m_logicPlane4{};
    G4LogicalVolume* m_logicPlane5{};
    G4LogicalVolume* m_logicPlane6{};
    G4LogicalVolume* m_logicMultiPlane{};
    // The gdml import sensitive Volumes
    std::vector<G4LogicalVolume*> sensitiveLVs;
    // Method constructs any sensitive detector or additional field
    void ConstructSDandField() override;
};

#endif

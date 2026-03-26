#include "DetectorConstruction.h"
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
#include "Config.h"
#include "SensitiveDetector.h"
// Optical surface coupling imports
#include "G4OpticalSurface.hh"
#include "G4LogicalBorderSurface.hh"
#include "G4LogicalSkinSurface.hh"
#include "G4GDMLParser.hh"
#include "G4LogicalVolumeStore.hh"
#include <cassert>


// Constructor
DetectorConstruction::DetectorConstruction(const SimFlags* flags) : m_flag(flags)
{

}

// Build the geometry
G4VPhysicalVolume *DetectorConstruction::Construct()
{
    assert(m_flag != nullptr);

    if (m_flag->gdmlBool)
    {
        G4GDMLParser parser;
        parser.Read(m_flag->gdmlStr, false);

        G4VPhysicalVolume* world = parser.GetWorldVolume();
        if (!world)
        {
            G4cerr << "GDML world volume is null!" << G4endl;
            return nullptr;  // abort
        }
        auto worldSolid = world->GetLogicalVolume()->GetSolid();
        G4cout << "World volume name: " << world->GetName()
            << ", cubic volume: " << worldSolid->GetCubicVolume() << G4endl;
        // Optionally assign a default visualization to all volumes
        auto lvStore = G4LogicalVolumeStore::GetInstance();
        for (auto lv : *lvStore)
        {
            auto volumeName = lv->GetName();
            auto volumeMaterial = lv->GetMaterial()->GetName();
            G4cout << "Volume name: " << volumeName << " Volume material: " << volumeMaterial << G4endl;
            if (!lv->GetVisAttributes())
            {
                double el1,el2,el3,el4;
                if(volumeMaterial == "Aluminium") el1 = 1.; el2 = 1.; el3 = 1.; el4 = 0.2;
                if(volumeMaterial == "Silicon")   
                {
                    el1 = 1.; el2 = 1.; el3 = 0.; el4 = 0.5;
                    sensitiveLVs.push_back(lv);
                }
                if(volumeMaterial == "Copper")    el1 = 1.; el2 = 0.; el3 = 0.; el4 = 0.1;
                if(volumeMaterial == "Tungsten")  el1 = 0.; el2 = 1.; el3 = 0.; el4 = 0.2;
                if(volumeMaterial == "Kapton")    el1 = 1.; el2 = 0.65; el3 = 0.; el4 = 0.2;
                if(volumeMaterial == "GSO")       
                {
                    el1 = 0.; el2 = 0.; el3 = 1.; el4 = 0.3;
                    sensitiveLVs.push_back(lv);
                }
                if(volumeMaterial == "Gten")      el1 = 0.68; el2 = 0.85; el3 = 0.9; el4 = 0.1;
                if(volumeMaterial == "Air")       el1 = 0.; el2 = 0.; el3 = 0.545; el4 = 0.5;
                if(volumeMaterial == "Acrylic")   el1 = 1.; el2 = 1.; el3 = 1.; el4 = 0.1;
                
                
                auto visAtt = new G4VisAttributes(G4Colour(el1,el2,el3,el4));
                visAtt->SetForceSolid(true);
                lv->SetVisAttributes(visAtt);
            }
        }
        

        // Keep a member pointer so your GUI code can reference it
    
        return world;
    }


    // Volume overlap check needed to ensure correct physics simulation
    const G4bool checkOverlaps {true};
    // Instantiate NIST material manager
    G4NistManager *nist = G4NistManager::Instance();
    // Define world material
    G4String outsideMat =  m_flag->outsideMaterial;
    G4Material *worldMat = nist->FindOrBuildMaterial(outsideMat);
    
    if (!worldMat)
    {
        throwError("DetectorConstruct::Construct", "SamplingError", "Material \"" + outsideMat + "\" not found in NIST database.");
    }

    if (outsideMat != "G4_Galactic" && outsideMat != "G4_Air")
    {
        throwWarning("DetectorConstruct::Construct", "SamplingWarning", "World material is not air or vacuum. Are you sure about this?");
    }
    
    G4Material *detMat = nist->FindOrBuildMaterial("G4_Si");
    G4double detectorXOffset = m_flag->detectorXOffset *cm;
    G4double detectorYOffset = m_flag->detectorYOffset *cm;
    G4double detectorZOffset = m_flag->detectorZOffset *cm;

    

    // Define the world
    const G4double xWorld = 2. *m;
    const G4double yWorld = 2. *m;
    const G4double zWorld = 2. *m;
    // Solid volume definition. 0.5* because G4Box takes the halflength as input
    G4Box *solidWorld = new G4Box("solidWorld", 0.5 * xWorld, 0.5 * yWorld, 0.5 * zWorld);
    // Logical world definition. Takes over solid volume and applies the worldMat material
    G4LogicalVolume *logicalWorld = new G4LogicalVolume(solidWorld, worldMat, "logicalWorld");
    // Physical Volume
    G4VPhysicalVolume *physWorld = new G4PVPlacement(0, G4ThreeVector(0.,0.,0.), logicalWorld, "physWorld", 0, false, 0, checkOverlaps);


    // Define an absorber. Hard coded for now
    G4Material *absorberMat = nist->FindOrBuildMaterial("G4_W");
    G4double absorberX = m_flag->detectorSizeX *cm;
    G4double absorberY = m_flag->detectorSizeY *cm;
    G4double absorberZ = m_flag->absorberThickness *cm;
    G4Box *solidAbsorber = new G4Box("solidAbsorber", 0.5 *absorberX, 0.5 *absorberY, 0.5 *absorberZ);
    G4LogicalVolume *logicalAbsorber = new G4LogicalVolume(solidAbsorber, absorberMat, "logicalAbsorber");
    if(m_flag->dutTungstenAbsorberFlag == true) 
    {
        new G4PVPlacement(0, G4ThreeVector(detectorXOffset, detectorYOffset, detectorZOffset - 0.5 *absorberZ - 0.01 *cm), logicalAbsorber, "physAbsorber", logicalWorld, false, 0, checkOverlaps); 
        G4VisAttributes *absorberVisAtt = new G4VisAttributes(G4Color(0.9, 0.4, 0.0, 0.6));
        absorberVisAtt->SetForceSolid(true);
        logicalAbsorber->SetVisAttributes(absorberVisAtt);
    }


    // MALTA implementation monolithic sensor
    if(m_flag->preDefinedGeometryFlag == "MALTA")
    {
        G4double maltaWidthX = m_flag->detectorSizeX *cm; 
        G4double maltaWidthY = m_flag->detectorSizeY *cm; 
        G4double maltaDepth  = m_flag->detectorDepth* um;

        if (std::abs(detectorXOffset) + maltaWidthX > xWorld /2 
         || std::abs(detectorYOffset) + maltaWidthY > yWorld /2 
         || std::abs(detectorZOffset) + maltaDepth > zWorld /2)
        {
            throwError("DetectorConstruct::Construct", "Invalid geometry", "DUT outside world");
        }
        
        auto DUTrotation = new G4RotationMatrix();
        // WARNING: Hardcoded values. This part of the simulation is not part of the core MALTA simulation
        // Repurpose as desired on own cost.
        DUTrotation->rotateY(60 * deg);

        G4Box *solidMALTA = new G4Box ("MALTASensor", maltaWidthX/2, maltaWidthY/2, maltaDepth/2);
        m_logicSensor = new G4LogicalVolume (solidMALTA, detMat, "logicSensor");
        new G4PVPlacement(0, G4ThreeVector(detectorXOffset, detectorYOffset, detectorZOffset), m_logicSensor, "physSensor", logicalWorld, false, 0, true);

        G4VisAttributes *pixelVisAtt = new G4VisAttributes(G4Color(0., 0., 1., 0.5));
        pixelVisAtt->SetForceSolid(true);
        m_logicSensor->SetVisAttributes(pixelVisAtt);

        std::vector< G4double> planeCorrections = {-64.2, -56.2, -48.2, 29.8, 37.8, 45.8};

        for (const double& corr : planeCorrections)
        {
            if (std::abs(corr) * cm + maltaDepth > zWorld /2)
            {
                throwError("DetectorConstruct::Construct", "Invalid geometry", "Tracking Plane outside world");
            }
        }

        m_logicPlane1 = new G4LogicalVolume (solidMALTA, detMat, "logicPlane1");
        new G4PVPlacement(0, G4ThreeVector(detectorXOffset, detectorYOffset, detectorZOffset + planeCorrections[0]*cm)
                                                            , m_logicPlane1, "physPlane1", logicalWorld, false, 0, true);
        m_logicPlane1->SetVisAttributes(pixelVisAtt);

        m_logicPlane2 = new G4LogicalVolume (solidMALTA, detMat, "logicPlane2");
        new G4PVPlacement(0, G4ThreeVector(detectorXOffset, detectorYOffset, detectorZOffset + planeCorrections[1]*cm)
                                                            , m_logicPlane2, "physPlane2", logicalWorld, false, 0, true);
        m_logicPlane2->SetVisAttributes(pixelVisAtt);

        m_logicPlane3 = new G4LogicalVolume (solidMALTA, detMat, "logicPlane3");
        new G4PVPlacement(0, G4ThreeVector(detectorXOffset, detectorYOffset, detectorZOffset + planeCorrections[2]*cm)
                                                            , m_logicPlane3, "physPlane3", logicalWorld, false, 0, true);
        m_logicPlane3->SetVisAttributes(pixelVisAtt);

        m_logicPlane4 = new G4LogicalVolume (solidMALTA, detMat, "logicPlane4");
        new G4PVPlacement(0, G4ThreeVector(detectorXOffset, detectorYOffset, detectorZOffset + planeCorrections[3]*cm)
                                                            , m_logicPlane4, "physPlane4", logicalWorld, false, 0, true);
        m_logicPlane4->SetVisAttributes(pixelVisAtt);

        m_logicPlane5 = new G4LogicalVolume (solidMALTA, detMat, "logicPlane5");
        new G4PVPlacement(0, G4ThreeVector(detectorXOffset, detectorYOffset, detectorZOffset + planeCorrections[4]*cm)
                                                            , m_logicPlane5, "physPlane5", logicalWorld, false, 0, true);
        m_logicPlane5->SetVisAttributes(pixelVisAtt);

        m_logicPlane6 = new G4LogicalVolume (solidMALTA, detMat, "logicPlane6");
        new G4PVPlacement(0, G4ThreeVector(detectorXOffset, detectorYOffset, detectorZOffset + planeCorrections[5]*cm)
                                                            , m_logicPlane6, "physPlane6", logicalWorld, false, 0, true);
        m_logicPlane6->SetVisAttributes(pixelVisAtt);


 
    }

    else if(m_flag->preDefinedGeometryFlag == "PCB")
    {
        // FR4 flame retardant dielectric for PCB implementation. 
        // Source: https://www.physics.smu.edu/web/research/preprints/SMU-HEP-08-11.pdf
        G4Material* FR4 = new G4Material("FR4", 1.9*g/cm3, 6);
        FR4->AddElement(nist->FindOrBuildElement("O"),  0.3782);
        FR4->AddElement(nist->FindOrBuildElement("C"),  0.3344);
        FR4->AddElement(nist->FindOrBuildElement("Si"), 0.2120);
        FR4->AddElement(nist->FindOrBuildElement("H"),  0.0308);
        FR4->AddElement(nist->FindOrBuildElement("Na"), 0.0246);
        FR4->AddElement(nist->FindOrBuildElement("B"),  0.0200);

        G4Material *metalPlane = nist->FindOrBuildMaterial("G4_Cu");
        // Hardocded PCB values. These will stay like this as no reasonable customization of these values is reasonable.
        // If another PCB geometry is desired, it should be implemented separately.
        G4double PCBLateralSize = 12.7 *cm;
        G4double CuPlaneThickness = 0.018 *mm;
        G4double FR4OuterPlaneThickness = 0.02 *mm;
        G4double FR4MiddlePlaneThickness = 0.1 *mm;
        G4double FR4InnerPlaneThickness = 0.2 *mm;

        // Define logical Cu Plane
        G4Box* solidCuPlane = new G4Box("solidCuPlane", 0.5 * PCBLateralSize, 0.5 * PCBLateralSize, 0.5 * CuPlaneThickness);
        G4LogicalVolume* logicCuPlane = new G4LogicalVolume(solidCuPlane, metalPlane, "logicCuPlane");
        G4VisAttributes *CuPlaneVisAtt = new G4VisAttributes(G4Color(0.6, 0.1, 0.1, 0.5));
        CuPlaneVisAtt->SetForceSolid(true);
        logicCuPlane->SetVisAttributes(CuPlaneVisAtt);

        // Define logical outer plane dielectric
        G4Box* solidFR4OuterPlane = new G4Box("solidFR4OuterPlane", 0.5 * PCBLateralSize, 0.5 * PCBLateralSize, 0.5 * FR4OuterPlaneThickness);
        G4LogicalVolume* logicFR4OuterPlane = new G4LogicalVolume(solidFR4OuterPlane, FR4, "logicFR4OuterPlane");
        G4VisAttributes *FR4OuterVisAtt = new G4VisAttributes(G4Color(0.5, 0.9, 0.5, 0.5));
        FR4OuterVisAtt->SetForceSolid(true);
        logicFR4OuterPlane->SetVisAttributes(FR4OuterVisAtt);

        // Define logical middle plane dielectric
        G4Box* solidFR4MiddlePlane = new G4Box("solidFR4MiddlePlane", 0.5 * PCBLateralSize, 0.5 * PCBLateralSize, 0.5 * FR4MiddlePlaneThickness);
        G4LogicalVolume* logicFR4MiddlePlane = new G4LogicalVolume(solidFR4MiddlePlane, FR4, "logicFR4MiddlePlane");
        G4VisAttributes *FR4MiddleVisAtt = new G4VisAttributes(G4Color(0., 0.5, 0., 0.5));
        FR4MiddleVisAtt->SetForceSolid(true);
        logicFR4MiddlePlane->SetVisAttributes(FR4MiddleVisAtt);

        // Define logical inner plane dielectric
        G4Box* solidFR4InnerPlane = new G4Box("solidFR4InnerPlane", 0.5 * PCBLateralSize, 0.5 * PCBLateralSize, 0.5 * FR4InnerPlaneThickness);
        G4LogicalVolume* logicFR4InnerPlane = new G4LogicalVolume(solidFR4InnerPlane, FR4, "logicFR4InnerPlane");
        G4VisAttributes *FR4InnerVisAtt = new G4VisAttributes(G4Color(0., 0.39, 0., 0.5));
        FR4InnerVisAtt->SetForceSolid(true);
        logicFR4InnerPlane->SetVisAttributes(FR4InnerVisAtt);

        // Rotation for inclined measurements
        auto rotation = new G4RotationMatrix();
        // WARNING: Hardcoded values. This part of the simulation is not part of the core MALTA simulation
        // Repurpose as desired on own cost.
        rotation->rotateX(90 * deg);
        rotation->rotateY(90 * deg);

        // Construct the physical PCB stack
        G4double currentZ = 6.0 * cm;
        
        new G4PVPlacement(rotation, G4ThreeVector(5 *cm, 5 *cm, currentZ), logicFR4OuterPlane, "Outer1", logicalWorld, false, 0, true);
        currentZ += FR4OuterPlaneThickness;
        new G4PVPlacement(rotation, G4ThreeVector(5 *cm, 5 *cm, currentZ + 0.5 * CuPlaneThickness), logicCuPlane, "Cu1", logicalWorld, false, 0, true);
        currentZ += CuPlaneThickness;
        new G4PVPlacement(rotation, G4ThreeVector(5 *cm, 5 *cm, currentZ + 0.5 * FR4MiddlePlaneThickness), logicFR4MiddlePlane, "Middle1", logicalWorld, false, 0, true);
        currentZ += FR4MiddlePlaneThickness;
        new G4PVPlacement(rotation, G4ThreeVector(5 *cm, 5 *cm, currentZ + 0.5 * CuPlaneThickness), logicCuPlane, "Cu2", logicalWorld, false, 1, true);
        currentZ += CuPlaneThickness;
        new G4PVPlacement(rotation, G4ThreeVector(5 *cm, 5 *cm, currentZ + 0.5 * FR4InnerPlaneThickness), logicFR4InnerPlane, "Inner1", logicalWorld, false, 0, true);
        currentZ += FR4InnerPlaneThickness;
        new G4PVPlacement(rotation, G4ThreeVector(5 *cm, 5 *cm, currentZ + 0.5 * CuPlaneThickness), logicCuPlane, "Cu3", logicalWorld, false, 2, true);
        currentZ += CuPlaneThickness;  
        new G4PVPlacement(rotation, G4ThreeVector(5 *cm, 5 *cm, currentZ + 0.5 * FR4InnerPlaneThickness), logicFR4InnerPlane, "Inner2", logicalWorld, false, 1, true);
        currentZ += FR4InnerPlaneThickness;
        new G4PVPlacement(rotation, G4ThreeVector(5 *cm, 5 *cm, currentZ + 0.5 * CuPlaneThickness), logicCuPlane, "Cu4", logicalWorld, false, 3, true);
        currentZ += CuPlaneThickness;
        new G4PVPlacement(rotation, G4ThreeVector(5 *cm, 5 *cm, currentZ + 0.5 * FR4InnerPlaneThickness), logicFR4InnerPlane, "Inner3", logicalWorld, false, 2, true);
        currentZ += FR4InnerPlaneThickness;
        new G4PVPlacement(rotation, G4ThreeVector(5 *cm, 5 *cm, currentZ + 0.5 * CuPlaneThickness), logicCuPlane, "Cu5", logicalWorld, false, 4, true);
        currentZ += CuPlaneThickness;
        new G4PVPlacement(rotation, G4ThreeVector(5 *cm, 5 *cm, currentZ + 0.5 * FR4InnerPlaneThickness), logicFR4InnerPlane, "Inner4", logicalWorld, false, 3, true);
        currentZ += FR4InnerPlaneThickness;
        new G4PVPlacement(rotation, G4ThreeVector(5 *cm, 5 *cm, currentZ + 0.5 * CuPlaneThickness), logicCuPlane, "Cu6", logicalWorld, false, 5, true);
        currentZ += CuPlaneThickness;
        new G4PVPlacement(rotation, G4ThreeVector(5 *cm, 5 *cm, currentZ + 0.5 * FR4InnerPlaneThickness), logicFR4InnerPlane, "Inner5", logicalWorld, false, 4, true);
        currentZ += FR4InnerPlaneThickness;
        new G4PVPlacement(rotation, G4ThreeVector(5 *cm, 5 *cm, currentZ + 0.5 * CuPlaneThickness), logicCuPlane, "Cu7", logicalWorld, false, 6, true);
        currentZ += CuPlaneThickness;
        new G4PVPlacement(rotation, G4ThreeVector(5 *cm, 5 *cm, currentZ + 0.5 * FR4InnerPlaneThickness), logicFR4InnerPlane, "Inner6", logicalWorld, false, 5, true);
        currentZ += FR4InnerPlaneThickness;
        new G4PVPlacement(rotation, G4ThreeVector(5 *cm, 5 *cm, currentZ + 0.5 * CuPlaneThickness), logicCuPlane, "Cu8", logicalWorld, false, 7, true);
        currentZ += CuPlaneThickness;
        new G4PVPlacement(rotation, G4ThreeVector(5 *cm, 5 *cm, currentZ + 0.5 * FR4InnerPlaneThickness), logicFR4InnerPlane, "Inner7", logicalWorld, false, 6, true);
        currentZ += FR4InnerPlaneThickness;
        new G4PVPlacement(rotation, G4ThreeVector(5 *cm, 5 *cm, currentZ + 0.5 * CuPlaneThickness), logicCuPlane, "Cu9", logicalWorld, false, 8, true);
        currentZ += CuPlaneThickness;
        new G4PVPlacement(rotation, G4ThreeVector(5 *cm, 5 *cm, currentZ + 0.5 * FR4MiddlePlaneThickness), logicFR4MiddlePlane, "Middle2", logicalWorld, false, 1, true);
        currentZ += FR4MiddlePlaneThickness;
        new G4PVPlacement(rotation, G4ThreeVector(5 *cm, 5 *cm, currentZ + 0.5 * CuPlaneThickness), logicCuPlane, "Cu10", logicalWorld, false, 9, true);
        currentZ += CuPlaneThickness;
        new G4PVPlacement(rotation, G4ThreeVector(5 *cm, 5 *cm, currentZ + 0.5 * FR4OuterPlaneThickness), logicFR4OuterPlane, "Outer2", logicalWorld, false, 1, true);
    } 
    else
    {
        throwError("DetectorConstruct::Construct", "FatalException", "Undefined Geometry");
    }       
    return physWorld;
}

void DetectorConstruction::ConstructSDandField()
{
    SensitiveDetector *sensDet = new SensitiveDetector("SensitiveDetector", m_flag);
    G4SDManager::GetSDMpointer()->AddNewDetector(sensDet);
    if(m_flag->preDefinedGeometryFlag == "MALTA" && m_flag->gdmlBool == false)
    {
        // Ensure that methods initialize at end of event
        m_logicSensor->SetSensitiveDetector(sensDet);
        m_logicPlane1->SetSensitiveDetector(sensDet);
        m_logicPlane2->SetSensitiveDetector(sensDet);
        m_logicPlane3->SetSensitiveDetector(sensDet);
        m_logicPlane4->SetSensitiveDetector(sensDet);
        m_logicPlane5->SetSensitiveDetector(sensDet);
        m_logicPlane6->SetSensitiveDetector(sensDet);
    }

    if (m_flag->gdmlBool == true)
    {
        for (auto lv : sensitiveLVs) 
        {
            lv->SetSensitiveDetector(sensDet);
            G4cout << "Assigned sensitive detector to " << lv->GetName() << " Which if you didn't already know as you might should it is a material made out of: " << lv->GetMaterial()->GetName()<< G4endl;
        }

    }
}
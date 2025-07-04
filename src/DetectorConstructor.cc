#include "DetectorConstruction.hh"
// Constructor
DetectorConstruction::DetectorConstruction()
{

}

// Destructor
DetectorConstruction::~DetectorConstruction()
{
    
}

G4VPhysicalVolume *DetectorConstruction::Construct()
{
    // Volume overlap check needed to ensure correct physics simulation
    G4bool checkOverlaps = true;
    // Instantiate NIST material manager
    G4NistManager *nist = G4NistManager::Instance();
    // Define world material
    G4Material *worldMat = nist->FindOrBuildMaterial("G4_Galactic");
    G4Material *detMat = nist->FindOrBuildMaterial("G4_Si");

    // Define the world
    G4double xWorld = 1. *m;
    G4double yWorld = 1. *m;
    G4double zWorld = 1. *m;
    // Solid volume definition. 0.5* because G4Box takes the halflength as input
    G4Box *solidWorld = new G4Box("solidWorld", 0.5 * xWorld, 0.5 * yWorld, 0.5 * zWorld);
    // Logical world definition. Takes over solid volume and applies the worldMat material
    G4LogicalVolume *logicalWorld = new G4LogicalVolume(solidWorld, worldMat, "logicalWorld");
    // Physical Volume
    G4VPhysicalVolume *physWorld = new G4PVPlacement(0, G4ThreeVector(0.,0.,0.), logicalWorld, "physWorld", 0, false, 0, checkOverlaps);
    // MALTA implementation pixel like structure
    /*
    G4int nPixelsX = 512;
    G4int nPixelsY = 512;

    G4double pixelPitch = 36.4 * um;
    G4double pixelDepth = 30.0 * um;

    // Define pixel
    G4Box* solidPixel = new G4Box("Pixel", pixelPitch/2, pixelPitch/2, pixelDepth/2);
    logicPixel = new G4LogicalVolume(solidPixel, detMat, "Pixel");
    // Loop to place each pixel
    for (G4int ix = 0; ix < nPixelsX; ix++) 
    {
        for (G4int iy = 0; iy < nPixelsY; iy++) {
            G4double xpos = (ix - nPixelsX/2 + 0.5) * pixelPitch;
            G4double ypos = (iy - nPixelsY/2 + 0.5) * pixelPitch;
            G4ThreeVector position(xpos + 5 *cm, ypos + 5 *cm, 5 *cm); 
            
            new G4PVPlacement(nullptr, position, logicPixel, "Pixel", logicalWorld, false, ix*nPixelsY + iy, false);
        }
    }
    */

    // MALTA implementation monolithic sensor
    G4double maltaWidth = 18.6368 *mm; 
    G4double maltaDepth = 30 * um;
    G4Box *solidMALTA = new G4Box ("MALTASensor", maltaWidth/2, maltaWidth/2, maltaDepth/2);
    logicSensor = new G4LogicalVolume (solidMALTA, detMat, "logicSensor");
    G4VPhysicalVolume *physSensor  = new G4PVPlacement(0, G4ThreeVector(5 *cm, 5 *cm, 5 *cm), logicSensor, "physSensor", logicalWorld, false, 0, true);

    
    G4VisAttributes *pixelVisAtt = new G4VisAttributes(G4Color(0., 0., 1., 0.5));
    pixelVisAtt->SetForceSolid(true);
    logicSensor->SetVisAttributes(pixelVisAtt);


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
    logicFR4MiddlePlane->SetVisAttributes(FR4InnerVisAtt);

    // Rotation for inclined measurements
    auto rotation = new G4RotationMatrix();
    rotation->rotateX(90 * deg);
    rotation->rotateY(90 * deg);

    // Construct the physical PCB stack
    G4double currentZ = 6.0 * cm;
    G4double halfThickness = 0.5 * FR4OuterPlaneThickness;
    /*
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
    */






    return physWorld;
}

void DetectorConstruction::ConstructSDandField()
{
    
    SensitiveDetector *sensDet = new SensitiveDetector("SensitiveDetector");
    // Ensure that methods initialize at end of event
    G4SDManager::GetSDMpointer()->AddNewDetector(sensDet);
    logicSensor->SetSensitiveDetector(sensDet);
    
}
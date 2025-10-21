#include "DetectorConstruction.hh"
// Constructor
DetectorConstruction::DetectorConstruction(SimFlags* flags) : fFlag(flags)
{

}
// Destructor
DetectorConstruction::~DetectorConstruction()
{
    
}
// Build the geometry
G4VPhysicalVolume *DetectorConstruction::Construct()
{
    // Volume overlap check needed to ensure correct physics simulation
    G4bool checkOverlaps = true;
    // Instantiate NIST material manager
    G4NistManager *nist = G4NistManager::Instance();
    // Define world material
    G4String outsideMat =  fFlag->outsideMaterial;
    G4Material *worldMat = nist->FindOrBuildMaterial(outsideMat);
    
    if (!worldMat)
    {
        trowError("DetectorConstruct::Construct", "SamplingError", "Material \"" + outsideMat + "\" not found in NIST database.");
    }

    if (outsideMat != "G4_Galactic" && outsideMat != "G4_Air")
    {
        trowWarning("DetectorConstruct::Construct", "SamplingWarning", "World material is not air or vacuum. Are you sure about this?");
    }
    
    G4Material *detMat = nist->FindOrBuildMaterial("G4_Si");
    G4double detectorXOffset = fFlag->detectorXOffset *cm;
    G4double detectorYOffset = fFlag->detectorYOffset *cm;
    G4double detectorZOffset = fFlag->detectorZOffset *cm;

    // Define the world
    G4double xWorld = 2. *m;
    G4double yWorld = 2. *m;
    G4double zWorld = 2. *m;
    // Solid volume definition. 0.5* because G4Box takes the halflength as input
    G4Box *solidWorld = new G4Box("solidWorld", 0.5 * xWorld, 0.5 * yWorld, 0.5 * zWorld);
    // Logical world definition. Takes over solid volume and applies the worldMat material
    G4LogicalVolume *logicalWorld = new G4LogicalVolume(solidWorld, worldMat, "logicalWorld");
    // Physical Volume
    G4VPhysicalVolume *physWorld = new G4PVPlacement(0, G4ThreeVector(0.,0.,0.), logicalWorld, "physWorld", 0, false, 0, checkOverlaps);

    // MALTA implementation monolithic sensor
    if(fFlag->preDefinedGeometryFlag == "MALTA")
    {
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 8a2e4b8 (Cleaned up the simulation files. Removed the Truth En tree as it was not used further in the analysis chain. Additionally, general clean up in terms of branch renaming. It most probably will impact the in_pixel_plots script. However the default analysis chain has already been modified to account for these changes.)
=======
>>>>>>> b3c9266 (rebasing, for merging. caused by: removed sqrt2 from smoothstep)
        G4double maltaWidthX = fFlag->detectorSizeX *cm; 
        G4double maltaWidthY = fFlag->detectorSizeY *cm; 
        G4double maltaDepth = fFlag->detectorDepth* um;

        if (std::abs(detectorXOffset) + maltaWidthX > xWorld /2 
         || std::abs(detectorYOffset) + maltaWidthY > yWorld /2 
         || std::abs(detectorZOffset) + maltaDepth > zWorld /2)
        {
            trowError("DetectorConstruct::Construct", "Invalid geometry", "DUT outside world");
        }
<<<<<<< HEAD

        G4Box *solidMALTA = new G4Box ("MALTASensor", maltaWidthX/2, maltaWidthY/2, maltaDepth/2);
=======
        //TODO: This is hardcoded. It has its own flag already. Fix it
        G4double maltaWidth = 18.6368 *mm; 
<<<<<<< HEAD
        G4double maltaDepth = 29.1 * um;
        G4Box *solidMALTA = new G4Box ("MALTASensor", maltaWidth/2, maltaWidth/2, maltaDepth/2);
>>>>>>> 8149767 (Added digitization + tracking + clustering + analysis in an automatic fashion for each run)
=======
=======
>>>>>>> 8a2e4b8 (Cleaned up the simulation files. Removed the Truth En tree as it was not used further in the analysis chain. Additionally, general clean up in terms of branch renaming. It most probably will impact the in_pixel_plots script. However the default analysis chain has already been modified to account for these changes.)

        G4Box *solidMALTA = new G4Box ("MALTASensor", maltaWidthX/2, maltaWidthY/2, maltaDepth/2);
>>>>>>> 66f7594 (DEBUG)
=======
        G4double maltaDepth = 30 * um; // 29.1 * um; for measurement result
        G4Box *solidMALTA = new G4Box ("MALTASensor", maltaWidth/2, maltaWidth/2, maltaDepth/2);
>>>>>>> 18e3f08 (removed sqrt2 from smoothstep)
>>>>>>> b3c9266 (rebasing, for merging. caused by: removed sqrt2 from smoothstep)
        logicSensor = new G4LogicalVolume (solidMALTA, detMat, "logicSensor");
        G4VPhysicalVolume *physSensor  = new G4PVPlacement(0, G4ThreeVector(detectorXOffset, detectorYOffset, detectorZOffset), logicSensor, "physSensor", logicalWorld, false, 0, true);

        G4VisAttributes *pixelVisAtt = new G4VisAttributes(G4Color(0., 0., 1., 0.5));
        pixelVisAtt->SetForceSolid(true);
        logicSensor->SetVisAttributes(pixelVisAtt);

<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 8a2e4b8 (Cleaned up the simulation files. Removed the Truth En tree as it was not used further in the analysis chain. Additionally, general clean up in terms of branch renaming. It most probably will impact the in_pixel_plots script. However the default analysis chain has already been modified to account for these changes.)
        std::vector<double> planeCorrections = {-64.2, -56.2, -48.2, 29.8, 37.8, 45.8};

        for (const double& corr : planeCorrections)
        {
            if (std::abs(corr) * cm + maltaDepth > zWorld /2)
            {
                trowError("DetectorConstruct::Construct", "Invalid geometry", "Tracking Plane outside world");
            }
        }

<<<<<<< HEAD
        logicPlane1 = new G4LogicalVolume (solidMALTA, detMat, "logicPlane1");
        G4VPhysicalVolume *physPlane1  = new G4PVPlacement(0, G4ThreeVector(detectorXOffset, detectorYOffset, detectorZOffset + planeCorrections[0]*cm)
                                                            , logicPlane1, "physPlane1", logicalWorld, false, 0, true);
        logicPlane1->SetVisAttributes(pixelVisAtt);

        logicPlane2 = new G4LogicalVolume (solidMALTA, detMat, "logicPlane2");
        G4VPhysicalVolume *physPlane2  = new G4PVPlacement(0, G4ThreeVector(detectorXOffset, detectorYOffset, detectorZOffset + planeCorrections[1]*cm)
                                                            , logicPlane2, "physPlane2", logicalWorld, false, 0, true);
        logicPlane2->SetVisAttributes(pixelVisAtt);

        logicPlane3 = new G4LogicalVolume (solidMALTA, detMat, "logicPlane3");
        G4VPhysicalVolume *physPlane3  = new G4PVPlacement(0, G4ThreeVector(detectorXOffset, detectorYOffset, detectorZOffset + planeCorrections[2]*cm)
                                                            , logicPlane3, "physPlane3", logicalWorld, false, 0, true);
        logicPlane3->SetVisAttributes(pixelVisAtt);

        logicPlane4 = new G4LogicalVolume (solidMALTA, detMat, "logicPlane4");
        G4VPhysicalVolume *physPlane4  = new G4PVPlacement(0, G4ThreeVector(detectorXOffset, detectorYOffset, detectorZOffset + planeCorrections[3]*cm)
                                                            , logicPlane4, "physPlane4", logicalWorld, false, 0, true);
        logicPlane4->SetVisAttributes(pixelVisAtt);

        logicPlane5 = new G4LogicalVolume (solidMALTA, detMat, "logicPlane5");
        G4VPhysicalVolume *physPlane5  = new G4PVPlacement(0, G4ThreeVector(detectorXOffset, detectorYOffset, detectorZOffset + planeCorrections[4]*cm)
                                                            , logicPlane5, "physPlane5", logicalWorld, false, 0, true);
        logicPlane5->SetVisAttributes(pixelVisAtt);

        logicPlane6 = new G4LogicalVolume (solidMALTA, detMat, "logicPlane6");
        G4VPhysicalVolume *physPlane6  = new G4PVPlacement(0, G4ThreeVector(detectorXOffset, detectorYOffset, detectorZOffset + planeCorrections[5]*cm)
                                                            , logicPlane6, "physPlane6", logicalWorld, false, 0, true);
        logicPlane6->SetVisAttributes(pixelVisAtt);


 
=======
=======
>>>>>>> 8a2e4b8 (Cleaned up the simulation files. Removed the Truth En tree as it was not used further in the analysis chain. Additionally, general clean up in terms of branch renaming. It most probably will impact the in_pixel_plots script. However the default analysis chain has already been modified to account for these changes.)
        logicPlane1 = new G4LogicalVolume (solidMALTA, detMat, "logicPlane1");
        G4VPhysicalVolume *physPlane1  = new G4PVPlacement(0, G4ThreeVector(detectorXOffset, detectorYOffset, detectorZOffset + planeCorrections[0]*cm)
                                                            , logicPlane1, "physPlane1", logicalWorld, false, 0, true);
        logicPlane1->SetVisAttributes(pixelVisAtt);

        logicPlane2 = new G4LogicalVolume (solidMALTA, detMat, "logicPlane2");
        G4VPhysicalVolume *physPlane2  = new G4PVPlacement(0, G4ThreeVector(detectorXOffset, detectorYOffset, detectorZOffset + planeCorrections[1]*cm)
                                                            , logicPlane2, "physPlane2", logicalWorld, false, 0, true);
        logicPlane2->SetVisAttributes(pixelVisAtt);

        logicPlane3 = new G4LogicalVolume (solidMALTA, detMat, "logicPlane3");
        G4VPhysicalVolume *physPlane3  = new G4PVPlacement(0, G4ThreeVector(detectorXOffset, detectorYOffset, detectorZOffset + planeCorrections[2]*cm)
                                                            , logicPlane3, "physPlane3", logicalWorld, false, 0, true);
        logicPlane3->SetVisAttributes(pixelVisAtt);

        logicPlane4 = new G4LogicalVolume (solidMALTA, detMat, "logicPlane4");
        G4VPhysicalVolume *physPlane4  = new G4PVPlacement(0, G4ThreeVector(detectorXOffset, detectorYOffset, detectorZOffset + planeCorrections[3]*cm)
                                                            , logicPlane4, "physPlane4", logicalWorld, false, 0, true);
        logicPlane4->SetVisAttributes(pixelVisAtt);

        logicPlane5 = new G4LogicalVolume (solidMALTA, detMat, "logicPlane5");
        G4VPhysicalVolume *physPlane5  = new G4PVPlacement(0, G4ThreeVector(detectorXOffset, detectorYOffset, detectorZOffset + planeCorrections[4]*cm)
                                                            , logicPlane5, "physPlane5", logicalWorld, false, 0, true);
        logicPlane5->SetVisAttributes(pixelVisAtt);

        logicPlane6 = new G4LogicalVolume (solidMALTA, detMat, "logicPlane6");
        G4VPhysicalVolume *physPlane6  = new G4PVPlacement(0, G4ThreeVector(detectorXOffset, detectorYOffset, detectorZOffset + planeCorrections[5]*cm)
                                                            , logicPlane6, "physPlane6", logicalWorld, false, 0, true);
        logicPlane6->SetVisAttributes(pixelVisAtt);
<<<<<<< HEAD
        
>>>>>>> 8149767 (Added digitization + tracking + clustering + analysis in an automatic fashion for each run)
    }

<<<<<<< HEAD
    else if(fFlag->preDefinedGeometryFlag == "PCB")
=======
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

    if(fFlag->preDefinedGeometryFlag == "PCB")
>>>>>>> 66f7594 (DEBUG)
=======


 
    }

    else if(fFlag->preDefinedGeometryFlag == "PCB")
>>>>>>> 8a2e4b8 (Cleaned up the simulation files. Removed the Truth En tree as it was not used further in the analysis chain. Additionally, general clean up in terms of branch renaming. It most probably will impact the in_pixel_plots script. However the default analysis chain has already been modified to account for these changes.)
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
        logicFR4MiddlePlane->SetVisAttributes(FR4InnerVisAtt);

        // Rotation for inclined measurements
        auto rotation = new G4RotationMatrix();
        // WARNING: Hardcoded values. This part of the simulation is not part of the core MALTA simulation
        // Repurpose as desired on own cost.
        rotation->rotateX(90 * deg);
        rotation->rotateY(90 * deg);

        // Construct the physical PCB stack
        G4double currentZ = 6.0 * cm;
        G4double halfThickness = 0.5 * FR4OuterPlaneThickness;
        
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
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 8a2e4b8 (Cleaned up the simulation files. Removed the Truth En tree as it was not used further in the analysis chain. Additionally, general clean up in terms of branch renaming. It most probably will impact the in_pixel_plots script. However the default analysis chain has already been modified to account for these changes.)
    } 
    else
    {
        trowError("DetectorConstruct::Construct", "SamplingWarning", "Undefined Geometry");
    }       
<<<<<<< HEAD
=======
    }        
>>>>>>> 66f7594 (DEBUG)
=======
>>>>>>> 8a2e4b8 (Cleaned up the simulation files. Removed the Truth En tree as it was not used further in the analysis chain. Additionally, general clean up in terms of branch renaming. It most probably will impact the in_pixel_plots script. However the default analysis chain has already been modified to account for these changes.)
    return physWorld;
}

void DetectorConstruction::ConstructSDandField()
{
    if(fFlag->preDefinedGeometryFlag == "MALTA")
    {
        SensitiveDetector *sensDet = new SensitiveDetector("SensitiveDetector", fFlag);
        // Ensure that methods initialize at end of event
        G4SDManager::GetSDMpointer()->AddNewDetector(sensDet);
        logicSensor->SetSensitiveDetector(sensDet);
        logicPlane1->SetSensitiveDetector(sensDet);
        logicPlane2->SetSensitiveDetector(sensDet);
        logicPlane3->SetSensitiveDetector(sensDet);
        logicPlane4->SetSensitiveDetector(sensDet);
        logicPlane5->SetSensitiveDetector(sensDet);
        logicPlane6->SetSensitiveDetector(sensDet);
    }
}
#include "PrimaryGenerator.hh"
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
=======

>>>>>>> 8a2e4b8 (Cleaned up the simulation files. Removed the Truth En tree as it was not used further in the analysis chain. Additionally, general clean up in terms of branch renaming. It most probably will impact the in_pixel_plots script. However the default analysis chain has already been modified to account for these changes.)
=======
=======
>>>>>>> ebfd7f7 (Added the MC truth primary vertex of particle gun from develop_Vlad branch)
//Constructor
PrimaryGenerator::PrimaryGenerator(SimFlags* flags) : fFlag(flags), fEventCounter(0)
{
    fParticleGun = new G4ParticleGun(1); // 1 particle per event
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> ebfd7f7 (Added the MC truth primary vertex of particle gun from develop_Vlad branch)
    // Particle Direction (momentum)
    G4double px = fFlag->particleMomentumX;
    G4double py = fFlag->particleMomentumY;
    G4double pz = fFlag->particleMomentumZ;
    G4ThreeVector mom(px,py,pz);
    fParticleGun->SetParticleMomentumDirection(mom);
<<<<<<< HEAD
>>>>>>> 8149767 (Added digitization + tracking + clustering + analysis in an automatic fashion for each run)
=======
>>>>>>> 689c0d7 (Added the MC truth primary vertex of particle gun from develop_Vlad branch)
>>>>>>> ebfd7f7 (Added the MC truth primary vertex of particle gun from develop_Vlad branch)

//Constructor
PrimaryGenerator::PrimaryGenerator(SimFlags* flags) : fFlag(flags), fEventCounter(0)
{
    fParticleGun = new G4ParticleGun(1); // 1 particle per event
=======
>>>>>>> 8a2e4b8 (Cleaned up the simulation files. Removed the Truth En tree as it was not used further in the analysis chain. Additionally, general clean up in terms of branch renaming. It most probably will impact the in_pixel_plots script. However the default analysis chain has already been modified to account for these changes.)
    // Set primary particle energy if a constant value is passed. If not, an energy method is used below
    if(!fFlag->particleEnergy.empty() && fFlag->particleEnergy.find_first_not_of("0123456789") == std::string::npos)
    {
        G4double particleEnergy = std::stod(fFlag->particleEnergy) * GeV;
        fParticleGun->SetParticleEnergy(particleEnergy);  //1.4608 * MeV K-40   0.661 * MeV Cs-137
    }
    else
    {
        trowError("PrimaryGenerator::PrimaryGenerator", "Sampling Failure", "Non-constant energy disrtibution selected but none is yet implemented.");                  
    }
    
    std::string particleType = fFlag->particleType;
    
    G4ParticleTable *particleTable = G4ParticleTable::GetParticleTable();
    G4ParticleDefinition *particle = particleTable->FindParticle(particleType);
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 8a2e4b8 (Cleaned up the simulation files. Removed the Truth En tree as it was not used further in the analysis chain. Additionally, general clean up in terms of branch renaming. It most probably will impact the in_pixel_plots script. However the default analysis chain has already been modified to account for these changes.)

    if (!particle)
    {
        trowError("PrimaryGenerator::PrimaryGenerator", "Sampling Failure", "Given particle type does not match any predefined GEANT4 value.");
    }

    fParticleGun->SetParticleDefinition(particle);
<<<<<<< HEAD
=======
    fParticleGun->SetParticleDefinition(particle);   
>>>>>>> 66f7594 (DEBUG)
=======
>>>>>>> 8a2e4b8 (Cleaned up the simulation files. Removed the Truth En tree as it was not used further in the analysis chain. Additionally, general clean up in terms of branch renaming. It most probably will impact the in_pixel_plots script. However the default analysis chain has already been modified to account for these changes.)
}
// Destructor
PrimaryGenerator::~PrimaryGenerator()
{
    delete fParticleGun;
}
// circular beam modeling
G4ThreeVector PrimaryGenerator::GetRandomPointOnCircle(G4double radius, G4ThreeVector center)
{
    while (true)
    {
        G4double r = std::sqrt(G4UniformRand()) * radius;

        // Uniform angle
        G4double phi = 2 * CLHEP::pi * G4UniformRand();
        // Coordinates in XY plane
        G4double x = r * std::cos(phi);
        G4double y = r * std::sin(phi);
        G4double z = 0.0;

        return center + G4ThreeVector(x, y, z);
    }
}

G4ThreeVector PrimaryGenerator::GetRandomPointOnRectangle(G4double height, G4double thickness, G4ThreeVector center)
{
    G4double halfHeight = height / 2.0;
    G4double halfThickness = thickness / 2.0;

    G4double x = center.x() + (2.0 * G4UniformRand() - 1.0) * halfThickness;
    G4double y = center.y() + (2.0 * G4UniformRand() - 1.0) * halfHeight;
    G4double z = center.z();

    return G4ThreeVector(x, y, z);
}

void PrimaryGenerator::GeneratePrimaries(G4Event *oneEvent)
{
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> ebfd7f7 (Added the MC truth primary vertex of particle gun from develop_Vlad branch)
    if(fFlag->verbosePG) std::cout << "This event contains " << fFlag->particleCount << " particles with:" << std::endl;
    for(int ev = 0; ev < fFlag->particleCount; ev++)
    {
        // Particle Direction (momentum)
        G4double px = fFlag->particleMomentumX;
        G4double py = fFlag->particleMomentumY;
        G4double pz = fFlag->particleMomentumZ;
        G4ThreeVector mom(px,py,pz);
        fParticleGun->SetParticleMomentumDirection(mom);

        double beamWidth = fFlag->sourceRadius *mm;
        G4double x = fFlag->beamXOffset *cm;
        G4double y = fFlag->beamYOffset *cm;
        G4double z = fFlag->beamZOffset *cm;
        G4ThreeVector pos;
        // Particle circular beam simulation
        if(fFlag->beamGeometry == "pencil")
        {
            pos = G4ThreeVector(x, y, z);
        }
        else if(fFlag->beamGeometry == "circle")
        {
            pos = GetRandomPointOnCircle(0.5 *beamWidth, G4ThreeVector(x, y, z));
        }
        else if(fFlag->beamGeometry == "rectangle")
        {
            pos = GetRandomPointOnRectangle(beamWidth, beamWidth, G4ThreeVector(x, y, z));
        }
        else if (fFlag->beamGeometry == "granularBeam")
        {
            pos = G4ThreeVector(x + ev * 72.8 *um, y, z);
        }
        else
        {
            trowError("PhMattPrimaryGenerator::GeneratePrimaries", "Sampling Failure", "Requested beam geometry not found.");
        }

        fParticleGun->SetParticlePosition(pos);

=======
=======
    // If you look at this you are probably wondering why bother doing a for loop when you can simply set a higher number in 
    // the particle gun argument. The answer why we dont do that is: Random sampling happens only outside the constructor. 
>>>>>>> 66f7594 (DEBUG)
=======
    if(fFlag->verbosePG) std::cout << "This event contains " << fFlag->particleCount << " particles with:" << std::endl;
>>>>>>> 8a2e4b8 (Cleaned up the simulation files. Removed the Truth En tree as it was not used further in the analysis chain. Additionally, general clean up in terms of branch renaming. It most probably will impact the in_pixel_plots script. However the default analysis chain has already been modified to account for these changes.)
    for(int ev = 0; ev < fFlag->particleCount; ev++)
    {
        // Particle Direction (momentum)
        G4double px = fFlag->particleMomentumX;
        G4double py = fFlag->particleMomentumY;
        G4double pz = fFlag->particleMomentumZ;
        G4ThreeVector mom(px,py,pz);
        fParticleGun->SetParticleMomentumDirection(mom);

        double beamWidth = fFlag->sourceRadius *mm;
        G4double x = fFlag->beamXOffset *cm;
        G4double y = fFlag->beamYOffset *cm;
        G4double z = fFlag->beamZOffset *cm;
        G4ThreeVector pos;
        // Particle circular beam simulation
        if(fFlag->beamGeometry == "pencil")
        {
            pos = G4ThreeVector(x, y, z);
        }
        else if(fFlag->beamGeometry == "circle")
        {
            pos = GetRandomPointOnCircle(0.5 *beamWidth, G4ThreeVector(x, y, z));
        }
        else if(fFlag->beamGeometry == "rectangle")
        {
            pos = GetRandomPointOnRectangle(beamWidth, beamWidth, G4ThreeVector(x, y, z));
        }
        else if (fFlag->beamGeometry == "granularBeam")
        {
            pos = G4ThreeVector(x + ev * 72.8 *um, y, z);
        }
        else
        {
            trowError("PhMattPrimaryGenerator::GeneratePrimaries", "Sampling Failure", "Requested beam geometry not found.");
        }

        fParticleGun->SetParticlePosition(pos);
<<<<<<< HEAD
<<<<<<< HEAD
        // Curently we hardcode a particle gun with 100 KHz frequency
        //fParticleGun->SetParticleTime(fEventCounter * 10.0 * us); 

>>>>>>> 8149767 (Added digitization + tracking + clustering + analysis in an automatic fashion for each run)
=======
>>>>>>> 66f7594 (DEBUG)
=======

>>>>>>> 8a2e4b8 (Cleaned up the simulation files. Removed the Truth En tree as it was not used further in the analysis chain. Additionally, general clean up in terms of branch renaming. It most probably will impact the in_pixel_plots script. However the default analysis chain has already been modified to account for these changes.)
        G4int evtID = oneEvent->GetEventID();
        double offSet =  fFlag->intraSpillOffset;
        fParticleGun->SetParticleTime(evtID * fFlag->beamVeto *ns + offSet *ns); // This is the only thread safe way to do this. Multithreading messes up life as always

        // Save Vertex Info
        G4AnalysisManager *analysisManager = G4AnalysisManager::Instance();
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 8a2e4b8 (Cleaned up the simulation files. Removed the Truth En tree as it was not used further in the analysis chain. Additionally, general clean up in terms of branch renaming. It most probably will impact the in_pixel_plots script. However the default analysis chain has already been modified to account for these changes.)
        analysisManager->FillNtupleIColumn(2, 0, evtID);
        analysisManager->FillNtupleDColumn(2, 1, pos[0]);
        analysisManager->FillNtupleDColumn(2, 2, pos[1]);
        analysisManager->FillNtupleDColumn(2, 3, pos[2]);
        analysisManager->FillNtupleDColumn(2, 4, evtID * fFlag->beamVeto * ns);
        analysisManager->FillNtupleDColumn(2, 5, std::stod(fFlag->particleEnergy));
        analysisManager->AddNtupleRow(2); 
<<<<<<< HEAD
=======
        analysisManager->FillNtupleIColumn(4, 0, evtID);
        analysisManager->FillNtupleDColumn(4, 1, pos[0]);
        analysisManager->FillNtupleDColumn(4, 2, pos[1]);
        analysisManager->FillNtupleDColumn(4, 3, pos[2]);
        analysisManager->FillNtupleDColumn(4, 4, evtID * fFlag->beamVeto * ns);
        analysisManager->AddNtupleRow(4); 
=======
>>>>>>> 8a2e4b8 (Cleaned up the simulation files. Removed the Truth En tree as it was not used further in the analysis chain. Additionally, general clean up in terms of branch renaming. It most probably will impact the in_pixel_plots script. However the default analysis chain has already been modified to account for these changes.)

<<<<<<< HEAD



>>>>>>> 8149767 (Added digitization + tracking + clustering + analysis in an automatic fashion for each run)

=======
>>>>>>> 66f7594 (DEBUG)
        // Create Vertex
        fParticleGun->GeneratePrimaryVertex(oneEvent);
        fEventCounter++;
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 8a2e4b8 (Cleaned up the simulation files. Removed the Truth En tree as it was not used further in the analysis chain. Additionally, general clean up in terms of branch renaming. It most probably will impact the in_pixel_plots script. However the default analysis chain has already been modified to account for these changes.)

        if(fFlag->verbosePG) std::cout << "              - " <<"Type: " << fFlag->particleType << "; X: " << pos[0] << "; Y: " << pos[1] << "; Z: " << pos[3] 
                              << "; pX: " << px << "; pY: " << py << "; pZ: " << pz << "; Energy: " << fFlag->particleEnergy;
                              
<<<<<<< HEAD
=======
>>>>>>> 8149767 (Added digitization + tracking + clustering + analysis in an automatic fashion for each run)
=======
>>>>>>> 8a2e4b8 (Cleaned up the simulation files. Removed the Truth En tree as it was not used further in the analysis chain. Additionally, general clean up in terms of branch renaming. It most probably will impact the in_pixel_plots script. However the default analysis chain has already been modified to account for these changes.)
    }
=======
    for(int ev = 0; ev < fFlag->particleCount; ev++)
    {
        double beamWidth = fFlag->sourceRadius *mm;
        G4double x = fFlag->beamXOffset *cm;
        G4double y = fFlag->beamYOffset *cm;
        G4double z = fFlag->beamZOffset *cm;
        G4ThreeVector pos;
        // Particle circular beam simulation
        if(fFlag->beamGeometry == "pencil")
        {
            pos = G4ThreeVector(x, y, z);
        }
        else if(fFlag->beamGeometry == "circle")
        {
            pos = GetRandomPointOnCircle(0.5 *beamWidth, G4ThreeVector(x, y, z));
        }
        else if(fFlag->beamGeometry == "rectangle")
        {
            pos = GetRandomPointOnRectangle(beamWidth, beamWidth, G4ThreeVector(x, y, z));
        }
        else if (fFlag->beamGeometry == "granularBeam")
        {
            pos = G4ThreeVector(x + ev * 72.8 *um, y, z);
        }
        else
        {
            G4Exception("PhMattPrimaryGenerator::GeneratePrimaries", "SamplingFailure", FatalException,
                "Requested geometry not found.");
        }

        fParticleGun->SetParticlePosition(pos);
        // Curently we hardcode a particle gun with 100 KHz frequency
        //fParticleGun->SetParticleTime(fEventCounter * 10.0 * us); 

        G4int evtID = oneEvent->GetEventID();
        double offSet =  fFlag->intraSpillOffset;
        fParticleGun->SetParticleTime(evtID * fFlag->beamVeto *ns + offSet *ns); // This is the only thread safe way to do this. Multithreading messes up life as always
>>>>>>> 689c0d7 (Added the MC truth primary vertex of particle gun from develop_Vlad branch)

        // Save Vertex Info
        G4AnalysisManager *analysisManager = G4AnalysisManager::Instance();
        analysisManager->FillNtupleIColumn(4, 0, evtID);
        analysisManager->FillNtupleDColumn(4, 1, pos[0]);
        analysisManager->FillNtupleDColumn(4, 2, pos[1]);
        analysisManager->FillNtupleDColumn(4, 3, pos[2]);
        analysisManager->FillNtupleDColumn(4, 4, evtID * fFlag->beamVeto * ns);
        analysisManager->AddNtupleRow(4); 


        // Create Vertex
        fParticleGun->GeneratePrimaryVertex(oneEvent);
        fEventCounter++;
    }
}
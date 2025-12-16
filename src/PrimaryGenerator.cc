#include "PrimaryGenerator.hh"

//Constructor
PrimaryGenerator::PrimaryGenerator(SimFlags* flags) : fFlag(flags), fEventCounter(0)
{
    fParticleGun = new G4ParticleGun(1); // 1 particle per event
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

    if (!particle)
    {
        trowError("PrimaryGenerator::PrimaryGenerator", "Sampling Failure", "Given particle type does not match any predefined GEANT4 value.");
    }

    fParticleGun->SetParticleDefinition(particle);
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

G4double PrimaryGenerator::GetRandomPointInLine( G4double xMin, G4double xMax)
{
    return xMin + (xMax - xMin) * G4UniformRand();
}

void PrimaryGenerator::GeneratePrimaries(G4Event *oneEvent)
{
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
        else if (fFlag->beamGeometry == "DColScanX")
        {
            // Currently the defualt is to scan the second double column for fixed y and z
            pos = G4ThreeVector(GetRandomPointInLine(4.07302 *cm, 4.07302 *cm + 600*um), y, z);
        }
        else if (fFlag->beamGeometry == "DColScanY")
        {
            // Currently the defualt is to scan the second double column for fixed y and z
            pos = G4ThreeVector(x, GetRandomPointInLine(4.27302 *cm, 4.27302 *cm + 36.4*um), z);
        }
        else
        {
            trowError("PhMattPrimaryGenerator::GeneratePrimaries", "Sampling Failure", "Requested beam geometry not found.");
        }

        fParticleGun->SetParticlePosition(pos);

        G4int evtID = oneEvent->GetEventID();
        double offSet =  fFlag->intraSpillOffset;
        double particleTime = evtID * fFlag->beamVeto *ns + offSet *ns;
        fParticleGun->SetParticleTime(particleTime); // This is the only thread safe way to do this. Multithreading messes up life as always

        // Save Vertex Info
        G4AnalysisManager *analysisManager = G4AnalysisManager::Instance();
        analysisManager->FillNtupleIColumn(2, 0, evtID);
        analysisManager->FillNtupleDColumn(2, 1, pos[0]);
        analysisManager->FillNtupleDColumn(2, 2, pos[1]);
        analysisManager->FillNtupleDColumn(2, 3, pos[2]);
        analysisManager->FillNtupleDColumn(2, 4, particleTime);
        analysisManager->FillNtupleDColumn(2, 5, std::stod(fFlag->particleEnergy));
        analysisManager->AddNtupleRow(2); 

        // Create Vertex
        fParticleGun->GeneratePrimaryVertex(oneEvent);
        fEventCounter++;

        if(fFlag->verbosePG) std::cout << "              - " <<"Type: " << fFlag->particleType << "; X: " << pos[0] << "; Y: " << pos[1] << "; Z: " << pos[3] 
                              << "; pX: " << px << "; pY: " << py << "; pZ: " << pz << "; Energy: " << fFlag->particleEnergy;
                              
    }

}
#include "PrimaryGenerator.h"
#include "G4RunManager.hh"
// Define particle types
#include "G4ParticleDefinition.hh"
// Particle Gun shoots particles
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4SystemOfUnits.hh"
#include "G4IonTable.hh"
#include "G4RadioactiveDecay.hh"
#include "Config.h"
#include "G4AnalysisManager.hh"

//Constructor
PrimaryGenerator::PrimaryGenerator(const SimFlags* flags) : m_flag(flags), m_particleGun(new G4ParticleGun(1)), m_eventCounter(0)
{
    m_particleGun = new G4ParticleGun(1); // 1 particle per event
    // Set primary particle energy if a constant value is passed. If not, an energy method is used below
    if(!m_flag->particleEnergy.empty() && m_flag->particleEnergy.find_first_not_of("0123456789") == std::string::npos)
    {
        G4double particleEnergy = std::stod(m_flag->particleEnergy) * GeV;
        m_particleGun->SetParticleEnergy(particleEnergy);  //1.4608 * MeV K-40   0.661 * MeV Cs-137
    }
    else
    {
        throwError("PrimaryGenerator::PrimaryGenerator", "Sampling Failure", "Non-constant energy disrtibution selected but none is yet implemented.");                  
    }
    
    std::string particleType = m_flag->particleType;
    
    G4ParticleTable *particleTable = G4ParticleTable::GetParticleTable();
    G4ParticleDefinition *particle = particleTable->FindParticle(particleType);

    if (!particle)
    {
        throwError("PrimaryGenerator::PrimaryGenerator", "Sampling Failure", "Given particle type does not match any predefined GEANT4 value.");
    }

    m_particleGun->SetParticleDefinition(particle);
}
// Destructor
PrimaryGenerator::~PrimaryGenerator()
{
    delete m_particleGun;
}
// circular beam modeling
G4ThreeVector PrimaryGenerator::GetRandomPointOnCircle(G4double radius, const G4ThreeVector center)
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

G4ThreeVector PrimaryGenerator::GetRandomPointOnRectangle(G4double height, G4double thickness, const G4ThreeVector center)
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
    if(m_flag->verbosePG) std::cout << "This event contains " << m_flag->particleCount << " particles with:" << std::endl;
    for(G4int ev = 0; ev < m_flag->particleCount; ev++)
    {

        G4AnalysisManager *analysisManager = G4AnalysisManager::Instance();
        // Particle Direction (momentum)
        G4double px = m_flag->particleMomentumX;
        G4double py = m_flag->particleMomentumY;
        G4double pz = m_flag->particleMomentumZ;
        G4ThreeVector mom(px,py,pz);
        m_particleGun->SetParticleMomentumDirection(mom);

        double beamWidth = m_flag->sourceRadius *mm;
        G4double x = m_flag->beamXOffset *cm;
        G4double y = m_flag->beamYOffset *cm;
        G4double z = m_flag->beamZOffset *cm;
        G4ThreeVector pos;
        // Particle circular beam simulation
        if(m_flag->beamGeometry == "pencil")
        {
            pos = G4ThreeVector(x, y, z);
        }
        else if (m_flag->beamGeometry == "gaussian")
        {
            G4double sigma = m_flag->gausSmearing *cm;
            G4double xGauss = G4RandGauss::shoot(x, sigma);
            G4double yGauss = G4RandGauss::shoot(y, sigma);
            //G4double zGauss = G4RandGauss::shoot(z, sigma);
            pos = G4ThreeVector(xGauss, yGauss, z);

        }
        else if(m_flag->beamGeometry == "circle")
        {
            pos = GetRandomPointOnCircle(0.5 *beamWidth, G4ThreeVector(x, y, z));
        }
        else if(m_flag->beamGeometry == "rectangle")
        {
            pos = GetRandomPointOnRectangle(beamWidth, beamWidth, G4ThreeVector(x, y, z));
        }
        else if (m_flag->beamGeometry == "granularBeam")
        {
            pos = G4ThreeVector(x + ev * 72.8 *um, y, z);
        }
        else if (m_flag->beamGeometry == "DColScanX")
        {
            // Currently the defualt is to scan the second double column for fixed y and z
            pos = G4ThreeVector(GetRandomPointInLine(4.07302 *cm, 4.07302 *cm + 600*um), y, z);
        }
        else if (m_flag->beamGeometry == "DColScanY")
        {
            // Currently the defualt is to scan the second double column for fixed y and z
            pos = G4ThreeVector(x, GetRandomPointInLine(4.27302 *cm, 4.27302 *cm + 36.4*um), z);
        }
        else
        {
            throwError("PhMattPrimaryGenerator::GeneratePrimaries", "Sampling Failure", "Requested beam geometry not found.");
        }

        m_particleGun->SetParticlePosition(pos);

        G4int evtID = oneEvent->GetEventID();
        double offSet =  m_flag->intraSpillOffset;
        double particleTime = evtID * m_flag->beamVeto *ns + offSet *ns;
        m_particleGun->SetParticleTime(particleTime); // This is the only thread safe way to do this. Multithreading messes up life as always

        // Save Vertex Info
        analysisManager->FillNtupleIColumn(2, 0, evtID);
        analysisManager->FillNtupleDColumn(2, 1, pos[0]);
        analysisManager->FillNtupleDColumn(2, 2, pos[1]);
        analysisManager->FillNtupleDColumn(2, 3, pos[2]);
        analysisManager->FillNtupleDColumn(2, 4, particleTime);
        analysisManager->FillNtupleDColumn(2, 5, std::stod(m_flag->particleEnergy));
        analysisManager->AddNtupleRow(2); 

        // Create Vertex
        m_particleGun->GeneratePrimaryVertex(oneEvent);
        m_eventCounter++;

        if(m_flag->verbosePG) std::cout << " - " <<"Type: " << m_flag->particleType << "; X: " << pos[0] << "; Y: " << pos[1] << "; Z: " << pos[2] 
                              << "; pX: " << px << "; pY: " << py << "; pZ: " << pz << "; Energy: " << m_flag->particleEnergy;
                              
    }

}
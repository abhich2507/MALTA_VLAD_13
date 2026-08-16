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
#include "CLHEP/Random/RandPoisson.h"

//Constructor
PrimaryGenerator::PrimaryGenerator(const SimFlags* flags) : m_flag(flags), m_particleGun(new G4ParticleGun(1)), m_eventCounter(0)
{
    m_particleGun = new G4ParticleGun(1); // 1 particle per event
    // Set primary particle energy if a constant value is passed. If not, an energy method is used below
    if(!m_flag->particleEnergy.empty() && m_flag->particleEnergy.find_first_not_of("0123456789") == std::string::npos)
    {
        G4float particleEnergy = std::stod(m_flag->particleEnergy) * GeV;
        m_particleGun->SetParticleEnergy(particleEnergy);  //1.4608 * MeV K-40   0.661 * MeV Cs-137
    }
    else if(m_flag->particleEnergy == "log")
    {
        G4float E0 = std::stod(m_flag->particleEnergy) * GeV;
        G4float particleEnergy = -E0 * std::log(G4UniformRand());
        m_particleGun->SetParticleEnergy(particleEnergy);
    }
    else
    {
        throwError("PrimaryGenerator::PrimaryGenerator", "Sampling Failure", "Non-constant energy disrtibution selected but none is yet implemented.");                  
    }
    G4String particleType{};
    if (m_flag->particleType == "pionMix")
    {
        particleType = (G4UniformRand() < 0.5) ? "pi+" : "pi-";
    }
    else particleType = m_flag->particleType;
    
    G4ParticleTable *particleTable = G4ParticleTable::GetParticleTable();
    G4ParticleDefinition *particle = particleTable->FindParticle(particleType);

    if (!particle)
    {
        throwError("PrimaryGenerator::PrimaryGenerator", "Sampling Failure", "Given particle type does not match any predefined GEANT4 value.");
    }

    m_particleGun->SetParticleDefinition(particle);

    itkParticlePop = ImportITK(m_flag->itkInput, m_flag->itkLayer, m_flag->itkZ);

}
// Destructor
PrimaryGenerator::~PrimaryGenerator()
{
    delete m_particleGun;
}
// circular beam modeling
G4ThreeVector PrimaryGenerator::GetRandomPointOnCircle(G4float radius, const G4ThreeVector center)
{
    G4float r = std::sqrt(G4UniformRand()) * radius;

    // Uniform angle
    G4float phi = 2 * CLHEP::pi * G4UniformRand();
    // Coordinates in XY plane
    G4float x = r * std::cos(phi);
    G4float y = r * std::sin(phi);
    G4float z = 0.0;

    return center + G4ThreeVector(x, y, z);
}

G4ThreeVector PrimaryGenerator::GetRandomPointOnRectangle(G4float height, G4float thickness, const G4ThreeVector center)
{
    G4float halfHeight = height / 2.0;
    G4float halfThickness = thickness / 2.0;

    G4float x = center.x() + (2.0 * G4UniformRand() - 1.0) * halfThickness;
    G4float y = center.y() + (2.0 * G4UniformRand() - 1.0) * halfHeight;
    G4float z = center.z();

    return G4ThreeVector(x, y, z);
}

G4float PrimaryGenerator::GetRandomPointInLine( G4float xMin, G4float xMax)
{
    return xMin + (xMax - xMin) * G4UniformRand();
}

G4double PrimaryGenerator::ImportITK(G4String filename, int layer, double z)
{

    std::ifstream file("../ITK_Input/" + filename + ".csv");
    //std::cout << "../ITK_Input/" + filename << std::endl;
    G4double output;

    if (!file) 
    {
        throw std::runtime_error("Cannot open ITK Input file!!!!!");
    }

    std::string line;

    // skip header
    std::getline(file, line);

    while (std::getline(file, line))
    {
        if (line.empty()) continue;
        std::stringstream ss(line);
        std::string value;
        G4double x,y;
        G4int itkLayer;

        // read comma-separated values
        std::getline(ss, value, ','); x           = std::stod(value);
        std::getline(ss, value, ','); y           = std::stod(value);
        std::getline(ss, value, ','); itkLayer    = std::stoi(value);
        if (itkLayer == layer && x >= z)
        {
            output = y;
            break;
        }
    }

    return output;
}


void PrimaryGenerator::GeneratePrimaries(G4Event *oneEvent)
{
    if(m_flag->verbosePG) std::cout << "This event contains " << m_flag->particleCount << " particles with:" << std::endl;
    int particleNum{};
    if(m_flag->itkEnable == true)
    {
        // itkParticlePop [part/ mm^2] * PileUp Scaling * Sensor Area [cm^2]. Is not yet scaled for multichip.
        particleNum = CLHEP::RandPoisson::shoot(itkParticlePop * m_flag->pileUpScale * m_flag->detectorSizeX * m_flag->detectorSizeY * 100);
        //std::cout << "itkPop: " << itkParticlePop <<  "; Mean: " << itkParticlePop * m_flag->detectorSizeX * m_flag->detectorSizeY * 100 << "; Poisson sample: " << particleNum << std::endl;
    }
    else
    {
        particleNum = m_flag->particleCount;
    }
    for(G4int ev = 0; ev < particleNum; ev++)
    {
        if(m_flag->largeScaleFlag == "EIC_FMT")
        {
            // Signal (ev == 0) and background particles are taken from the config file.
            // Energies in the config are given in GeV.
            G4String particleType{};
            G4double energyValue{};
            if (ev == 0) 
            {
                particleType = m_flag->particleType;
                energyValue = std::stod(m_flag->particleEnergy) * GeV;
            }
            else
            {
                particleType = m_flag->bkgparticleType;
                energyValue = std::stod(m_flag->bkgparticleEnergy) * GeV;
            }

            G4ParticleTable *particleTable = G4ParticleTable::GetParticleTable();
            G4ParticleDefinition *particle = particleTable->FindParticle(particleType);
            if (!particle)
            {
                throwError("PrimaryGenerator::GeneratePrimaries", "Sampling Failure", "Given particle type does not match any predefined GEANT4 value.");
            }
            m_particleGun->SetParticleDefinition(particle);
            m_particleGun->SetParticleEnergy(energyValue);
        }

        G4AnalysisManager *analysisManager = G4AnalysisManager::Instance();
        // Particle Direction (momentum)
        G4float px = m_flag->particleMomentumX;
        G4float py = m_flag->particleMomentumY;
        G4float pz = m_flag->particleMomentumZ;
        G4ThreeVector mom(px,py,pz);
        m_particleGun->SetParticleMomentumDirection(mom);

        float beamWidth = m_flag->sourceRadius *mm;
        float beamWidthX = m_flag->sourceRadiusX *mm;
        float beamWidthY = m_flag->sourceRadiusY *mm;
        G4float x = m_flag->beamXOffset *cm;
        G4float y = m_flag->beamYOffset *cm;
        G4float z = m_flag->beamZOffset *cm;
        G4ThreeVector pos;
        // Particle circular beam simulation
        if(m_flag->beamGeometry == "pencil")
        {
            pos = G4ThreeVector(x, y, z);
        }
        else if (m_flag->beamGeometry == "gaussian")
        {
            G4float sigma = m_flag->gausSmearing *cm;
            G4float xGauss = G4RandGauss::shoot(x, sigma);
            G4float yGauss = G4RandGauss::shoot(y, sigma);
            //G4float zGauss = G4RandGauss::shoot(z, sigma);
            pos = G4ThreeVector(xGauss, yGauss, z);

        }
        else if(m_flag->beamGeometry == "circle")
        {
            pos = GetRandomPointOnCircle(0.5 *beamWidth, G4ThreeVector(x, y, z));
        }
        else if(m_flag->beamGeometry == "rectangle")
        {  
           pos = GetRandomPointOnRectangle(beamWidthX, beamWidthY, G4ThreeVector(x, y, z));
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
        
        float offSet{};
        if(m_flag->largeScaleFlag == "EIC_FMT") offSet =  G4UniformRand() * 2000.0 * ns;
        else offSet =  m_flag->intraSpillOffset;

        float particleTime = evtID * m_flag->beamVeto *ns + offSet *ns;
        m_particleGun->SetParticleTime(particleTime); // This is the only thread safe way to do this. Multithreading messes up life as always

        // Save Vertex Info
        analysisManager->FillNtupleIColumn(1, 0, evtID);
        analysisManager->FillNtupleFColumn(1, 1, pos[0]);
        analysisManager->FillNtupleFColumn(1, 2, pos[1]);
        analysisManager->FillNtupleFColumn(1, 3, pos[2]);
        analysisManager->FillNtupleFColumn(1, 4, particleTime);
        //analysisManager->FillNtupleFColumn(1, 5, std::stod(m_flag->particleEnergy));
        analysisManager->AddNtupleRow(1); 

        // Create Vertex
        m_particleGun->GeneratePrimaryVertex(oneEvent);
        m_eventCounter++;

        if(m_flag->verbosePG) std::cout << " - " <<"Type: " << m_flag->particleType << "; X: " << pos[0] << "; Y: " << pos[1] << "; Z: " << pos[2] 
                              << "; pX: " << px << "; pY: " << py << "; pZ: " << pz << "; Energy: " << m_flag->particleEnergy;
                              
    }

}
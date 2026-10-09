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
#include "GenUtil.h"
#include "G4AnalysisManager.hh"
#include "CLHEP/Random/RandPoisson.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>

//Constructor
PrimaryGenerator::PrimaryGenerator(const SimFlags* flags) : m_flag(flags), m_particleGun(new G4ParticleGun(1)), m_eventCounter(0)
{
    m_particleGun = new G4ParticleGun(1); // 1 particle per event
    // Set primary particle energy if a constant value is passed. If not, an energy method is used below
    if(!m_flag->particleEnergy.empty() && m_flag->particleEnergy != "log")
    {
        // Accept any numeric string: integer, decimal, sign, scientific notation
        G4float particleEnergy{};
        try
        {
            particleEnergy = std::stod(m_flag->particleEnergy) * GeV;
        }
        catch (...)
        {
            throwError("PrimaryGenerator::PrimaryGenerator", "Sampling Failure",
                       "particleEnergy '" + m_flag->particleEnergy + "' is not a valid number and no energy distribution with this name is implemented.");
        }
        m_particleGun->SetParticleEnergy(particleEnergy);  //1.4608 * MeV K-40   0.661 * MeV Cs-137

    }
    else if(m_flag->particleEnergy == "log")
    {
        // A logarithmic energy distribution is not implemented. Previously this
        // branch crashed on std::stod("log"); fail cleanly instead.
        throwError("PrimaryGenerator::PrimaryGenerator", "Sampling Failure",
                   "The 'log' energy distribution is not yet implemented.");
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
    if (m_flag->largeScaleFlag == "EIC_FMT")
    {
        m_modules = LoadModules(m_flag->geoFile);
        m_planePositions = GenUtil::GetPlanePositions(m_modules);
    }

    // Background energy mode switch:
    //   bkgenergyDistribution = none -> fixed bkgparticleEnergy (default, all legacy cfgs)
    //   bkgenergyDistribution = CB   -> sample |p| from the Crystal Ball fit
    if (m_flag->bkgenergyDistribution == "CB")
    {
        if (m_flag->bkgMomentumCB.empty())
        {
            throwError("PrimaryGenerator::PrimaryGenerator", "Sampling Failure",
                       "bkgenergyDistribution = CB requires bkgMomentumCB = \"alpha,n,mean,sigma\".");
        }
        double alpha = 0.0, n = 0.0, mean = 0.0, sigma = 0.0;
        char c1 = 0, c2 = 0, c3 = 0;
        std::stringstream ss(m_flag->bkgMomentumCB);
        bool ok = static_cast<bool>(ss >> alpha >> c1 >> n >> c2 >> mean >> c3 >> sigma);
        if (!ok || c1 != ',' || c2 != ',' || c3 != ',' || alpha <= 0.0 || n <= 1.0 || sigma <= 0.0)
        {
            throwError("PrimaryGenerator::PrimaryGenerator", "Sampling Failure",
                       "bkgMomentumCB must be \"alpha,n,mean,sigma\" with alpha>0, n>1, sigma>0: got '" + m_flag->bkgMomentumCB + "'.");
        }
        m_cbCDF = BuildCBCDF(alpha, n, mean, sigma);
        m_hasBkgMomentum = !m_cbCDF.empty();
    }


}
// Destructor
PrimaryGenerator::~PrimaryGenerator()
{
    delete m_particleGun;
}

PrimaryGenerator::CDF PrimaryGenerator::BuildCBCDF(double alpha, double n, double mean, double sigma) const
{
    const int npts = 20000;
    const double pmin = std::max(0.0, mean - 10.0 * sigma);
    const double pmax = mean + 200.0 * sigma;

    const double A = std::pow(n / alpha, n) * std::exp(-0.5 * alpha * alpha);
    const double B = n / alpha - alpha;

    auto density = [&](double x) {
        double t = (x - mean) / sigma;
        return (t <= alpha) ? std::exp(-0.5 * t * t) : A * std::pow(B + t, -n);
    };

    CDF cdf;
    cdf.reserve(npts);
    const double dx = (pmax - pmin) / (npts - 1);
    double cum = 0.0;
    double fPrev = density(pmin);
    cdf.emplace_back(pmin, 0.0);
    for (int i = 1; i < npts; ++i)
    {
        double x = pmin + i * dx;
        double f = density(x);
        cum += 0.5 * (fPrev + f) * dx;
        cdf.emplace_back(x, cum);
        fPrev = f;
    }
    if (cum <= 0.0)
    {
        throwError("PrimaryGenerator::BuildCBCDF", "Sampling Failure",
                   "Crystal Ball CDF integrates to zero.");
    }
    for (auto& [x, c] : cdf) c /= cum;
    return cdf;
}

double PrimaryGenerator::SampleCB(const CDF& cdf) const
{
    double u = G4UniformRand();                       // thread-local CLHEP engine
    auto it = std::lower_bound(cdf.begin(), cdf.end(), u,
        [](const std::pair<double,double>& e, double v){ return e.second < v; });
    if (it == cdf.begin()) return cdf.front().first;
    if (it == cdf.end())   return cdf.back().first;
    auto prev = it - 1;
    double x0 = prev->first, c0 = prev->second;
    double x1 = it->first,  c1 = it->second;
    double frac = (c1 > c0) ? (u - c0) / (c1 - c0) : 0.0;
    return x0 + frac * (x1 - x0);
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

    G4float y = center.y() + (2.0 * G4UniformRand() - 1.0) * halfThickness;
    G4float x = center.x() + (2.0 * G4UniformRand() - 1.0) * halfHeight;
    G4float z = center.z();

    return G4ThreeVector(x, y, z);
}

G4ThreeVector PrimaryGenerator::GetRandomPointInBox(G4float xMin, G4float xMax, 
                                                    G4float yMin, G4float yMax, 
                                                    G4float zMin, G4float zMax)
{
    G4float x = xMin + (xMax - xMin) * G4UniformRand();
    G4float y = yMin + (yMax - yMin) * G4UniformRand();
    G4float z = zMin + (zMax - zMin) * G4UniformRand();

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
    else if (m_flag->largeScaleFlag == "EIC_FMT" && m_flag->bkgRateMean >= 0.0)
    {
        // Realistic EIC hit rate: 1 signal + Poisson-distributed background.
        // bkgRateMean is the TOTAL background mean per event, set in the cfg.
        G4int nBkg = CLHEP::RandPoisson::shoot(m_flag->bkgRateMean);
        particleNum = 1 + nBkg;
    }
    else
    {
        particleNum = m_flag->particleCount;
    }
    // Signal particle is a random particle among the total particles, not necessarily the first one
    G4int signalIndex = static_cast<G4int>(G4UniformRand() * particleNum);
    G4int trackID = 0;
    G4int mcFlag = 0;
    for(G4int ev = 0; ev < particleNum; ev++)
    {
        if(m_flag->largeScaleFlag == "EIC_FMT")
        {
            // Signal (ev == signalIndex) and background particles are taken from the config file.
            // Energies in the config are given in GeV.
            G4String particleType{};
            G4double energyValue{};
            if (ev == signalIndex) 
            {   mcFlag = 0;
                particleType = m_flag->particleType;
                energyValue = std::stod(m_flag->particleEnergy) * GeV;
            }
            else
            {   mcFlag = 1;
                
                particleType = m_flag->bkgparticleType;
                energyValue = std::stod(m_flag->bkgparticleEnergy) * GeV;
            }
            trackID++; 
            G4ParticleTable *particleTable = G4ParticleTable::GetParticleTable();
            G4ParticleDefinition *particle = particleTable->FindParticle(particleType);
            if (!particle)
            {
                throwError("PrimaryGenerator::GeneratePrimaries", "Sampling Failure", "Given particle type does not match any predefined GEANT4 value.");
            }
            m_particleGun->SetParticleDefinition(particle);

            if (mcFlag == 1 && m_hasBkgMomentum)
            {
                double p = SampleCB(m_cbCDF);                          // |p| in GeV/c
                double mass = particle->GetPDGMass() / GeV;            // 0.000511 for e-
                energyValue = (std::sqrt(p*p + mass*mass) - mass) * GeV; // kinetic E (MeV)
                G4AnalysisManager::Instance()->FillH1(0, p);           // verify sampled |p| (GeV/c)
            }
            m_particleGun->SetParticleEnergy(energyValue);
        }

        G4AnalysisManager *analysisManager = G4AnalysisManager::Instance();
        // Particle Direction (momentum) — configured direction (used for the signal)
        G4ThreeVector mom(m_flag->particleMomentumX,
                          m_flag->particleMomentumY,
                          m_flag->particleMomentumZ);

        float beamWidth = m_flag->sourceRadius *mm;
        float beamWidthX = m_flag->sourceRadiusX *mm;
        float beamWidthY = m_flag->sourceRadiusY *mm;
        G4float x = m_flag->beamXOffset *cm;
        G4float y = m_flag->beamYOffset *cm;
        G4float z = m_flag->beamZOffset *cm;
        G4ThreeVector pos;
        // Particle circular beam simulation
        if (m_flag->largeScaleFlag == "EIC_FMT" && mcFlag == 1)
        {
            pos = GetRandomPointInBox(m_flag->bkgXMin *cm, m_flag->bkgXMax *cm, 
                                      m_flag->bkgYMin *cm, m_flag->bkgYMax *cm, 
                                      m_flag->bkgZMin *cm, m_flag->bkgZMax *cm);
        }
        
        else if(m_flag->beamGeometry == "pencil")
        {
            pos = G4ThreeVector(x, y, z);
        }
        else if (m_flag->beamGeometry == "gaussian")
        {
            G4float sigma_X = m_flag->gausSmearingX *cm;
            G4float sigma_Y = m_flag->gausSmearingY *cm;
            G4float sigma_Z = m_flag->gausSmearingZ *cm;
            G4float xGauss = G4RandGauss::shoot(x, sigma_X);
            G4float yGauss = G4RandGauss::shoot(y, sigma_Y);
            G4float zGauss = G4RandGauss::shoot(z, sigma_Z); // z used here.
            pos = G4ThreeVector(xGauss, yGauss, zGauss);
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

        // Background (mcFlag == 1): aim so the particle lands on a sensor plane.
        // Region 1 -> plane-0, region 3 -> plane-1, region 2 -> 50/50.
        G4ThreeVector finalDir = mom;
        if (m_flag->largeScaleFlag == "EIC_FMT" && mcFlag == 1)
        {
            finalDir = GenUtil::BackgroundDirection(
                G4ThreeVector(pos.x() / cm, pos.y() / cm, pos.z() / cm),
                m_modules, m_planePositions,
                m_flag->detectorSizeX, m_flag->detectorSizeY);
        }
        m_particleGun->SetParticleMomentumDirection(finalDir);

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
        analysisManager->FillNtupleIColumn(1, 5, trackID);
        analysisManager->FillNtupleIColumn(1, 6, mcFlag);

        // Momentum truth information: full momentum vector (GeV) from the
        // configured direction (px,py,pz) and the gun's kinetic energy.
        G4double ekin = m_particleGun->GetParticleEnergy();                 // MeV
        G4double mass = m_particleGun->GetParticleDefinition()->GetPDGMass(); // MeV
        G4double eTot = ekin + mass;
        G4double pMag = std::sqrt(eTot * eTot - mass * mass);               // MeV/c
        G4double momNorm = finalDir.mag();
        if (momNorm <= 0.0) momNorm = 1.0;
        analysisManager->FillNtupleFColumn(1, 7,  pMag * finalDir.x() / momNorm / GeV);
        analysisManager->FillNtupleFColumn(1, 8,  pMag * finalDir.y() / momNorm / GeV);
        analysisManager->FillNtupleFColumn(1, 9,  pMag * finalDir.z() / momNorm / GeV);
        analysisManager->FillNtupleFColumn(1, 10, pMag / GeV);
        analysisManager->FillNtupleFColumn(1, 11, ekin / GeV);
        analysisManager->AddNtupleRow(1); 

        // Create Vertex
        m_particleGun->GeneratePrimaryVertex(oneEvent);
        m_eventCounter++;

        if(m_flag->verbosePG) std::cout << " - " <<"Type: " << m_flag->particleType << "; X: " << pos[0] << "; Y: " << pos[1] << "; Z: " << pos[2] 
                              << "; pX: " << finalDir.x() << "; pY: " << finalDir.y() << "; pZ: " << finalDir.z() << "; Energy: " << m_flag->particleEnergy;
                              
    }

}
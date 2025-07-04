#include "PrimaryGenerator.hh"
//Constructor
PrimaryGenerator::PrimaryGenerator()
{
    fParticleGun = new G4ParticleGun(1); // 1 particle per event
    // Particle position for a point like source
    
    G4double x = 5. *cm;
    G4double y = 5. *cm - 10 *um;
    G4double z = -10. *cm; // shifted away slightly
    G4ThreeVector pos(x,y,z);
    //fParticleGun->SetParticlePosition(pos);
    

    // Particle Direction (momemntum)
    G4double px = 0.;
    G4double py = 0.;
    G4double pz = 1.;
    G4ThreeVector mom(px,py,pz);
    
    
    
    G4ParticleTable *particleTable = G4ParticleTable::GetParticleTable();
    G4ParticleDefinition *particle = particleTable->FindParticle("proton");

    fParticleGun->SetParticleMomentumDirection(mom);
    fParticleGun->SetParticleEnergy(120 * GeV); 
    fParticleGun->SetParticleDefinition(particle);
    
    
    
}
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
    // Particle circular beam simulation
    
    double beamWidth = 18.6368 *mm; //Half-chip beam
    G4ThreeVector pos = GetRandomPointOnCircle(beamWidth/2, G4ThreeVector(5 *cm, 5 *cm, -10 *cm));
    fParticleGun->SetParticlePosition(pos);
    

    // Particle rectangel beam simulation. Useful for edge simulation
    /*
    G4double thickness = 1.82 *mm;
    G4double height = 1.82 *mm;
    G4ThreeVector pos = GetRandomPointOnRectangle(height, thickness, G4ThreeVector(5 *cm, 5 *cm, -10 *cm));
    fParticleGun->SetParticlePosition(pos);
    */
    // Create Vertex
    fParticleGun->GeneratePrimaryVertex(oneEvent);

}
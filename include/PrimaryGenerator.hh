#ifndef PRIMARYGENERATOR_HH
#define PRIMARYGENERATOR_HH

#include "G4VUserPrimaryGeneratorAction.hh"
// Define particle types
#include "G4ParticleDefinition.hh"
// Particle Gun shoots particles
#include "G4ParticleGun.hh"

#include "G4ParticleTable.hh"
#include "G4SystemOfUnits.hh"
#include "G4IonTable.hh"
#include "G4RadioactiveDecay.hh"

#include "G4AnalysisManager.hh"

class PrimaryGenerator : public G4VUserPrimaryGeneratorAction
{
public:
    PrimaryGenerator();
    ~PrimaryGenerator();

    virtual void GeneratePrimaries(G4Event *);
    
private:
    G4ThreeVector GetRandomPointOnCircle(G4double radius, G4ThreeVector center);
    G4ThreeVector GetRandomPointOnRectangle(G4double height, G4double thickness, G4ThreeVector center);
    G4ParticleGun *fParticleGun;
};
#endif
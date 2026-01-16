#ifndef PRIMARYGENERATOR_HH
#define PRIMARYGENERATOR_HH

#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4RunManager.hh"
// Define particle types
#include "G4ParticleDefinition.hh"
// Particle Gun shoots particles
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4SystemOfUnits.hh"
#include "G4IonTable.hh"
#include "G4RadioactiveDecay.hh"
#include "Config.hh"
#include "G4AnalysisManager.hh"

class PrimaryGenerator : public G4VUserPrimaryGeneratorAction
{
public:
    PrimaryGenerator(SimFlags* flags);
    ~PrimaryGenerator();

    virtual void GeneratePrimaries(G4Event *);
    
private:
    SimFlags* fFlag;
    G4ThreeVector GetRandomPointOnCircle(G4double radius, G4ThreeVector center);
    G4ThreeVector GetRandomPointOnRectangle(G4double height, G4double thickness, G4ThreeVector center);
    G4double GetRandomPointInLine( G4double xMin, G4double xMax);
    G4ParticleGun *fParticleGun;
    G4int fEventCounter;
};
#endif
#ifndef PRIMARYGENERATOR_H
#define PRIMARYGENERATOR_H

#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4ThreeVector.hh"

class SimFlags;
class G4Event;
class G4ParticleGun;

class PrimaryGenerator : public G4VUserPrimaryGeneratorAction
{
public:
    explicit PrimaryGenerator(const SimFlags* flags);
    ~PrimaryGenerator() override;
    void GeneratePrimaries(G4Event* event) override;
    
private:
    G4ThreeVector GetRandomPointOnCircle(G4float radius, const G4ThreeVector center);
    G4ThreeVector GetRandomPointOnRectangle(G4float height, G4float thickness, const G4ThreeVector center);
    G4float GetRandomPointInLine( G4float xMin, G4float xMax);
    const SimFlags* m_flag{};
    G4ParticleGun* m_particleGun{};
    G4int m_eventCounter{0};
};
#endif
#ifndef PHMATTTRACKINGACTION_HH
#define PHMATTTRACKINGACTION_HH

#include "G4UserTrackingAction.hh"
#include "G4Track.hh"
#include "G4ParticleDefinition.hh"
#include "G4ParticleTypes.hh"

class PhMattTrackingAction : public G4UserTrackingAction
{
public:
PhMattTrackingAction();
    virtual ~PhMattTrackingAction();

    virtual void PreUserTrackingAction(const G4Track*);
    virtual void PostUserTrackingAction(const G4Track*);
};

#endif
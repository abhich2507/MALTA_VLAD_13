#ifndef PHMATTACTIONINITIALIATION_HH
#define PHMATTACTIONINITIALIATION_HH

#include "G4VUserActionInitialization.hh"
#include "PhMattPrimaryGenerator.hh"
#include "PhMattRunAction.hh"
#include "PhMattSteppingAction.hh"
#include "PhMattEventAction.hh"
#include "PhMattTrackingAction.hh"

class PhMattActionInitialization: public G4VUserActionInitialization
{
public:
    PhMattActionInitialization();
    ~PhMattActionInitialization();
    //Master thread if multithreading
    virtual void BuildForMaster() const;
    //Single thread
    virtual void Build() const;
};
#endif
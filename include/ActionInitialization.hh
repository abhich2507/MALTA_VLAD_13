#ifndef ACTIONINITIALIATION_HH
#define ACTIONINITIALIATION_HH

#include "G4VUserActionInitialization.hh"
#include "PrimaryGenerator.hh"
#include "RunAction.hh"
#include "Config.hh"
#include "SteppingAction.hh"
#include "EventAction.hh"
#include "TrackingAction.hh"

class ActionInitialization: public G4VUserActionInitialization
{
public:
    ActionInitialization(SimFlags* flags);
    ~ActionInitialization();
    //Master thread if multithreading
    virtual void BuildForMaster() const;
    //Single thread
    virtual void Build() const;

private:
    SimFlags* fFlag;
    
};
#endif
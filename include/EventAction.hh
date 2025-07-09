#ifndef EVENTACTION_HH
#define EVENTACTION_HH


#include "G4UserEventAction.hh"
#include "globals.hh"
#include "G4AutoLock.hh"
#include "G4Event.hh"
#include "G4PrimaryVertex.hh"
#include "G4AnalysisManager.hh"
#include "G4Run.hh"
#include "G4RunManager.hh"
#include <chrono>

class EventAction : public G4UserEventAction
{
public:
    EventAction();
    ~EventAction();

    virtual void BeginOfEventAction(const G4Event*);
    virtual void EndOfEventAction(const G4Event*);
private:
    
};






#endif
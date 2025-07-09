#ifndef STEPPINGACTION_HH
#define STEPPINGACTION_HH

#include "G4UserSteppingAction.hh"
#include "globals.hh"
#include "G4OpBoundaryProcess.hh"
#include "G4ProcessManager.hh"
#include "G4RunManager.hh"
#include "G4AnalysisManager.hh"
#include "G4Proton.hh"
#include <set>


class SteppingAction : public G4UserSteppingAction
{
public:
    SteppingAction();
    ~SteppingAction() override;

    void UserSteppingAction(const G4Step*) override;
private:
    // WARNING FOR ALL WHO READ THIS. If you declare maps globally outside a class, you will have race conditions when multithreding.
    // put in the effort and declare things private
    // Define a set with all the PCB componenets names
    std::set<G4String> pcbVolumeNames = {
        "Outer1", "Outer2", "Cu1", "Cu2", "Cu3", "Cu4", "Cu5", "Cu6", "Cu7", "Cu8", "Cu9", "Cu10", "Middle1", 
        "Middle2" , "Inner1", "Inner2", "Inner3", "Inner4", "Inner5", "Inner6", "Inner7",
    };
    // Define a map for protons that enter/ leave the PCB
    std::map<G4int, G4ThreeVector> entryDirections;
    std::map<G4int, G4ThreeVector> entryPositions;
};

#endif
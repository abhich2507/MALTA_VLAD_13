#ifndef STEPPINGACTION_H
#define STEPPINGACTION_H

#include "G4UserSteppingAction.hh"
#include "G4ThreeVector.hh"
#include "G4String.hh"
#include <set>
#include <map>

class SimFlags;
class G4Step;

class SteppingAction : public G4UserSteppingAction
{
public:
    explicit SteppingAction(const SimFlags* flags);
    ~SteppingAction() override = default;

    void UserSteppingAction(const G4Step*) override;
private:
    // Define a set with all the PCB componenets names
    const SimFlags* m_flag{};
    const std::set<G4String> pcbVolumeNames{
        "Outer1", "Outer2", "Cu1", "Cu2", "Cu3", "Cu4", "Cu5", "Cu6", "Cu7", "Cu8", "Cu9", "Cu10", "Middle1", 
        "Middle2" , "Inner1", "Inner2", "Inner3", "Inner4", "Inner5", "Inner6", "Inner7",
    };
    // Define a map for protons that enter/ leave the PCB
    std::map<G4int, G4ThreeVector> entryDirections{};
    std::map<G4int, G4ThreeVector> entryPositions{};
};

#endif
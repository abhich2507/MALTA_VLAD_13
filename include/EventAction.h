#ifndef EVENTACTION_H
#define EVENTACTION_H

#include "G4UserEventAction.hh"
#include "G4UserEventAction.hh"
#include "G4Types.hh"          // G4double, G4int
#include "G4ThreeVector.hh"    // G4ThreeVector
#include <map>
#include <array>

class G4Event;

struct m_trackEdepTime 
{
  G4float edep = 0.;
  G4float time = 0.;
  G4int planeNum = -1;
  G4int X = -1;
  G4int Y = -1;
  G4int eventNum = -1;
};

class EventAction : public G4UserEventAction
{
public:
    EventAction()  = default;
    ~EventAction() override = default;
    void BeginOfEventAction(const G4Event*) override;
    void EndOfEventAction(const G4Event*) override;
    void addEdep(G4int eventID, G4float energy, G4float timing, G4int planeID, G4int pixX, G4int pixY);

private:
    // trackID, {edep, timing} map
    std::map<std::tuple<G4int, G4int, G4int>, m_trackEdepTime> m_trackEdep{};

};

#endif
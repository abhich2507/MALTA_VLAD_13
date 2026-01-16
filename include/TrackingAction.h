#ifndef TRACKINGACTION_H
#define TRACKINGACTION_H

#include "G4UserTrackingAction.hh"
class G4Track;

class TrackingAction : public G4UserTrackingAction
{
public:
    TrackingAction();
    ~TrackingAction() override = default;
    void PreUserTrackingAction(const G4Track*) override;
    void PostUserTrackingAction(const G4Track*) override;
};

#endif
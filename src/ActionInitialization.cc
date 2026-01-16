#include "ActionInitialization.h"
#include "PrimaryGenerator.h"
#include "RunAction.h"
#include "Config.h"
#include "SteppingAction.h"
#include "EventAction.h"
#include "TrackingAction.h"
#include <cassert>

ActionInitialization::ActionInitialization(const SimFlags* flags) : m_flag(flags)
{
    assert(flags != nullptr);
}
// Histogram will be created in the master thread. runAction needs ot be initialized also in the master thread
void ActionInitialization::BuildForMaster() const
{
    // Initialize Run Action for the master thread
    // GEANT4 takes ownership of user actions
    RunAction *runAction = new RunAction(m_flag);
    SetUserAction(runAction);
}

void ActionInitialization::Build() const
{
    // Initialize Primary Generator
    PrimaryGenerator *generator = new PrimaryGenerator(m_flag);
    SetUserAction(generator);
    // Initialize Run Action
    RunAction *runAction = new RunAction(m_flag);
    SetUserAction(runAction);
    // Initialize Event Action
    EventAction *eventAction = new EventAction();
    SetUserAction(eventAction);
    // Initialize Stepping Action
    SteppingAction *stepAction = new SteppingAction(m_flag);
    SetUserAction(stepAction);
    // Initialize Tracking Action
    TrackingAction *trackingAction = new TrackingAction();
    //Tracking Initialization for customized tracking
    SetUserAction(trackingAction);

}
#include "ActionInitialization.hh"


ActionInitialization::ActionInitialization(SimFlags* flags) : fFlag(flags)
{

}
ActionInitialization::~ActionInitialization()
{

}
// Histogram will be created in the master thread. runAction needs ot be initialized also in the master thread
void ActionInitialization::BuildForMaster() const
{
    // Initialize Run Action for the master thread
    RunAction *runAction = new RunAction(fFlag);
    SetUserAction(runAction);
}

void ActionInitialization::Build() const
{
    // Initialize Primary Generator
    PrimaryGenerator *generator = new PrimaryGenerator(fFlag);
    SetUserAction(generator);
    // Initialize Run Action
    RunAction *runAction = new RunAction(fFlag);
    SetUserAction(runAction);
    // Initialize Event Action
    EventAction *eventAction = new EventAction();
    SetUserAction(eventAction);
    // Initialize Stepping Action
    SteppingAction *stepAction = new SteppingAction(fFlag);
    SetUserAction(stepAction);
    // Initialize Tracking Action
    TrackingAction *trackingAction = new TrackingAction();
    //Tracking Initialization for customized tracking
    SetUserAction(trackingAction);

}
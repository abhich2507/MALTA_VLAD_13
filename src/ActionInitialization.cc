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
    RunAction *runAction = new RunAction(fFlag);
    SetUserAction(runAction);
}

void ActionInitialization::Build() const
{
    PrimaryGenerator *generator = new PrimaryGenerator(fFlag);
    SetUserAction(generator);

    RunAction *runAction = new RunAction(fFlag);
    SetUserAction(runAction);

    EventAction *eventAction = new EventAction();
    SetUserAction(eventAction);

    SteppingAction *stepAction = new SteppingAction();
    SetUserAction(stepAction);

    TrackingAction *trackingAction = new TrackingAction();
    //Tracking Initialization for customized tracking
    SetUserAction(trackingAction);

}
#include "PhMattActionInitialization.hh"


PhMattActionInitialization::PhMattActionInitialization()
{

}
PhMattActionInitialization::~PhMattActionInitialization()
{

}
// Histogram will be created in the master thread. runAction needs ot be initialized also in the master thread
void PhMattActionInitialization::BuildForMaster() const
{
    PhMattRunAction *runAction = new PhMattRunAction();
    SetUserAction(runAction);
}

void PhMattActionInitialization::Build() const
{
    PhMattPrimaryGenerator *generator = new PhMattPrimaryGenerator();
    SetUserAction(generator);

    PhMattRunAction *runAction = new PhMattRunAction();
    SetUserAction(runAction);

    PhMattEventAction *eventAction = new PhMattEventAction();
    SetUserAction(eventAction);

    PhMattSteppingAction *stepAction = new PhMattSteppingAction();
    SetUserAction(stepAction);

    PhMattTrackingAction *trackingAction = new PhMattTrackingAction();
    //Tracking Initialization for customized tracking
    SetUserAction(trackingAction);

}
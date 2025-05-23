#include "PhMattPhysicsList.hh"

// Consrtuctor call
PhMattPhysicsList::PhMattPhysicsList()
// Body of the contructor - contains all the physics for the simulation
{
    // Instantiation of GEANT4 methods to be put in the memory heap. Empty constructors -> ()
    // EM Phyics
    // Average precision Low Computation
    //RegisterPhysics(new G4EmStandardPhysics());
    // High precision, High Computation
    RegisterPhysics(new G4EmStandardPhysics_option4());   // EM Physics
    RegisterPhysics(new G4DecayPhysics());                // Decays
    RegisterPhysics(new G4HadronElasticPhysics());        // Hadron elastic
    RegisterPhysics(new G4HadronPhysicsFTFP_BERT());      // Hadronic inelastic
    RegisterPhysics(new G4StoppingPhysics());             // Stopping
    RegisterPhysics(new G4IonPhysics());                  // Ions

    
}
// Destructor
PhMattPhysicsList::~PhMattPhysicsList()
{
    
}
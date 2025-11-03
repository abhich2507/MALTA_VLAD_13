#include "PhysicsList.hh"
// Constructor call
PhysicsList::PhysicsList(SimFlags* flags) : fFlag(flags)
// Body of the contructor - contains all the physics for the simulation
{
    // Instantiation of GEANT4 methods to be put in the memory heap. Empty constructors -> ()
    if(fFlag->EMPhysics)
    {
        // EM Phyics
        RegisterPhysics(new G4EmStandardPhysics());   // EM Physics
    }
    if(fFlag->hadronPhysics)
    {
        RegisterPhysics(new G4DecayPhysics());                // Decays
        RegisterPhysics(new G4HadronElasticPhysics());        // Hadron elastic
        RegisterPhysics(new G4HadronPhysicsFTFP_BERT());      // Hadronic inelastic
        RegisterPhysics(new G4StoppingPhysics());             // Stopping
        RegisterPhysics(new G4IonPhysics());                  // Ions
    }
    
}
// Destructor
PhysicsList::~PhysicsList()
{
    
}

void PhysicsList::SetCuts()
{
    // Optional: define the range of energies to which production cuts apply
    if(fFlag->setGEANT4Cuts)
    {
        G4ProductionCutsTable::GetProductionCutsTable()->SetEnergyRange(250 * eV, 120 * GeV);

        SetCutValue(fFlag->GEANT4CutValue *um, "gamma");
        SetCutValue(fFlag->GEANT4CutValue *um, "e-");
        SetCutValue(fFlag->GEANT4CutValue *um, "e+");

        // Optionally print the cut values
        if (fFlag->verbosePL)DumpCutValuesTable();
    }

    if(fFlag->GEANT4CutValue >= fFlag->detectorDepth)
    {
        trowWarning("PhysicsList::SetCuts", "Value Warning", "Energy cut value larger or equal than sensitive depth. Energy deposition in a single step.");
    }
}
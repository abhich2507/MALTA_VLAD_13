#include "PhysicsList.h"
#include "G4EmStandardPhysics.hh"
#include "Config.h"
#include "G4DecayPhysics.hh"
#include "G4HadronElasticPhysics.hh"
#include "G4HadronPhysicsFTFP_BERT.hh"
#include "G4StoppingPhysics.hh"
#include "G4IonPhysics.hh"
#include "G4SystemOfUnits.hh"
#include <cassert>

// Constructor call
PhysicsList::PhysicsList(const SimFlags* flags) : m_flag(flags)
// Body of the contructor - contains all the physics for the simulation
{
    assert (m_flag != nullptr);
    // Instantiation of GEANT4 methods to be put in the memory heap. Empty constructors -> ()
    if(m_flag->EMPhysics)
    {
        // EM Phyics
        RegisterPhysics(new G4EmStandardPhysics());   // EM Physics
    }
    if(m_flag->hadronPhysics)
    {
        RegisterPhysics(new G4DecayPhysics());                // Decays
        RegisterPhysics(new G4HadronElasticPhysics());        // Hadron elastic
        RegisterPhysics(new G4HadronPhysicsFTFP_BERT());      // Hadronic inelastic
        RegisterPhysics(new G4StoppingPhysics());             // Stopping
        RegisterPhysics(new G4IonPhysics());                  // Ions
    }
    
}

void PhysicsList::SetCuts()
{
    if(m_flag->GEANT4CutValue >= m_flag->detectorDepth)
    {
        throwWarning("PhysicsList::SetCuts", "Value Warning", "Energy cut value larger or equal than sensitive depth. Energy deposition in a single step.");
    }
    // Optional: define the range of energies to which production cuts apply
    if(m_flag->setGEANT4Cuts)
    {
        G4ProductionCutsTable::GetProductionCutsTable()->SetEnergyRange(250 * eV, 120 * GeV);

        SetCutValue(m_flag->GEANT4CutValue *um, "gamma");
        SetCutValue(m_flag->GEANT4CutValue *um, "e-");
        SetCutValue(m_flag->GEANT4CutValue *um, "e+");

        // Optionally print the cut values
        if (m_flag->verbosePL)DumpCutValuesTable();
    }

}
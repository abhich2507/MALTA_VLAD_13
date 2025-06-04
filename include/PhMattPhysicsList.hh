// Classic trick. Protects against multiple imports
#ifndef PHMATTPHYSICSLIST_HH
#define PHMATTPHYSICSLIST_HH

//Physics Lists
#include "G4VModularPhysicsList.hh"
#include "G4EmStandardPhysics.hh"
#include "G4EmStandardPhysics_option4.hh"
#include "G4DecayPhysics.hh"
#include "G4HadronElasticPhysics.hh"
#include "G4HadronPhysicsFTFP_BERT.hh"
#include "G4StoppingPhysics.hh"
#include "G4IonPhysics.hh"
#include "G4SystemOfUnits.hh"


// Class inherits/extends the public class G4VModularPhysicsList to gain its methods
// and variables. public = all public methods stay public also here
class PhMattPhysicsList : public G4VModularPhysicsList
{
public:
    // Constructor - called when object is created: PhMattPhysicsList::PhMattPhysicsList()
    PhMattPhysicsList();
    // Destructor - called when object goes out of scope
    ~PhMattPhysicsList();
    virtual void SetCuts();
};
// File processed only once per compilation
#endif
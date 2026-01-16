#ifndef PHYSICSLIST_H
#define PHYSICSLIST_H

//Physics Lists
#include "G4VModularPhysicsList.hh"

class SimFlags;


// Class inherits/extends the public class G4VModularPhysicsList to gain its methods
// and variables. public = all public methods stay public also here
class PhysicsList : public G4VModularPhysicsList
{
public:
    // Constructor - called when object is created: PhysicsList::PhysicsList()
    explicit PhysicsList(const SimFlags* flags);
    // Destructor - called when object goes out of scope
    ~PhysicsList() override = default;
    void SetCuts() override;
private:
    const SimFlags* m_flag{};
};
// File processed only once per compilation

#endif
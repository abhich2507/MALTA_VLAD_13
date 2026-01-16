#ifndef ACTIONINITIALIZATION_H
#define ACTIONINITIALIZATION_H

#include "G4VUserActionInitialization.hh"

class SimFlags;

class ActionInitialization: public G4VUserActionInitialization
{
public:
    explicit ActionInitialization(const SimFlags* flags);
    ~ActionInitialization() override = default;
    //Master thread if multithreading
    void BuildForMaster() const override;
    //Single thread
    void Build() const override;

private:
    const SimFlags* m_flag{};
    
};
#endif
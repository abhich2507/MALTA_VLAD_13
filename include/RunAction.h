#ifndef RUNACTION_H
#define RUNACTION_H

#include "G4UserRunAction.hh"

class SimFlags;

class RunAction: public G4UserRunAction
{
public:
    explicit RunAction(const SimFlags* flags);
    ~RunAction() override = default;

    void BeginOfRunAction(const G4Run *) override;
    void EndOfRunAction(const G4Run *) override;
    G4Run* GenerateRun() override;
    
    
private:
    const SimFlags* m_flag{};
    std::string m_outputPath{};
};
#endif
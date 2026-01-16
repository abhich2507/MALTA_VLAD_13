#ifndef RUNACTION_HH
#define RUNACTION_HH

#include "G4UserRunAction.hh"
#include "G4Run.hh"
#include "G4AnalysisManager.hh"
#include "G4SystemOfUnits.hh"
#include "G4UnitsTable.hh"
#include "Config.hh"
#include <filesystem>

class RunAction: public G4UserRunAction
{
public:
    RunAction(SimFlags* flags);
    ~RunAction();

    virtual void BeginOfRunAction(const G4Run *);
    virtual void EndOfRunAction(const G4Run *);
    virtual G4Run* GenerateRun() override;
    
    
private:
    SimFlags* fFlag;
    std::string fOutputPath;
};
#endif
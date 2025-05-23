#ifndef PHMATTRUNACTION_HH
#define PHMATTRUNACTION_HH

#include "G4UserRunAction.hh"
#include "G4Run.hh"
#include "G4AnalysisManager.hh"
#include "G4SystemOfUnits.hh"
#include "G4UnitsTable.hh"

class PhMattRunAction: public G4UserRunAction
{
public:
    PhMattRunAction();
    ~PhMattRunAction();

    virtual void BeginOfRunAction(const G4Run *);
    virtual void EndOfRunAction(const G4Run *);
    virtual G4Run* GenerateRun() override;
};
#endif
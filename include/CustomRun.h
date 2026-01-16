#ifndef CUSTOMRUN_HH
#define CUSTOMRUN_HH

#include "G4Run.hh"

class CustomRun: public G4Run
{
public:
    CustomRun(G4int id)
    {
        SetRunID(id);
    }
};

#endif
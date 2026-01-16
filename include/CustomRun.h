#ifndef CUSTOMRUN_H
#define CUSTOMRUN_H

#include "G4Run.hh"

class CustomRun: public G4Run
{
public:
    explicit CustomRun(G4int id)
    {
        SetRunID(id);
    }
};

#endif
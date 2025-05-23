#ifndef PHMATTCUSTOMRUN_HH
#define PHMATTCUSTOMRUN_HH

#include "G4Run.hh"

class PhMattCustomRun: public G4Run
{
public:
    PhMattCustomRun(G4int id)
    {
        SetRunID(id);
    }
};

#endif
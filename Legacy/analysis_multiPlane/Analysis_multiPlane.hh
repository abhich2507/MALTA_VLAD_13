#pragma once
#include <ROOT/RNTuple.hxx>
#include <TH3D.h>
#include <TFile.h>
#include <iostream>
#include <math.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <chrono>
#include "DigitalProcessing.hh"
#include "Tracking_multiPlane.hh"
// Computes efficiency in percent
double getEff(int Npassed, int Nall);

// Computes efficiency error in percent (binomial)
double getEffErr(int Npassed, int Nall);

// Main simple plots function
void Analysis_multiPlane(int runNumber = 91, std::string saveName = "default");
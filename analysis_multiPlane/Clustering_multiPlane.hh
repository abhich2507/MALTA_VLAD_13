#pragma once
#include <iostream>
#include <ROOT/RNTuple.hxx>
#include <TH3D.h>
#include <TFile.h>
#include <chrono>
#include "DigitalProcessing.hh"

// Main clustering function
void Clustering_multiPlane(int runNumber, std::string saveName = "default");
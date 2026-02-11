
#pragma once
#include <utility>
#include <iostream>
#include <ROOT/RNTuple.hxx>
#include <TH3D.h>
#include <TFile.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <chrono>
#include "DigitalProcessing.hh"

void GenericClustering(double threshold = 200, int runNumber = 102, std::string saveName = "Test");

struct hitCandidate
{
    int pixX;
    int pixY;
    double timing;
};
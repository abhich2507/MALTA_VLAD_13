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
#include "Tracking.hh"


void MALTATracking(double threshold = 200, int runNumber = 102, std::string saveName = "Test");

struct hitCluster
{
    int pixX;
    int pixY;
    int clSize;
    double hitTiming;
};
#pragma once
#include <ROOT/RNTuple.hxx>
#include <TH3D.h>
#include <TFile.h>
#include <iostream>
#include <math.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <chrono>
#include "DigitalProcessing_multiPlane.hh"
#include "Tracking_multiPlane.hh"



struct AnalysisHits
{
    int planeID;
    double x;
    double y;
    int clSize;
    double timing;
    double correctedTiming;
};

enum Hist2DIndex 
{
    kALL = 0,        // explicitly 0
    kPASS,           // 1
    kClSize,         // 2
    kTiming,         // 3
    kALLInPixel,     // 4
    kPASSInPixel,    // 5
    kClSizeInPixel,  // 6
    kTimingInPixel,  // 7
};

enum Hist1DIndex 
{
    kTiming1D = 0,
    kCorrectedTiming,
    kPASSInPixelXProj,
    kPASSInPixelYProj
};

struct AnalyzedHit
{
    double avgEff;
    double errEff;
    double avgTiming;
    double avgClSize;
    double errClSize;
};


void Analysis_multiPlane(double threshold, int runNumber = 91, std::string saveName = "default");
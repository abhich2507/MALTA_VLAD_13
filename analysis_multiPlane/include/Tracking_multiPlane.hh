#pragma once
#include <utility>
#include <iostream>
#include <ROOT/RNTuple.hxx>
#include <TH3D.h>
#include <TFile.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <chrono>
#include "DigitalProcessing_multiPlane.hh"
// Structure to hold track information
struct TrackEntry 
{
    double x;
    double y;
    double z;
    double t;
};

struct DetectorConfig 
{
    double detectorXOffset;
    double detectorYOffset;
    double pixelSize;
    double detectorSizeX;
    double detectorSizeY;
    double momX, momY, momZ;
};

struct Module 
{
    int x, y, z;
    double xoff, yoff, zoff, xrot, yrot, zrot;
    int modID;
};

struct Offset
{
    double x, y, z;
    double xrot, yrot, zrot;
    double R[3][3];
};

struct Vec3
{
    double x, y, z;
};

struct FullTrackInfo
{
    int planeID;
    int trackID;
    double vertexX;
    double vertexY;
    double vertexTime;
    int dutX;
    int dutY;
    int dutNHits;
    double dutTime;
};

struct Residual
{
    double rx;
    double ry;
};

// Main tracking function
void Tracking_multiPlane(double threshold, int runNumber, std::string saveName);
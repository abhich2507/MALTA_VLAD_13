#pragma once
#include <iostream>
#include <ROOT/RNTuple.hxx>
#include <TH3D.h>
#include <TFile.h>
#include <chrono>
#include "DigitalProcessing_multiPlane.hh"
#include "Tracking_multiPlane.hh"


struct Hit
{
    int x;
    int y;
    double t;
};


struct Cluster
{
    int clSize;
    double x;
    double y;
};

struct ClusterState
{
    std::vector<Hit>  cluster;
    double            currentX{};
    double            currentY{};
    int               currentPlaneID{-1};
    int               currentTrackID{-1};
    int               currentPixY{-1};
};

struct ClusteredHit
{
    int planeID;
    double x;
    double y;
    int clSize;
    double timing;
    double corrTiming;
};


// Main clustering function
void Clustering_multiPlane(double threshold, int runNumber, std::string saveName = "default");
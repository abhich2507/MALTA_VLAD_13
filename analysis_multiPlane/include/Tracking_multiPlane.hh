#pragma once
#include <string>

struct TrackEntry 
{
    double x;
    double y;
    double z;
    double t;
    int mcFlag;
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
    int mcFlag;
};
struct Residual
{
    double rx;
    double ry;
};

void Tracking_multiPlane(double threshold, int runNumber, std::string saveName);
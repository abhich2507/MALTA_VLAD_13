#pragma once
#include "ConfigAnalysis.hh"
#include "Tracking_multiPlane.hh"
#include "DigitalProcessing_multiPlane.hh"
#include <string>
#include <utility>
#include <vector>

std::pair<std::vector<FullTrackInfo>, std::vector<Residual>> MatchHits(std::vector<TrackEntry> tracks, std::vector<ProcessedHit> hits, AnaFlags cfg, int runNumber);
Vec3 PixelPositionReconstruction(int pixelX, int pixelY, const DetectorConfig& cfg);
Vec3 IntersectTrackPlane(const Vec3& V,  const DetectorConfig& cfg, Offset& g);
std::pair<double,double> GetSpecificPlaneOffset(int plane, std::string geometry);
double dot(const Vec3& a, const Vec3& b);
void BuildRotationMatrix(Offset& g);
Vec3 ApplyGeometry3D(const Vec3& p, Offset& g);
Vec3 ApplyInverseGeometry3D(const Vec3& global, const Offset& g);
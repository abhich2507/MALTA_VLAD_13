#pragma once
#include "Clustering_multiPlane.hh"
#include "ConfigAnalysis.hh"
#include "Tracking_multiPlane.hh"
#include <string>
#include <utility>
#include <vector>

bool hasHitAt(const std::vector<Hit>& cluster, int x, int y);
Cluster ValidateCluster(std::vector<Hit>& cluster, const DetectorConfig& cfg);
std::pair<double,double> GetClusterPosition(const Cluster& cl, ClusterState& state, const std::string&   clPosMode);
ClusteredHit GetValidCluster(const Cluster& cl, ClusterState& state, const std::pair<double,double> vertex);
void ResetClusterState(ClusterState& state, FullTrackInfo track);
std::vector<std::vector<FullTrackInfo>> GroupHitsByTrack(const std::vector<FullTrackInfo>& hits);
ClusterState BuildClusterState(const std::vector<FullTrackInfo>& track);
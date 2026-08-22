#include "ClusteringUtils.hh"
#include "TrackingUtils.hh"
#include <algorithm>
#include <string>
#include <utility>
#include <vector>

bool hasHitAt(const std::vector<Hit>& cluster, int x, int y)
{
    return std::any_of(cluster.begin(), cluster.end(), [&](const Hit& h)
        {
            return h.x == x && h.y == y;
        });
}
Cluster ValidateCluster(std::vector<Hit>& cluster, const DetectorConfig& cfg)
{
    Cluster result{};
    const std::vector<std::pair<int,int>> diagonals = { {1,1}, {1,-1}, {-1,1}, {-1,-1} };

    for (auto it = cluster.begin(); it != cluster.end(); )
    {
        const int xPos = it->x;
        const int yPos = it->y;
        bool erase = (xPos == -1 || yPos == -1);

        for (const auto& [dx, dy] : diagonals)
        {
            if (!erase &&
                hasHitAt(cluster, xPos + dx, yPos + dy) &&
                !(hasHitAt(cluster, xPos + dx, yPos) ||
                  hasHitAt(cluster, xPos, yPos + dy)))
            {
                erase = true;
            }
        }

        if (erase)
        {
            it = cluster.erase(it);
        }
        else
        {
            ++result.clSize;
            Vec3 pix = PixelPositionReconstruction(xPos, yPos, cfg);
            result.x += pix.x;
            result.y += pix.y;
            ++it;
        }
    }
    return result;
}
std::pair<double,double> GetClusterPosition(const Cluster& cl, ClusterState& state, const std::string&   clPosMode)
{
    if (clPosMode == "COM" && cl.x > 0 && cl.y > 0)
        return { cl.x / cl.clSize, cl.y / cl.clSize};

    return { state.currentX, state.currentY }; // fallback: MC truth
}
ClusteredHit GetValidCluster(const Cluster& cl, ClusterState& state, const std::pair<double,double> vertex)
{
    double vertexX = vertex.first;
    double vertexY = vertex.second;
    int clSize  = cl.clSize;
    int clPlaneID = state.currentPlaneID;
    int clMCFlag = state.currentMCFlag;

    double timing = std::min_element(state.cluster.begin(), state.cluster.end(),
                 [](const Hit& a, const Hit& b){ return a.t < b.t; })->t;

    double correctedTiming = timing - state.currentPixY * 0.0125;

    return {clPlaneID, vertexX, vertexY, clSize, timing, correctedTiming, clMCFlag};

}
void ResetClusterState(ClusterState& state, FullTrackInfo track)
{
    state.cluster.clear();
    state.cluster.push_back({track.dutX, track.dutY, track.dutTime});
    state.currentPlaneID  = track.planeID;
    state.currentTrackID  = track.trackID;
    state.currentMCFlag   = track.mcFlag;
    state.currentPixY     = track.dutY;
    state.currentX        = track.vertexX;
    state.currentY        = track.vertexY;
}
std::vector<std::vector<FullTrackInfo>> GroupHitsByTrack(const std::vector<FullTrackInfo>& hits)
{
    std::vector<std::vector<FullTrackInfo>> tracks;
    for (const auto& hit : hits)
    {
        if (tracks.empty() || hit.trackID != tracks.back().front().trackID)
            tracks.push_back({hit});
        else
            tracks.back().push_back(hit);
    }
    return tracks;
}
ClusterState BuildClusterState(const std::vector<FullTrackInfo>& track)
{
    ClusterState state;
    state.currentPlaneID = track.front().planeID;
    state.currentTrackID = track.front().trackID;
    state.currentMCFlag  = track.front().mcFlag;
    state.currentPixY    = track.front().dutY;
    state.currentX       = track.front().vertexX;
    state.currentY       = track.front().vertexY;
    for (const auto& hit : track)
        state.cluster.push_back({hit.dutX, hit.dutY, hit.dutTime});
    return state;
}
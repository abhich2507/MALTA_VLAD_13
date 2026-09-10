#ifndef GENUTIL_HH
#define GENUTIL_HH

// GenUtil.hh
// Background-generation utilities.
// All positions/sizes are in CM (matching flags.cfg and the geometry CSV).
// Returned directions are unit vectors (dimensionless).

#include "G4ThreeVector.hh"
#include "Randomize.hh"
#include "Config.h"

#include <cmath>
#include <vector>

namespace GenUtil
{
    struct PlanePositions
    {
        bool   valid = false;
        double z0 = 0.0;   // plane-0 (smallest zoff)
        double z1 = 0.0;   // plane-1 (largest zoff)
    };

    // z positions (cm) of the two sensor planes, derived from the geometry modules
    inline PlanePositions GetPlanePositions(const std::vector<Module>& modules)
    {
        PlanePositions p;
        double zmin =  1e30;
        double zmax = -1e30;
        for (const auto& m : modules)
        {
            if (m.zoff < zmin) zmin = m.zoff;
            if (m.zoff > zmax) zmax = m.zoff;
        }
        if (zmin < zmax) { p.valid = true; p.z0 = zmin; p.z1 = zmax; }
        return p;
    }

    // Uniform random point inside a box
    inline G4ThreeVector RandomPointInBox(double xMin, double xMax,
                                          double yMin, double yMax,
                                          double zMin, double zMax)
    {
        return G4ThreeVector(xMin + (xMax - xMin) * G4UniformRand(),
                             yMin + (yMax - yMin) * G4UniformRand(),
                             zMin + (zMax - zMin) * G4UniformRand());
    }

    // Region of a vertex (z in cm): 1 = before plane-0, 2 = between, 3 = after plane-1
    inline int BackgroundRegion(double z, const PlanePositions& p)
    {
        if (z < p.z0) return 1;
        if (z > p.z1) return 3;
        return 2;
    }

    // Target plane z (cm): region 1 -> plane-0, region 3 -> plane-1, region 2 -> 50/50
    inline double PickTargetPlaneZ(int region, const PlanePositions& p)
    {
        if (region == 1) return p.z0;
        if (region == 3) return p.z1;
        return (G4UniformRand() < 0.5) ? p.z0 : p.z1;
    }

    // Random hit point over ALL modules in a plane (cm).
    // Picks a module uniformly, then a point uniformly inside its active area.
    inline G4ThreeVector RandomHitPointOnPlane(double zPlane,
                                               const std::vector<Module>& modules,
                                               double sizeX, double sizeY)
    {
        std::vector<const Module*> planeMods;
        for (const auto& m : modules)
            if (std::abs(m.zoff - zPlane) < 1e-9)
                planeMods.push_back(&m);

        if (planeMods.empty())
            return G4ThreeVector(0.0, 0.0, zPlane);   // fallback

        const Module* m = planeMods[static_cast<size_t>(G4UniformRand() * planeMods.size())
                                    % planeMods.size()];
        double x = m->xoff + (G4UniformRand() - 0.5) * sizeX;
        double y = m->yoff + (G4UniformRand() - 0.5) * sizeY;
        return G4ThreeVector(x, y, zPlane);
    }

    // Unit direction that makes a particle from `vertex` pass through `target`
    inline G4ThreeVector AimAt(const G4ThreeVector& vertex, const G4ThreeVector& target)
    {
        G4ThreeVector d = target - vertex;
        double mag = d.mag();
        if (mag < 1e-12) return G4ThreeVector(0.0, 0.0, 1.0);
        return d / mag;
    }

    // One-shot: background momentum direction for a vertex (cm)
    inline G4ThreeVector BackgroundDirection(const G4ThreeVector& vertexCm,
                                             const std::vector<Module>& modules,
                                             const PlanePositions& p,
                                             double sizeX, double sizeY)
    {
        int    region = BackgroundRegion(vertexCm.z(), p);
        double zplane = PickTargetPlaneZ(region, p);
        G4ThreeVector hit = RandomHitPointOnPlane(zplane, modules, sizeX, sizeY);
        return AimAt(vertexCm, hit);
    }
}

#endif
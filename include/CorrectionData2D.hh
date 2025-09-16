#include <array>
#pragma once

constexpr int XSteps = 32; // 36 ?
constexpr int YSteps = 32;
extern const double spacingX;
extern const double spacingY;
extern double EffMap2D[XSteps][YSteps];

// Lookup table for clustering

const std::array<std::array<std::array<int, 2>, 4>, 4> deltaTable = {{
    // flag = 0b00
    {{{-1, -1}, {0, -1}, {-1, 0}, {0, 0}}},
    // flag = 0b01
    {{{ 0, -1}, {1, -1}, { 0, 0}, {1, 0}}},
    // flag = 0b10
    {{{-1,  0}, {0,  0}, {-1, 1}, {0, 1}}},
    // flag = 0b11
    {{{ 0,  0}, {0,  1}, { 0,-1}, {1, 1}}}
}};
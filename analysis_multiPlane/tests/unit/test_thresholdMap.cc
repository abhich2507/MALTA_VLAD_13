#include "DigitalProcessing_multiPlane.hh"
#include "Utils.hh"
#include "ConfigAnalysis.hh"

#include <cassert>
#include <iostream>
#include <cmath>

// Already broken lol
void test_mapSize()
{
    AnaFlags cfg{};

    cfg.xPix = 16;
    cfg.yPix = 8;

    cfg.mirrorRepetition = 32;

    cfg.meanSmearing = 0.0;
    cfg.colSmearing = 0.0;

    auto thr = generateThrMap(200.0, 1, "test", cfg, 1234);

    assert (thr.size() == 16 * 8);

    std::cout << "[PASS] test_mapSize\n";
}

int main()
{
    std::cout << "Running threshold map tests...\n";

    test_mapSize();

    std::cout << "\nAll tests passed.\n";

    return 0;
}
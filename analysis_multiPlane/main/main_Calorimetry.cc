#include "Calorimetry_multiPlane.hh"
#include <string>
#include <iostream> 


int main(int argc, char* argv[])
{
    if (argc < 3) {
        std::cerr << "Usage: run_digital <threshold> <runNumber> <saveName> \n";
        return 1;
    }
    double threshold  = std::stod(argv[1]);
    int    runNumber  = std::stoi(argv[2]);
    std::string save  = argv[3];

    Calorimetry_multiPlane(threshold, runNumber, save);
    return 0;
}
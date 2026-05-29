#include "DigitalProcessing_multiPlane.hh"
#include <string>
#include <iostream> 

int main(int argc, char* argv[])
{
    if (argc < 4) {
        std::cerr << "Usage: run_digital <threshold> <runNumber> <saveName> <proteusFlag>\n";
        return 1;
    }
    double threshold  = std::stod(argv[1]);
    int    runNumber  = std::stoi(argv[2]);
    std::string save  = argv[3];
    bool   proteus    = (argc > 4) && std::string(argv[4]) == "1";

    return DigitalProcessing_multiPlane(threshold, runNumber, save, proteus);
}
#pragma once
#include <map>
#include <string>
#include <vector>
#include "ConfigAnalysis.hh"
#include "Tracking_multiPlane.hh"

AnaFlags GetDigitalConfig();
void set_style();
std::string getVarFromConfig();
std::vector<Module> LoadModules(const std::string& filename);
std::map<int, Offset> LoadGeometry(const std::string& geoPath, const DetectorConfig& cfg);
DetectorConfig LoadConfig(const std::string& configPath);
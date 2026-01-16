#ifndef SENSITIVEDETECTOR_H
#define SENSITIVEDETECTOR_H

#include "G4VSensitiveDetector.hh"
#include <map>
#include <utility>
#include <array>
#include <string>
#include <fstream>
#include <cstdint>
#include "G4ThreeVector.hh"
#include "G4Types.hh"

class SimFlags;

class SensitiveDetector: public G4VSensitiveDetector
{
public:
    // G4String = Detector Name
    SensitiveDetector(const G4String& name, const SimFlags* flags);
    ~SensitiveDetector() override;
    //Getter
    const std::map<std::pair<int, int>, int>& getChannelHitMap() const { return m_channelHitMap; }
    G4double getEfficiencyCorrectionXY(const G4ThreeVector& inPixPosition);
    std::pair<std::array<double, 4>, uint8_t>  getEfficiencyAnalytical(const G4ThreeVector& inPixPosition) const;
    //G4double getTimingOffset(G4double amplitude) const;

private:
    const SimFlags* m_flag{};
    G4double m_totalEnergyDeposited{};
    std::map<std::pair<int, int>, G4double> m_pixelLastHitTime;
    std::map<std::pair<int, int>, int> m_pixelHitCount;
    std::map<std::pair<int, int>, int> m_pixelHitMap;
    //std::vector<std::vector<int>> channelHitMap;
    std::map<std::pair<int, int>, int> m_channelHitMap;
    std::ofstream m_hitDataFile{};
    std::map<G4int, G4double> m_trackLengths;
    std::string m_outputPath{};

    // G4HCofThisEvent - generate hit collections for analysis and reconstruction within gent4 or add electronic noise?
    void Initialize(G4HCofThisEvent *) override;
    void EndOfEvent(G4HCofThisEvent *) override;
    // Main function that handles whatever happens during the time when the particle is inside this detector. also gives acces to the volumes
    G4bool ProcessHits(G4Step *, G4TouchableHistory *) override;
};

#endif
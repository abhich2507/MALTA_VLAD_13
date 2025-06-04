#ifndef PHMATTSENSITIVEDETECTOR_HH
#define PHMATTSENSITIVEDETECTOR_HH

#include "G4VSensitiveDetector.hh"
#include "G4RunManager.hh"
#include "G4AnalysisManager.hh"
#include "G4SystemOfUnits.hh"
#include "G4UnitsTable.hh"
#include "G4OpticalPhoton.hh"
#include "PhMattDetectorConstruction.hh"
#include "PhMattEventAction.hh"
#include "G4Poisson.hh"
// Thread Safety
#include <mutex>
#include "CLHEP/Random/RandFlat.h"
class PhMattSensitiveDetector: public G4VSensitiveDetector
{
public:
    // G4String = Detector Name
    PhMattSensitiveDetector(G4String);
    ~PhMattSensitiveDetector();
    //Getter
    const std::map<std::pair<int, int>, int>& GetChannelHitMap() const { return channelHitMap; }
    

private:
    G4double fTotalEnergyDeposited;
    std::map<std::pair<int, int>, G4double> pixelLastHitTime;
    std::map<std::pair<int, int>, int> pixelHitCount;
    std::map<std::pair<int, int>, int> fPixelHitMap;
    //std::vector<std::vector<int>> channelHitMap;
    std::map<std::pair<int, int>, int> channelHitMap;
    std::ofstream hitDataFile;
    std::map<G4int, G4double> fTrackLengths;


    // G4HCofThisEvent - generate hit collections for analysis and reconstruction within gent4 or add electronic noise?
    virtual void Initialize(G4HCofThisEvent *) override;
    virtual void EndOfEvent(G4HCofThisEvent *) override;
    // Main function that handles whatever happens during the time when the particle is inside this detector. also gives acces to the volumes
    virtual G4bool ProcessHits(G4Step *, G4TouchableHistory *);
};



#endif
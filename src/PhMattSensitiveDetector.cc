#include "PhMattSensitiveDetector.hh"

// Implement the desired number of channels
const G4int channelNum = 64;
PhMattSensitiveDetector::PhMattSensitiveDetector(G4String name): G4VSensitiveDetector(name)
{
    fTotalEnergyDeposited = 0.;
}

PhMattSensitiveDetector::~PhMattSensitiveDetector()
{
    if (hitDataFile.is_open()) {
        hitDataFile.close();
    }
}

void PhMattSensitiveDetector::Initialize(G4HCofThisEvent *)
{
    fTotalEnergyDeposited = 0.;
}

void PhMattSensitiveDetector::EndOfEvent(G4HCofThisEvent *)
{

}



G4bool PhMattSensitiveDetector::ProcessHits(G4Step *aStep, G4TouchableHistory *)
{
    G4int eventID = G4RunManager::GetRunManager()->GetCurrentEvent()->GetEventID();

    G4AnalysisManager *analysisManager = G4AnalysisManager::Instance();
    // Implementation of all analysis info to be stored in nTuple
    // PreStep point includes all info of the first interaction within one step. Post step includes the last interaction of that particle.
    G4StepPoint *preStepPoint = aStep->GetPreStepPoint();
    G4double energy = aStep->GetTotalEnergyDeposit();
    G4double fglobalTime = preStepPoint->GetGlobalTime();
    G4ThreeVector posPixel = preStepPoint->GetPosition();

    analysisManager->FillNtupleIColumn(0, 0, eventID);
    analysisManager->FillNtupleDColumn(0, 1, posPixel[0]);
    analysisManager->FillNtupleDColumn(0, 2, posPixel[1]);
    analysisManager->FillNtupleDColumn(0, 3, posPixel[2]);
    analysisManager->FillNtupleIColumn(0, 4, fglobalTime);
    analysisManager->FillNtupleDColumn(0, 5, energy);
    analysisManager->AddNtupleRow(0); 

    // Get out the secondary particle step length
    if (aStep->GetTrack()->GetParentID() != 0)
    {
        
        fTrackLengths[aStep->GetTrack()->GetTrackID()] += aStep->GetStepLength();
        analysisManager->FillNtupleIColumn(2, 0, eventID);
        analysisManager->FillNtupleDColumn(2, 1,  fTrackLengths[aStep->GetTrack()->GetTrackID()] * 1000);
        analysisManager->AddNtupleRow(2); 
    }

    return true;
}
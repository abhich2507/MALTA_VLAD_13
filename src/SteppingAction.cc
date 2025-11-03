#include "SteppingAction.hh"

SteppingAction::SteppingAction(SimFlags* flags): fFlag(flags)
{

}
SteppingAction::~SteppingAction() 
{
    
}

void SteppingAction::UserSteppingAction(const G4Step* aStep)
{
    // Look only at protons
    G4Track* aTrack = aStep->GetTrack();
    if(aTrack->GetDefinition() == G4Proton::Definition()) 
    {
        G4int eventID = G4RunManager::GetRunManager()->GetCurrentEvent()->GetEventID();
        G4AnalysisManager *analysisManager = G4AnalysisManager::Instance();
        
        // Get the pre and post step
        G4StepPoint* preStep = aStep->GetPreStepPoint();
        G4StepPoint* postStep = aStep->GetPostStepPoint();
        // Skip steps where one of the volumes is null (e.g., world boundary)
        G4VPhysicalVolume* prePhysVol = preStep->GetPhysicalVolume();
        G4VPhysicalVolume* postPhysVol = postStep->GetPhysicalVolume();
        if (!prePhysVol || !postPhysVol) return;
        if (aTrack->GetTrackStatus() != fAlive) return;
        // Get the volumes that the particle is transitioning between
        G4String preVol = preStep->GetTouchableHandle()->GetVolume()->GetName();
        G4String postVol = postStep->GetTouchableHandle()->GetVolume()->GetName();
        // Check if the volumes for pre or post are in the PCB stack set
        bool preInPCB  = pcbVolumeNames.count(preVol);
        bool postInPCB = pcbVolumeNames.count(postVol);

        auto dirIt = entryDirections.find(aTrack->GetTrackID());
        auto posIt = entryPositions.find(aTrack->GetTrackID());
        // Proton that enters the PCB
        if (!preInPCB && postInPCB) 
        {
            entryDirections[aTrack->GetTrackID()] = preStep->GetMomentumDirection();
            entryPositions[aTrack->GetTrackID()] = preStep->GetPosition();
        }
        // Proton that leaves the PCB
        if (preInPCB && !postInPCB && dirIt != entryDirections.end() && posIt != entryPositions.end()) 
        {
            G4ThreeVector inDir = dirIt->second;
            G4ThreeVector outDir = postStep->GetMomentumDirection();
            G4ThreeVector momVal = postStep->GetMomentum();

            G4double angle = inDir.angle(outDir);  // radians
            G4double angleDeg = angle * (180.0 / CLHEP::pi);

            // Save the scattering angle to nTuple
            G4ThreeVector entryPos = posIt->second;

            if(fFlag->verboseSA) G4cout << "Track " << aTrack->GetTrackID() << entryPos.x() << entryPos.y() << entryPos.z() << " scattering angle (deg): " << angleDeg << G4endl;
            
            analysisManager->FillNtupleIColumn(0, 0, eventID);
            analysisManager->FillNtupleDColumn(0, 1, entryPos.x());
            analysisManager->FillNtupleDColumn(0, 2, entryPos.y());
            analysisManager->FillNtupleDColumn(0, 3, entryPos.z());
            analysisManager->FillNtupleDColumn(0, 4, angleDeg);
            analysisManager->FillNtupleDColumn(0, 5, momVal.mag());
            analysisManager->AddNtupleRow(0); 

            analysisManager->FillH1(0, angleDeg);
            analysisManager->FillH1(1, momVal.mag());

            entryDirections.erase(dirIt);
            entryPositions.erase(posIt);
        }
    }
}
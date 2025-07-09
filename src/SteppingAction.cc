#include "SteppingAction.hh"



SteppingAction::SteppingAction() {}
SteppingAction::~SteppingAction() {}

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
        if (!preInPCB && postInPCB) {
            entryDirections[aTrack->GetTrackID()] = preStep->GetMomentumDirection();
            entryPositions[aTrack->GetTrackID()] = preStep->GetPosition();
            //G4cout << "I am here!";
        }
        // Proton that leaves the PCB
        if (preInPCB && !postInPCB && dirIt != entryDirections.end() && posIt != entryPositions.end()) 
        {
            //G4cout << "Now I am here!";
            G4ThreeVector inDir = dirIt->second;
            G4ThreeVector outDir = postStep->GetMomentumDirection();
            G4ThreeVector momVal = postStep->GetMomentum();

            G4double angle = inDir.angle(outDir);  // radians
            G4double angleDeg = angle * (180.0 / CLHEP::pi);


            // Save the scattering angle to nTuple
            G4ThreeVector entryPos = posIt->second;

            //G4cout << "Track " << aTrack->GetTrackID() << entryPos.x() << entryPos.y() << entryPos.z() << " scattering angle (deg): " << angleDeg << G4endl;

            
            analysisManager->FillNtupleIColumn(1, 0, eventID);
            analysisManager->FillNtupleDColumn(1, 1, entryPos.x());
            analysisManager->FillNtupleDColumn(1, 2, entryPos.y());
            analysisManager->FillNtupleDColumn(1, 3, entryPos.z());
            analysisManager->FillNtupleDColumn(1, 4, angleDeg);
            analysisManager->FillNtupleDColumn(1, 5, momVal.mag());
            analysisManager->AddNtupleRow(1); 

            analysisManager->FillH1(0, angleDeg);
            analysisManager->FillH1(1, momVal.mag());


            entryDirections.erase(dirIt);
            entryPositions.erase(posIt);
        }

    }
    
}
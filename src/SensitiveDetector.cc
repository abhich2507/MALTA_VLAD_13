#include "SensitiveDetector.hh"
#include <cmath>
#include "CorrectionData2D.hh" // Access EffMap2D

// Implement the desired number of channels
const G4int channelNum = 64;
SensitiveDetector::SensitiveDetector(G4String name): G4VSensitiveDetector(name)
{
    fTotalEnergyDeposited = 0.;
}

SensitiveDetector::~SensitiveDetector()
{
    if (hitDataFile.is_open()) {
        hitDataFile.close();
    }
}

void SensitiveDetector::Initialize(G4HCofThisEvent *)
{
    fTotalEnergyDeposited = 0.;
}

void SensitiveDetector::EndOfEvent(G4HCofThisEvent *)
{

}



G4bool SensitiveDetector::ProcessHits(G4Step *aStep, G4TouchableHistory *)
{
    G4int eventID = G4RunManager::GetRunManager()->GetCurrentEvent()->GetEventID();

    G4AnalysisManager *analysisManager = G4AnalysisManager::Instance();
    // Implementation of all analysis info to be stored in nTuple
    // PreStep point includes all info of the first interaction within one step. Post step includes the last interaction of that particle.
    G4StepPoint *preStepPoint = aStep->GetPreStepPoint();
    G4double energy = aStep->GetTotalEnergyDeposit();
    G4double fglobalTime = preStepPoint->GetGlobalTime();
    G4ThreeVector posPixel = preStepPoint->GetPosition();

    //G4cout << posPixel[0] << "," << posPixel[1] << G4endl;
    // get modulus for InPixel location.
    G4ThreeVector InPixPos = G4ThreeVector(std::fmod(posPixel[0],36.4*um), std::fmod(posPixel[1],36.4*um), posPixel[2]); // result in mm
    //G4double MPV_binsize = 36.4/16; // unit um
    double efficiency = GetEfficiencyCorrectionXY(InPixPos);
    // G4cout << "InPixPos: " << InPixPos[0]/um << ", " << InPixPos[1]/um << " --> Eff: " << efficiency << G4endl;
    G4double edep_corr = efficiency * energy;

    analysisManager->FillNtupleIColumn(0, 0, eventID);
    analysisManager->FillNtupleDColumn(0, 1, posPixel[0]);
    analysisManager->FillNtupleDColumn(0, 2, posPixel[1]);
    analysisManager->FillNtupleDColumn(0, 3, posPixel[2]);
    analysisManager->FillNtupleIColumn(0, 4, fglobalTime);
    analysisManager->FillNtupleDColumn(0, 5, energy);
    analysisManager->FillNtupleDColumn(0, 6, edep_corr);
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

// obtain a scalar efficiency based on the XY positions within a pixel.
// binsize in unit um
G4double SensitiveDetector::GetEfficiencyCorrectionXY(const G4ThreeVector& InPixPosition) {

G4double eff;
G4double c00, c10, c01, c11;

G4double xx = InPixPosition.x() / um; // in unit um with origin at bottom left corner (from 0 to 36.4)
G4double yy = InPixPosition.y() / um;

const int dx = floor(xx/spacingX);
const int dy = floor(yy/spacingY);

size_t dimX = sizeof(EffMap2D) / sizeof(EffMap2D[0]);       // nBinsX
size_t dimY = sizeof(EffMap2D[0]) / sizeof(EffMap2D[0][0]); // nBinsY

if (dx < 0 || dy < 0 || dx > dimX || dy > dimY) { 
    G4cout << " Extend range of input." << G4endl;
    return 0;
    }

c00 = EffMap2D[dx][dy];
c10 = EffMap2D[dx+1][dy];
c01 = EffMap2D[dx][dy+1];
c11 = EffMap2D[dx+1][dy+1];

if ((c00<0.) or (c10<0.) or (c01<0.) or (c11<0.)) {
        eff = 0.; // if any close point is negative --> energy not detected
    }
else {
    G4double x1 = dx*spacingX;
    G4double x2 = (dx+1)*spacingX;
    G4double y1 = dy*spacingY;
    G4double y2 = (dy+1)*spacingY;

    eff=( (y2-yy)*(x2-xx)*c00 + 
            (y2-yy)*(xx-x1)*c10 + 
            (yy-y1)*(x2-xx)*c01 + 
            (yy-y1)*(xx-x1)*c11)/(spacingX*spacingY); // for same binsize: division by *binsize^2
}

return eff;
}
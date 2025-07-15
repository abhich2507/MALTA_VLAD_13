#include "SensitiveDetector.hh"
#include <cmath>
#include "CorrectionData2D.hh" // Access EffMap2D

// Implement the desired number of channels
const G4int channelNum = 64;
SensitiveDetector::SensitiveDetector(G4String name, SimFlags* flags): G4VSensitiveDetector(name), fFlag(flags)
{
    fTotalEnergyDeposited = 0.;
    if(fFlag->runMode == "local") 
    {
        fOutputPath = fFlag->outputPathLocal;
    }
    else
    {
        fOutputPath = fFlag->outputPathNAF;
    }
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
    std::array<double, 4> effAn = GetEfficiencyAnalytical(InPixPos);
    G4cout << "InPixPos: " << InPixPos[0]/um << ", " << InPixPos[1]/um << " --> Eff: " << efficiency << " of 4 pixels: " << effAn[0] << " " << effAn[1] << " " << effAn[2] << " " << effAn[3] << " " << G4endl;

    // fill the 4 efficiencies into a tree. Apply a minimal threshold here already?
    // take care if hit is at boundary of sensor (minimal or maximal pix number.)
    // associate timing based on amplitude (from time walk)

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


// --- Helper function definitions ---
//constexpr double SQRT2 = std::sqrt(2.0);

// Error function-based 1D step
// x is coordinate in range [0, pitch]. Hence, pixel is centered around pitch/2.
// error-fct that parameterize edge of pixel are at x = 0 and x = pitch
// sigma is gaussian standard deviation
G4double smoothStep(G4double x, G4double pitch, G4double sigma) {
    return 0.5 * (std::erf((x) / (sigma * std::sqrt(2.0))) - std::erf((x - pitch) / (sigma * std::sqrt(2.0))));
}


// Analytical model of smeared rectangular box (error-functions in X and Y )
// obtain a scalar efficiency based on the XY positions within a pixel.
// binsize in unit um
// per definition the some of all 4 efficiencies = 1.0
std::array<double, 4>  SensitiveDetector::GetEfficiencyAnalytical(const G4ThreeVector& InPixPosition) {

    G4double pitch = 36.4; // in um
    G4double sigmaX = 4.3; // in um;
    G4double sigmaY = 4.3; // in um;

    // contribution to 4 neighboring pixels
    // 00 is bottom left    (low X,     low Y)
    // 01 is bottom right   (low X,     large Y)
    // 10 is top left       (large X,   low Y)
    // 11 is top right      (large X,   large Y)

    G4double xx = InPixPosition.x() / um; // in unit um with origin at bottom left corner (from 0 to 36.4)
    G4double yy = InPixPosition.y() / um;

    // center InPixPos around center
    G4double eff_X0, eff_Y0, eff_X1, eff_Y1;

    if(xx<pitch/2.) { // in "left" half of pixel
        eff_X0 = smoothStep(xx + pitch, pitch, sigmaX); // nearest neighbor is left
        eff_X1 = smoothStep(xx, pitch, sigmaX); // seed pixel is right
    }
    else {
        eff_X0 = smoothStep(xx, pitch, sigmaX); // seed pixel is left
        eff_X1 = smoothStep(xx - pitch, pitch, sigmaX); // nearest neighbor is right
    }

    if(yy<pitch/2.) { // in "bottom" half of pixel
        eff_Y0 = smoothStep(yy + pitch, pitch, sigmaY); // nearest neighbor is below
        eff_Y1 = smoothStep(yy, pitch, sigmaY); // seed pixel is top
    }
    else {
        eff_Y0 = smoothStep(yy, pitch, sigmaY); // seed pixel is below
        eff_Y1 = smoothStep(yy - pitch, pitch, sigmaY); // nearest neighbor is above
    }
    
    G4double eff00, eff01, eff10, eff11; 
    
    eff00 = eff_X0 * eff_Y0; // bottom left
    eff01 = eff_X0 * eff_Y1; // bottom right
    eff10 = eff_X1 * eff_Y0; // top left
    eff11 = eff_X1 * eff_Y1; // top right

    return {eff00, eff01, eff10, eff11}; // ordering not certain yet.
}
#include "SensitiveDetector.h"
#include "G4RunManager.hh"
#include "G4AnalysisManager.hh"
#include "G4SystemOfUnits.hh"
#include "G4UnitsTable.hh"
#include "G4OpticalPhoton.hh"
#include "DetectorConstruction.h"
#include "EventAction.h"
#include "G4Poisson.hh"
#include "Config.h"
// Thread Safety
#include <mutex>
#include "CLHEP/Random/RandFlat.h"
#include <cmath>

SensitiveDetector::SensitiveDetector(const G4String& name, const SimFlags* flags): G4VSensitiveDetector(name), m_flag(flags)
{
    m_totalEnergyDeposited = 0.;
    if(m_flag->runMode == "local") 
    {
        m_outputPath = m_flag->outputPathLocal;
    }
    else
    {
        m_outputPath = m_flag->outputPathNAF;
    }
}

SensitiveDetector::~SensitiveDetector()
{
    if (m_hitDataFile.is_open()) {
        m_hitDataFile.close();
    }
}

void SensitiveDetector::Initialize(G4HCofThisEvent *)
{
    m_totalEnergyDeposited = 0.;
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
    G4float energy = aStep->GetTotalEnergyDeposit();
    G4int trackID = aStep->GetTrack()->GetTrackID();
    G4float fglobalTime = preStepPoint->GetGlobalTime();
    G4ThreeVector posVertex = aStep->GetTrack()->GetVertexPosition();
    // Position is currently defined as the preStepPoint position. 
    G4ThreeVector posPixel = preStepPoint->GetPosition();
    const float pixelSize = m_flag->pixelSize *mm;
    const float detX = m_flag->detectorSizeX *cm;
    const float detY = m_flag->detectorSizeY *cm;
    const float detXOffset = m_flag->detectorXOffset *cm;
    const float detYOffset = m_flag->detectorYOffset *cm;

    const std::array<std::array<std::array<int, 2>, 4>, 4> deltaTable = 
    {{
        // flag = 0b00
        {{{-1, -1}, {0, -1}, {-1, 0}, {0, 0}}},
        // flag = 0b01
        {{{ 0, -1}, {1, -1}, { 0, 0}, {1, 0}}},
        // flag = 0b10
        {{{-1,  0}, {0,  0}, {-1, 1}, {0, 1}}},
        // flag = 0b11
        {{{ 0,  0}, {0,  1}, { 1, 0}, {1, 1}}}  // 0, -1 -> 1, 0 
    }};

    G4String planeName = preStepPoint->GetTouchableHandle()->GetVolume()->GetName();
    int planeID;
    if(m_flag->preDefinedGeometryFlag == "MALTA")
    {
        if(planeName == "physSensor") 
        {
            planeID = 0;
        }
        else if(planeName == "physPlane1") 
        {
            planeID = 1;
        }
        else if(planeName == "physPlane2") 
        {
            planeID = 2;
        }
        else if(planeName == "physPlane3") 
        {
            planeID = 3;
        }
        else if(planeName == "physPlane4") 
        {
            planeID = 4;
        }
        else if(planeName == "physPlane5") 
        {
            planeID = 5;
        }
        else if(planeName == "physPlane6") 
        {
            planeID = 6;
        }
        else
        {
            G4cout << "Error: Unknown plane name " << planeName << G4endl;
            planeID = -1;
        }
    }
    // get modulus for InPixel location.
    G4ThreeVector InPixPos = G4ThreeVector(std::fmod(posPixel[0],pixelSize), std::fmod(posPixel[1],pixelSize), posPixel[2]); // result in mm
    //double efficiency = GetEfficiencyCorrectionXY(InPixPos);
    //G4double edep_corr = efficiency * energy;    
    auto [effAn, quadrantFlag] = getEfficiencyAnalytical(InPixPos);

    int pixX = static_cast<int>((posPixel.x() - detXOffset + detX / 2) / pixelSize);
    int pixY = static_cast<int>((posPixel.y() - detYOffset + detY / 2) / pixelSize);
    // Handle edge cases: The filtering of out of bound pixels is graciously done in the DigitalProcessing via threshold mapping, 
    // This block however, also correctly endowes charge sharing to the correct side of the pixel edge.
    
    if(pixX == 0 && ((quadrantFlag & 0x1) == 0 ))
    {
        effAn[1]+=effAn[0];
        effAn[0] = 0;
        effAn[3]+=effAn[2];
        effAn[2] = 0;

    }

    if(pixX == 511 && (quadrantFlag & 0x1) )
    {
        effAn[0]+=effAn[1];
        effAn[1] = 0;
        effAn[2]+=effAn[3];
        effAn[3] = 0;
    }

    if(pixY == 0 && ( (quadrantFlag & (1 << 1)) == 0) )
    {
        effAn[2]+=effAn[0];
        effAn[0] = 0;
        effAn[3]+=effAn[1];
        effAn[1] = 0;

    }

    if(pixY == 511 && (quadrantFlag & (1 << 1)) )
    {
        effAn[0]+=effAn[2];
        effAn[2] = 0;
        effAn[1]+=effAn[3];
        effAn[3] = 0;
    }
    
    if (pixX < 0 || pixX >= 512 || pixY < 0 || pixY >= 512)
        return false;


    std::array<std::array<int, 2>, 4> pixelCluster;
    for (std::array<std::array<int, 2>, 4>::size_type i = 0; i < 4; ++i) 
    {
        pixelCluster[i] = {pixX, pixY};
    }
    const auto& deltas = deltaTable[quadrantFlag];
    
    for(std::array<std::array<int, 2>, 4>::size_type i = 0; i<4; i++)
    {
        pixelCluster[i][0] +=deltas[i][0];
        pixelCluster[i][1] +=deltas[i][1];
    }

    if (m_flag->verboseSD)
    {
        G4cout << "InPixPos: " << InPixPos[0]/um << ", " << InPixPos[1]/um << " --> En: " << energy << " of 4 pixels: " << effAn[0] << " " << effAn[1] << " " << effAn[2] << " " << effAn[3] << " " << G4endl;
        G4cout << "Pix coordinate: " << pixX << ";" << pixY << G4endl;
        G4cout << "Initial Pixel position: " << posPixel.x() << " ; "<< posPixel.y() << G4endl;
        G4cout << "Cluster pixel positions: "<< G4endl << pixelCluster[0][0] << "," << pixelCluster[0][1] << " ; " << G4endl 
                                            << pixelCluster[1][0] << "," << pixelCluster[1][1] << " ; " << G4endl 
                                            << pixelCluster[2][0] << "," << pixelCluster[2][1] << " ; " << G4endl 
                                            << pixelCluster[3][0] << "," << pixelCluster[3][1] << "  " << G4endl ;
    }

    const float epsilon = 3.66; // electron-hole pair creaton energy = (3.66 +- 0.03) eV
    // Call event action eDep function for adding up energy and timing derivation
    auto eventAction = static_cast<EventAction*>(G4EventManager::GetEventManager()->GetUserEventAction());
    // Hardcoded disable all other planes
    std::array<float,4> effAnCopy = effAn; // forces evaluation
    for(std::array<std::array<int, 2>, 4>::size_type i = 0; i<4; i++)
    {
        eventAction->addEdep(eventID, effAnCopy[i] * energy *1000000/epsilon, fglobalTime *ns, planeID, pixelCluster[i][0], pixelCluster[i][1]);
    }
    // fill the 4 efficiencies and timing into a tree. 
    // Apply a minimal threshold here already of 50 e-? edep in MeV --> if (edep*10^6/epsilon > 50) // threshold in e- // edep in MeV
    // take care if hit is at boundary of sensor (minimal or maximal pix number.)
    // associate timing based on amplitude (from time walk):

    // This line ensures that the compiler evaluates all effAn at the same time. If you dont do this float point fluctuations might appear
    // This is most probably due to multithreading even though I cant prove it.
    // Logic moved to EventAction.cc
    /* 
    int iHit = 0;
    for(std::array<std::array<int, 2>, 4>::size_type i = 0; i<4; i++)
    {
        analysisManager->FillNtupleIColumn(1, 0, eventID);
        analysisManager->FillNtupleIColumn(1, 1, planeID);
        analysisManager->FillNtupleIColumn(1, 2, iHit);
        analysisManager->FillNtupleIColumn(1, 3, pixelCluster[i][0]);
        analysisManager->FillNtupleIColumn(1, 4, pixelCluster[i][1]);
        // This branch will be modified. Before it as encoding the timewalk of each hit. Now it stores the hit time of a particle.
        analysisManager->FillNtupleDColumn(1, 5, fglobalTime *ns);
        analysisManager->FillNtupleDColumn(1, 6, effAnCopy[i] * energy *1000000/epsilon);
        analysisManager->AddNtupleRow(1); 
        iHit++;
    }
    */
    return true;
}


// Error function-based 1D step
// x is coordinate in range [0, pitch]. Hence, pixel is centered around pitch/2.
// error-fct that parameterize edge of pixel are at x = 0 and x = pitch
// sigma is gaussian standard deviation
G4float smoothStep(G4float x, G4float pitch, G4float sigma) {
    return 0.5 * (std::erf((x) / sigma) - std::erf((x - pitch) / sigma));
}

// Analytical model of smeared rectangular box (error-functions in X and Y )
// obtain a scalar efficiency based on the XY positions within a pixel.
// binsize in unit um
// per definition the some of all 4 efficiencies = 1.0
std::pair<std::array<float, 4>, uint8_t>  SensitiveDetector::getEfficiencyAnalytical(const G4ThreeVector& InPixPosition) const {

    const float pitch = m_flag->pixelSize *1000; // convert from mm to um (default 36.4 um)
    const float sigmaX = m_flag->CCModelSigmaX; // in um (default 4.3 um)
    const float sigmaY = m_flag->CCModelSigmaY; // in um (default 4.3 um)

    // contribution to 4 neighboring pixels
    // 00 is bottom left    (low X,     low Y)
    // 01 is bottom right   (low X,     large Y)
    // 10 is top left       (large X,   low Y)
    // 11 is top right      (large X,   large Y)

    G4float xx = InPixPosition.x() / um; // in unit um with origin at bottom left corner (from 0 to 36.4)
    G4float yy = InPixPosition.y() / um;

    // center InPixPos around center
    G4float eff_X0, eff_Y0, eff_X1, eff_Y1;
    // method to output the quadrant position of the hit
    uint8_t quadrantFlag = 0;

    if(xx<pitch/2.) { // in "left" half of pixel
        eff_X0 = smoothStep(xx + pitch, pitch, sigmaX); // nearest neighbor is left
        eff_X1 = smoothStep(xx, pitch, sigmaX); // seed pixel is right
    }
    else {
        quadrantFlag |= (1 << 0); // set bit 0
        eff_X0 = smoothStep(xx, pitch, sigmaX); // seed pixel is left
        eff_X1 = smoothStep(xx - pitch, pitch, sigmaX); // nearest neighbor is right
    }

    if(yy<pitch/2.) { // in "bottom" half of pixel
        eff_Y0 = smoothStep(yy + pitch, pitch, sigmaY); // nearest neighbor is below
        eff_Y1 = smoothStep(yy, pitch, sigmaY); // seed pixel is top
    }
    else {
        quadrantFlag |= (1 << 1); // set bit 1
        eff_Y0 = smoothStep(yy, pitch, sigmaY); // seed pixel is below
        eff_Y1 = smoothStep(yy - pitch, pitch, sigmaY); // nearest neighbor is above
    }
    
    G4float eff00, eff01, eff10, eff11; 
    
    eff00 = eff_X0 * eff_Y0; // bottom left
    eff01 = eff_X0 * eff_Y1; // bottom right
    eff10 = eff_X1 * eff_Y0; // top left
    eff11 = eff_X1 * eff_Y1; // top right

    return {{eff00, eff01, eff10, eff11}, quadrantFlag} ; // ordering not certain yet. 
}
#include "SensitiveDetector.hh"

<<<<<<< HEAD
<<<<<<< HEAD
=======
// TODO: Create debug flags for different points of simulation.
// WARNING A lot of work ahead 

>>>>>>> 66f7594 (DEBUG)
=======
>>>>>>> 8a2e4b8 (Cleaned up the simulation files. Removed the Truth En tree as it was not used further in the analysis chain. Additionally, general clean up in terms of branch renaming. It most probably will impact the in_pixel_plots script. However the default analysis chain has already been modified to account for these changes.)
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
    G4ThreeVector posVertex = aStep->GetTrack()->GetVertexPosition();
    // Position is currently defined as the preStepPoint position. 
    G4ThreeVector posPixel = preStepPoint->GetPosition();
    double pixelSize = fFlag->pixelSize *mm;
    double detX = fFlag->detectorSizeX *cm;
    double detY = fFlag->detectorSizeY *cm;
    double detXOffset = fFlag->detectorXOffset *cm;
    double detYOffset = fFlag->detectorYOffset *cm;

    G4String planeName = preStepPoint->GetTouchableHandle()->GetVolume()->GetName();
    int planeID;
    if(fFlag->preDefinedGeometryFlag == "MALTA")
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
<<<<<<< HEAD
<<<<<<< HEAD
=======

<<<<<<< HEAD
    //G4cout << posPixel[0] << "," << posPixel[1] << G4endl;
>>>>>>> 8149767 (Added digitization + tracking + clustering + analysis in an automatic fashion for each run)
    // get modulus for InPixel location.
=======
>>>>>>> 66f7594 (DEBUG)
=======
    // get modulus for InPixel location.
>>>>>>> 8a2e4b8 (Cleaned up the simulation files. Removed the Truth En tree as it was not used further in the analysis chain. Additionally, general clean up in terms of branch renaming. It most probably will impact the in_pixel_plots script. However the default analysis chain has already been modified to account for these changes.)
    G4ThreeVector InPixPos = G4ThreeVector(std::fmod(posPixel[0],pixelSize), std::fmod(posPixel[1],pixelSize), posPixel[2]); // result in mm
    double efficiency = GetEfficiencyCorrectionXY(InPixPos);
    G4double edep_corr = efficiency * energy;    
    auto [effAn, quadrantFlag] = GetEfficiencyAnalytical(InPixPos);

    int pixX = static_cast<int>((posPixel.x() - detXOffset + detX / 2) / pixelSize);
    int pixY = static_cast<int>((posPixel.y() - detYOffset + detY / 2) / pixelSize);
    std::array<std::array<int, 2>, 4> pixelCluster;
    for (int i = 0; i < 4; ++i) 
    {
    pixelCluster[i] = {pixX, pixY};
    }
    const auto& deltas = deltaTable[quadrantFlag];
    for(int i = 0; i<=4; i++)
    {
        pixelCluster[i][0] +=deltas[i][0] ;
        pixelCluster[i][1] +=deltas[i][1] ;
    }

    if (fFlag->verboseSD)
    {
        G4cout << "InPixPos: " << InPixPos[0]/um << ", " << InPixPos[1]/um << " --> Eff: " << efficiency << " of 4 pixels: " << effAn[0] << " " << effAn[1] << " " << effAn[2] << " " << effAn[3] << " " << G4endl;
        G4cout << "Pix coordinate: " << pixX << ";" << pixY << G4endl;
        G4cout << "Initial Pixel position: " << posPixel.x() << " ; "<< posPixel.y() << G4endl;
        G4cout << "Cluster pixel positions: "<< G4endl << pixelCluster[0][0] << "," << pixelCluster[0][1] << " ; " << G4endl 
                                            << pixelCluster[1][0] << "," << pixelCluster[1][1] << " ; " << G4endl 
                                            << pixelCluster[2][0] << "," << pixelCluster[2][1] << " ; " << G4endl 
                                            << pixelCluster[3][0] << "," << pixelCluster[3][1] << "  " << G4endl ;
    }

    double epsilon = 3.66; // electron-hole pair creaton energy = (3.66 +- 0.03) eV
    // fill the 4 efficiencies and timing into a tree. 
    // Apply a minimal threshold here already of 50 e-? edep in MeV --> if (edep*10^6/epsilon > 50) // threshold in e- // edep in MeV
    // take care if hit is at boundary of sensor (minimal or maximal pix number.)
    // associate timing based on amplitude (from time walk):

    // This line ensures that the compiler evaluates all effAn at the same time. If you dont do this float point fluctuations might appear
    // This is most probably due to multithreading even though I cant prove it.
    std::array<double,4> effAnCopy = effAn; // forces evaluation
    int iHit = 0;
    // Store only the largest deposited energy in a cluster and its corresponding timing
    auto it = std::max_element(effAnCopy.begin(), effAnCopy.end());
    size_t maxIndex = std::distance(effAnCopy.begin(), it);
    double leadingEnergy = *it * energy; // leading energy in MeV
    for(int i = 0; i<4; i++)
    {
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 8a2e4b8 (Cleaned up the simulation files. Removed the Truth En tree as it was not used further in the analysis chain. Additionally, general clean up in terms of branch renaming. It most probably will impact the in_pixel_plots script. However the default analysis chain has already been modified to account for these changes.)
        analysisManager->FillNtupleIColumn(1, 0, eventID);
        analysisManager->FillNtupleIColumn(1, 1, planeID);
        analysisManager->FillNtupleIColumn(1, 2, iHit);
        analysisManager->FillNtupleIColumn(1, 3, pixelCluster[i][0]);
        analysisManager->FillNtupleIColumn(1, 4, pixelCluster[i][1]);
<<<<<<< HEAD
        // This branch will be modified. Before it as encoding the timewalk of each hit. Now it stores the hit time of a particle.
        analysisManager->FillNtupleDColumn(1, 5, fglobalTime *ns);
        analysisManager->FillNtupleDColumn(1, 6, effAnCopy[i] * energy *1000000/epsilon);
        analysisManager->AddNtupleRow(1); 
        iHit++;
=======
        //if (effAn[i] * energy *1000000/epsilon > 50) // Set a data supression threshold at 50 deposited el

        analysisManager->FillNtupleIColumn(3, 0, eventID);
        analysisManager->FillNtupleIColumn(3, 1, planeID);
        analysisManager->FillNtupleIColumn(3, 2, iHit);
        analysisManager->FillNtupleIColumn(3, 3, pixelCluster[i][0]);
        analysisManager->FillNtupleIColumn(3, 4, pixelCluster[i][1]);
=======
>>>>>>> 8a2e4b8 (Cleaned up the simulation files. Removed the Truth En tree as it was not used further in the analysis chain. Additionally, general clean up in terms of branch renaming. It most probably will impact the in_pixel_plots script. However the default analysis chain has already been modified to account for these changes.)
        // This branch will be modified. Before it as encoding the timewalk of each hit. Now it stores the hit time of a particle.
        analysisManager->FillNtupleDColumn(1, 5, fglobalTime *ns);
        analysisManager->FillNtupleDColumn(1, 6, effAnCopy[i] * energy *1000000/epsilon);
        analysisManager->AddNtupleRow(1); 
        iHit++;
<<<<<<< HEAD

    }

    /**/
    analysisManager->FillNtupleIColumn(0, 0, eventID);
    analysisManager->FillNtupleIColumn(0, 1, planeID);
    // X, Y, Z position of the hit
    analysisManager->FillNtupleDColumn(0, 2, posPixel[0]);
    analysisManager->FillNtupleDColumn(0, 3, posPixel[1]);
    analysisManager->FillNtupleDColumn(0, 4, posPixel[2]);

    analysisManager->FillNtupleDColumn(0, 5, posVertex[0]);
    analysisManager->FillNtupleDColumn(0, 6, posVertex[1]);
    analysisManager->FillNtupleDColumn(0, 7, posVertex[2]);

    analysisManager->FillNtupleDColumn(0, 8, fglobalTime *ns);
    analysisManager->FillNtupleDColumn(0, 9, energy);
    analysisManager->FillNtupleDColumn(0, 10, edep_corr);
    analysisManager->FillNtupleIColumn(0, 11, iHit);
    analysisManager->FillNtupleDColumn(0, 12, leadingEnergy);
    analysisManager->FillNtupleDColumn(0, 13, leadingTime);
    analysisManager->AddNtupleRow(0); 
    





    // Get out the secondary particle step length
    if (aStep->GetTrack()->GetParentID() != 0)
    {
        fTrackLengths[aStep->GetTrack()->GetTrackID()] += aStep->GetStepLength();
        analysisManager->FillNtupleIColumn(2, 0, eventID);
        analysisManager->FillNtupleDColumn(2, 1,  fTrackLengths[aStep->GetTrack()->GetTrackID()] * 1000);
        analysisManager->AddNtupleRow(2); 
>>>>>>> 8149767 (Added digitization + tracking + clustering + analysis in an automatic fashion for each run)
=======
>>>>>>> 8a2e4b8 (Cleaned up the simulation files. Removed the Truth En tree as it was not used further in the analysis chain. Additionally, general clean up in terms of branch renaming. It most probably will impact the in_pixel_plots script. However the default analysis chain has already been modified to account for these changes.)
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

    if (dx < 0 || dy < 0 || dx > dimX || dy > dimY) 
    { 
        G4cout << " Extend range of input." << G4endl;
        return 0;
    }

    c00 = EffMap2D[dx][dy];
    c10 = EffMap2D[dx+1][dy];
    c01 = EffMap2D[dx][dy+1];
    c11 = EffMap2D[dx+1][dy+1];

    if ((c00<0.) or (c10<0.) or (c01<0.) or (c11<0.)) 
    {
            eff = 0.; // if any close point is negative --> energy not detected
    }
    else 
    {
        G4double x1 = dx*spacingX;
        G4double x2 = (dx+1)*spacingX;
        G4double y1 = dy*spacingY;
        G4double y2 = (dy+1)*spacingY;

        eff=(   (y2-yy)*(x2-xx)*c00 + 
                (y2-yy)*(xx-x1)*c10 + 
                (yy-y1)*(x2-xx)*c01 + 
                (yy-y1)*(xx-x1)*c11)/(spacingX*spacingY); // for same binsize: division by *binsize^2
    }

    return eff;
}

// Error function-based 1D step
// x is coordinate in range [0, pitch]. Hence, pixel is centered around pitch/2.
// error-fct that parameterize edge of pixel are at x = 0 and x = pitch
// sigma is gaussian standard deviation
G4double smoothStep(G4double x, G4double pitch, G4double sigma) {
    return 0.5 * (std::erf((x) / sigma) - std::erf((x - pitch) / sigma));
<<<<<<< HEAD
=======
    //return 0.5 * (std::erf((x) / (sigma * std::sqrt(2.0))) - std::erf((x - pitch) / (sigma * std::sqrt(2.0))));
>>>>>>> 18e3f08 (removed sqrt2 from smoothstep)
}

// Analytical model of smeared rectangular box (error-functions in X and Y )
// obtain a scalar efficiency based on the XY positions within a pixel.
// binsize in unit um
// per definition the some of all 4 efficiencies = 1.0
std::pair<std::array<double, 4>, uint8_t>  SensitiveDetector::GetEfficiencyAnalytical(const G4ThreeVector& InPixPosition) {

    double pitch = fFlag->pixelSize *1000; // convert from mm to um (default 36.4 um)
    double sigmaX = fFlag->CCModelSigmaX; // in um (default 4.3 um)
    double sigmaY = fFlag->CCModelSigmaY; // in um (default 4.3 um)

    // contribution to 4 neighboring pixels
    // 00 is bottom left    (low X,     low Y)
    // 01 is bottom right   (low X,     large Y)
    // 10 is top left       (large X,   low Y)
    // 11 is top right      (large X,   large Y)

    G4double xx = InPixPosition.x() / um; // in unit um with origin at bottom left corner (from 0 to 36.4)
    G4double yy = InPixPosition.y() / um;

    // center InPixPos around center
    G4double eff_X0, eff_Y0, eff_X1, eff_Y1;
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
    
    G4double eff00, eff01, eff10, eff11; 
    
    eff00 = eff_X0 * eff_Y0; // bottom left
    eff01 = eff_X0 * eff_Y1; // bottom right
    eff10 = eff_X1 * eff_Y0; // top left
    eff11 = eff_X1 * eff_Y1; // top right

    return {{eff00, eff01, eff10, eff11}, quadrantFlag} ; // ordering not certain yet.
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 580f2b4 (Tgain and agian. time-walk model extended to all thresholds based on simple assumptions.)
}
=======
}

// Get time-walk from parameterization.
// amplitude in unit electrons
// returns timing in ns
<<<<<<< HEAD
G4double SensitiveDetector::GetTimingOffset(G4double amplitude) {
    // function diverges at 150e-
    if (amplitude < 150.) 
    { // delay only down to amplitudes of 150e-. 
        return 200;
    }
    return 390. /pow(amplitude-149.8, 0.65);

}
>>>>>>> 8149767 (Added digitization + tracking + clustering + analysis in an automatic fashion for each run)
=======
}
>>>>>>> 8a2e4b8 (Cleaned up the simulation files. Removed the Truth En tree as it was not used further in the analysis chain. Additionally, general clean up in terms of branch renaming. It most probably will impact the in_pixel_plots script. However the default analysis chain has already been modified to account for these changes.)
=======
// at 150e- threshold
G4double SensitiveDetector::GetTimingOffset(G4double amplitude) {
    // function diverges at 150e-
    if (amplitude < 160.) 
    { // delay only down to amplitudes of 160e-. 
        return 390. /pow(160-149.8, 0.65);
    }

    return 390. /pow(amplitude-149.8, 0.65);

}

G4double SensitiveDetector::GetTimingOffsetatThreshold(G4double amplitude, G4double threshold) {
    // function diverges at 150e-
    if (amplitude < 160.) 
    { // delay only down to amplitudes of 160e-. 
        return 390. /pow(160-149.8, 0.65);
    }

    return 390. /pow(amplitude/threshold*150.-149.8, 0.65);

}
>>>>>>> 2d19158 (Time-walk model extended to all thresholds based on simple assumptions.)
>>>>>>> 580f2b4 (Tgain and agian. time-walk model extended to all thresholds based on simple assumptions.)

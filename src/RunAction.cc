#include "RunAction.hh"
#include "CustomRun.hh"

RunAction::RunAction(SimFlags* flags) : fFlag(flags)
{
    // Simplified instantiation thanks to GEANT4 implementation
    G4AnalysisManager *analysisManager = G4AnalysisManager::Instance();

<<<<<<< HEAD
<<<<<<< HEAD
=======
    // Deposited Energy Ntuple. MONTE CARLO TRUTH
    analysisManager->CreateNtuple("TruthEnDeposited", "Monte Carlo Truth Energy Deposited");
    // Create Integer Event # column
    analysisManager->CreateNtupleIColumn("iEvent");
    analysisManager->CreateNtupleIColumn("iPlane");
    // Create Double position columns
    analysisManager->CreateNtupleDColumn("fX");
    analysisManager->CreateNtupleDColumn("fY");
    analysisManager->CreateNtupleDColumn("fZ");
    analysisManager->CreateNtupleDColumn("vertexX");
    analysisManager->CreateNtupleDColumn("vertexY");
    analysisManager->CreateNtupleDColumn("vertexZ");

    // Create Integer Global time column = time that starts when each event begins. Local time = time when the particle is created
    analysisManager->CreateNtupleDColumn("fGlobalTime");
    // Create Double Energy column
    analysisManager->CreateNtupleDColumn("Energy");
    // Create Double corrected Energy column
    analysisManager->CreateNtupleDColumn("CorrEnergy");
    analysisManager->CreateNtupleIColumn("ClSize");
    analysisManager->CreateNtupleDColumn("LeadingEnergy");
    analysisManager->CreateNtupleDColumn("LeadingTime");
    analysisManager->FinishNtuple(0);

>>>>>>> 8149767 (Added digitization + tracking + clustering + analysis in an automatic fashion for each run)
=======
>>>>>>> 8a2e4b8 (Cleaned up the simulation files. Removed the Truth En tree as it was not used further in the analysis chain. Additionally, general clean up in terms of branch renaming. It most probably will impact the in_pixel_plots script. However the default analysis chain has already been modified to account for these changes.)
    // Scattering Angle Ntuple
    analysisManager->CreateNtuple("ScatAngle", "Scatering Angle");
    analysisManager->CreateNtupleIColumn("iEvent");
    // Create Double position columns
    analysisManager->CreateNtupleDColumn("fX");
    analysisManager->CreateNtupleDColumn("fY");
    analysisManager->CreateNtupleDColumn("fZ");
    analysisManager->CreateNtupleDColumn("ScateringAngle");
    analysisManager->CreateNtupleDColumn("MomentumVal");
    analysisManager->FinishNtuple(0);

    analysisManager->CreateH1("ScatteringAngle", "Scattering Angle", 100, 0., 180.0);
    analysisManager->CreateH1("MomentumDistribution", "Momentum Distribution", 100, 0., 190.0 *GeV);

    analysisManager->CreateNtuple("RawPixelHits", "Raw Pixel Hits");
    analysisManager->CreateNtupleIColumn("iEvent");
    analysisManager->CreateNtupleIColumn("iPlane");
    analysisManager->CreateNtupleIColumn("iHit");
    analysisManager->CreateNtupleIColumn("PixX");
    analysisManager->CreateNtupleIColumn("PixY");
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> ebfd7f7 (Added the MC truth primary vertex of particle gun from develop_Vlad branch)
=======
>>>>>>> beda3d3 (Updating src folder with newest develop branch. Before was old version)
    analysisManager->CreateNtupleDColumn("hitTime");
    analysisManager->CreateNtupleDColumn("hitEnergy");
    analysisManager->FinishNtuple(1);
=======
    analysisManager->CreateNtupleDColumn("timeWalkHit");
    analysisManager->CreateNtupleDColumn("Energy");
    analysisManager->FinishNtuple(3);
<<<<<<< HEAD
=======
    analysisManager->CreateNtupleDColumn("hitTime");
    analysisManager->CreateNtupleDColumn("hitEnergy");
    analysisManager->FinishNtuple(1);
>>>>>>> 8a2e4b8 (Cleaned up the simulation files. Removed the Truth En tree as it was not used further in the analysis chain. Additionally, general clean up in terms of branch renaming. It most probably will impact the in_pixel_plots script. However the default analysis chain has already been modified to account for these changes.)

    // MONTE CARLO Truth
    analysisManager->CreateNtuple("TruthVertex", "Monte Carlo Truth Vertex Position");
    // Create Integer Event # column
    analysisManager->CreateNtupleIColumn("iEvent");
    analysisManager->CreateNtupleDColumn("trueVertexX");
    analysisManager->CreateNtupleDColumn("trueVertexY");
    analysisManager->CreateNtupleDColumn("trueVertexZ");
    // Create Integer Global time column = time that starts when each event begins. Local time = time when the particle is created
<<<<<<< HEAD
    analysisManager->CreateNtupleDColumn("fGlobalTime");
    analysisManager->FinishNtuple(4);
<<<<<<< HEAD

    
>>>>>>> 8149767 (Added digitization + tracking + clustering + analysis in an automatic fashion for each run)
=======

    
>>>>>>> 689c0d7 (Added the MC truth primary vertex of particle gun from develop_Vlad branch)
<<<<<<< HEAD
>>>>>>> ebfd7f7 (Added the MC truth primary vertex of particle gun from develop_Vlad branch)
=======
=======
    analysisManager->CreateNtupleDColumn("hitTime");
    analysisManager->CreateNtupleDColumn("hitEnergy");
    analysisManager->FinishNtuple(1);
>>>>>>> fdaa68d (Updating src folder with newest develop branch. Before was old version)
>>>>>>> beda3d3 (Updating src folder with newest develop branch. Before was old version)

    // MONTE CARLO Truth
    analysisManager->CreateNtuple("TruthVertex", "Monte Carlo Truth Vertex Position");
    // Create Integer Event # column
    analysisManager->CreateNtupleIColumn("iEvent");
    analysisManager->CreateNtupleDColumn("trueVertexX");
    analysisManager->CreateNtupleDColumn("trueVertexY");
    analysisManager->CreateNtupleDColumn("trueVertexZ");
    // Create Integer Global time column = time that starts when each event begins. Local time = time when the particle is created
    analysisManager->CreateNtupleDColumn("trueGlobalTime");
    analysisManager->CreateNtupleDColumn("trueEnergy");
    analysisManager->FinishNtuple(2);
=======
>>>>>>> 66f7594 (DEBUG)
=======
    analysisManager->CreateNtupleDColumn("trueGlobalTime");
    analysisManager->CreateNtupleDColumn("trueEnergy");
    analysisManager->FinishNtuple(2);
>>>>>>> 8a2e4b8 (Cleaned up the simulation files. Removed the Truth En tree as it was not used further in the analysis chain. Additionally, general clean up in terms of branch renaming. It most probably will impact the in_pixel_plots script. However the default analysis chain has already been modified to account for these changes.)
}

RunAction::~RunAction()
{

}

void RunAction::BeginOfRunAction(const G4Run *run)
{
    G4AnalysisManager *analysisManager = G4AnalysisManager::Instance();
    // Create run ID
    G4int runID = run->GetRunID();
    std::stringstream strRunID;
    strRunID << runID;
    
    //analysisManager->SetNtupleMerging(true);
    if (isMaster && fFlag->isBatch)
    {
        fOutputPath = CreateNextRunDirectory(true, fFlag);
        DumpConfigToFile(fOutputPath + "/flags.cfg");
    }
    else
    {
        fOutputPath = CreateNextRunDirectory(false, fFlag);
    }

    analysisManager->OpenFile(fOutputPath + "/output" + strRunID.str() + ".root");
}

void RunAction::EndOfRunAction(const G4Run *run)
{
    G4AnalysisManager *analysisManager = G4AnalysisManager::Instance();
    analysisManager->Write();
    analysisManager->CloseFile();
    G4int runID = run->GetRunID();
    G4cout << "Finishing run " << runID <<G4endl;
    
}

// Overload the runID method to increment the run ID
G4Run* RunAction::GenerateRun() {
    static G4int runCounter = 0;
    return new CustomRun(runCounter++);
}
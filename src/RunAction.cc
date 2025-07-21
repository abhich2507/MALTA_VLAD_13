#include "RunAction.hh"
#include "CustomRun.hh"

RunAction::RunAction(SimFlags* flags) : fFlag(flags)
{
    // Simplified instantiation thanks to GEANT4 implementation
    G4AnalysisManager *analysisManager = G4AnalysisManager::Instance();

    // Deposited Energy Ntuple. MONTE CARLO TRUTH
    analysisManager->CreateNtuple("TruthEnDeposited", "Monte Carlo Truth Energy Deposited");
    // Create Integer Event # column
    analysisManager->CreateNtupleIColumn("iEvent");
    // Create Double position columns
    analysisManager->CreateNtupleDColumn("fX");
    analysisManager->CreateNtupleDColumn("fY");
    analysisManager->CreateNtupleDColumn("fZ");
    // Create Integer Global time column = time that starts when each event begins. Local time = time when the particle is created
    analysisManager->CreateNtupleIColumn("fGlobalTime");
    // Create Double Energy column
    analysisManager->CreateNtupleDColumn("Energy");
    // Create Double corrected Energy column
    analysisManager->CreateNtupleDColumn("CorrEnergy");
    analysisManager->CreateNtupleIColumn("ClSize");
    analysisManager->CreateNtupleDColumn("LeadingEnergy");
    analysisManager->CreateNtupleDColumn("LeadingTime");
    analysisManager->FinishNtuple(0);

    // Scattering Angle Ntuple
    analysisManager->CreateNtuple("ScatAngle", "Scatering Angle");
    analysisManager->CreateNtupleIColumn("iEvent");
    // Create Double position columns
    analysisManager->CreateNtupleDColumn("fX");
    analysisManager->CreateNtupleDColumn("fY");
    analysisManager->CreateNtupleDColumn("fZ");
    analysisManager->CreateNtupleDColumn("ScateringAngle");
    analysisManager->CreateNtupleDColumn("MomentumVal");
    analysisManager->FinishNtuple(1);

    analysisManager->CreateH1("ScatteringAngle", "Scattering Angle", 100, 0., 180.0);
    analysisManager->CreateH1("MomentumDistribution", "Momentum Distribution", 100, 0., 190.0 *GeV);

    // Create Ntuple for debugging info
    analysisManager->CreateNtuple("DebuggingInfo", "Debugging Info");
    analysisManager->CreateNtupleIColumn("iEvent");
    // Create Double position columns
    analysisManager->CreateNtupleDColumn("TravelLength");
    analysisManager->FinishNtuple(2);    

    analysisManager->CreateNtuple("RawPixelHits", "Raw Pixel Hits");
    analysisManager->CreateNtupleIColumn("iEvent");
    analysisManager->CreateNtupleIColumn("iHit");
    analysisManager->CreateNtupleIColumn("PixX");
    analysisManager->CreateNtupleIColumn("PixY");
    analysisManager->CreateNtupleDColumn("timeWalkHit");
    analysisManager->CreateNtupleDColumn("Energy");
    analysisManager->FinishNtuple(3);

    

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
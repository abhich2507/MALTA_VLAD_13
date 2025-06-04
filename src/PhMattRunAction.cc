#include "PhMattRunAction.hh"
#include "PhMattCustomRun.hh"

PhMattRunAction::PhMattRunAction()
{
    // Simplified instantiation thanks to GEANT4 implementation
    G4AnalysisManager *analysisManager = G4AnalysisManager::Instance();

    // Deposited Energy Ntuple
    analysisManager->CreateNtuple("EnDeposited", "Energy Deposited");
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
    

}
PhMattRunAction::~PhMattRunAction()
{

}

void PhMattRunAction::BeginOfRunAction(const G4Run *run)
{
    G4AnalysisManager *analysisManager = G4AnalysisManager::Instance();
    // Create run ID
    G4int runID = run->GetRunID();

    std::stringstream strRunID;
    strRunID << runID;
    analysisManager->OpenFile("output" + strRunID.str() + ".root");


}

void PhMattRunAction::EndOfRunAction(const G4Run *run)
{
    G4AnalysisManager *analysisManager = G4AnalysisManager::Instance();
    analysisManager->Write();

    analysisManager->CloseFile();

    G4int runID = run->GetRunID();

    G4cout << "Finishing run " << runID <<G4endl;
    
}

// Overload the runID method to increment the run ID
G4Run* PhMattRunAction::GenerateRun() {
    static G4int runCounter = 0;
    return new PhMattCustomRun(runCounter++);
}
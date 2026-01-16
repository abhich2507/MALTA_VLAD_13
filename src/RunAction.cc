#include "RunAction.h"
#include "CustomRun.h"
#include "G4Run.hh"
#include "G4AnalysisManager.hh"
#include "G4SystemOfUnits.hh"
#include "G4UnitsTable.hh"
#include "Config.h"
#include <filesystem>

RunAction::RunAction(const SimFlags* flags) : m_flag(flags)
{
    // Simplified instantiation thanks to GEANT4 implementation
    G4AnalysisManager *analysisManager = G4AnalysisManager::Instance();

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
    analysisManager->CreateNtupleDColumn("hitTime");
    analysisManager->CreateNtupleDColumn("hitEnergy");
    analysisManager->FinishNtuple(1);

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
}

void RunAction::BeginOfRunAction(const G4Run *run)
{
    G4AnalysisManager *analysisManager = G4AnalysisManager::Instance();
    // Create run ID
    G4int runID = run->GetRunID();
    std::stringstream strRunID;
    strRunID << runID;
    
    //analysisManager->SetNtupleMerging(true);
    std::string saveName{};
    if (isMaster && m_flag->isBatch)
    {
        m_outputPath = CreateNextRunDirectory(true, m_flag);
        DumpConfigToFile(m_outputPath + "/flags.cfg");
    }
    else
    {
        m_outputPath = CreateNextRunDirectory(false, m_flag);
    }
    if(m_flag->macroFileLocal.find("run") != std::string::npos)
    {
        saveName = "/output";    
        analysisManager->OpenFile(m_outputPath + saveName + strRunID.str() + ".root");
    }
    else
    {
        saveName = "/visOut";
        std::string visPath = "../Results/VisOutput/";
        try 
        {
            if (std::filesystem::create_directories(visPath)) 
            {
                G4cout << "Directory created: " << visPath << std::endl;
            } else {
                G4cout << "Directory already exists or failed to create.\n";
            }
        } 
        catch (const std::filesystem::filesystem_error& e) 
        {
                std::cerr << "Error: " << e.what() << std::endl;
        }
        analysisManager->OpenFile(visPath + saveName + ".root");
    }

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
G4Run* RunAction::GenerateRun() 
{
    static G4int runCounter = 0;
    return new CustomRun(runCounter++);
}
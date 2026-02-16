#include "EventAction.h"
#include "G4SDManager.hh"
#include "SensitiveDetector.h"
#include "globals.hh"
#include "G4AutoLock.hh"
#include "G4Event.hh"
#include "G4Run.hh"
#include "G4RunManager.hh"
#include "G4AnalysisManager.hh"
#include <chrono>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>

// Thread safe event progress 
namespace
{
    G4int g4EventCounter = 0;
    G4Mutex g4CounterMutex = G4MUTEX_INITIALIZER;
    std::chrono::time_point<std::chrono::steady_clock> g4StartTime;
    bool gStartTimeInitialized = false;
}

void EventAction::BeginOfEventAction(const G4Event*)
{
    m_trackEdep.clear();
}

void EventAction::addEdep(G4int eventID, G4float energy, G4float timing, G4int planeID, G4int pixX, G4int pixY)
{
    auto key = std::make_tuple(planeID, pixX, pixY);
    auto &entry = m_trackEdep[key];
    G4float beforeEnergy = entry.edep;
    entry.edep += energy;
    if (beforeEnergy < 50 && entry.edep>=50) entry.time = timing;
    entry.eventNum = eventID;
    entry.planeNum = planeID;
    entry.X = pixX;
    entry.Y = pixY;

}

void EventAction::EndOfEventAction(const G4Event*) 
{
    G4AnalysisManager *analysisManager = G4AnalysisManager::Instance();
    // Loop over all tracks (includes all seondaries)
    for (const auto& [key, entry]: m_trackEdep)
    {
        if(entry.edep >= 50)
        {
            analysisManager->FillNtupleIColumn(0, 0, entry.eventNum);
            analysisManager->FillNtupleIColumn(0, 1, entry.planeNum);
            analysisManager->FillNtupleIColumn(0, 2, entry.X);
            analysisManager->FillNtupleIColumn(0, 3, entry.Y);
            // This branch will be modified. Before it as encoding the timewalk of each hit. Now it stores the hit time of a particle.
            analysisManager->FillNtupleFColumn(0, 4, entry.time);
            analysisManager->FillNtupleFColumn(0, 5, entry.edep);
            analysisManager->AddNtupleRow(0); 
            //std::cout << "Saving entry: " << "eventID: " << entry.eventNum << "; X: " << pixelCluster[i][0] << "; Y:" << pixelCluster[i][1] << "; time: " << entry.time << "; energy: " << effAnCopy[i] * entry.edep <<  std::endl;
        }
        
        
    }



    
    // Implementation of a rudimentary progress bar. Added thread safety
    G4AutoLock lock(&g4CounterMutex);
    ++g4EventCounter;
    // Initialize the timing only once in working thread space
    if (!gStartTimeInitialized) 
    {
        g4StartTime = std::chrono::steady_clock::now();
        gStartTimeInitialized = true;
    }

    // Current event
    G4int eventID = g4EventCounter;
    // Total # of events
    G4int totalEvents = G4RunManager::GetRunManager()->GetCurrentRun()->GetNumberOfEventToBeProcessed();

    // GUard against low number of events in vis.mac
    int reportInterval = std::max(1, totalEvents / 100);
    if (eventID % reportInterval == 0 || eventID == totalEvents) {
        // Timing info
        auto now = std::chrono::steady_clock::now();
        std::chrono::seconds::rep elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - g4StartTime).count();
        // Run percentage info
        G4double percent = 100.0 * eventID / totalEvents;
        int eta = 0;
        if (percent > 0.0) 
        {
            double elapsedD = static_cast<double>(elapsed);
            eta = static_cast<int>(elapsedD * (100 / percent));
        }
        // Timing format
        auto formatTime = [](std::chrono::seconds::rep seconds) {
            std::chrono::seconds::rep h = seconds / 3600;
            std::chrono::seconds::rep m = (seconds % 3600) / 60;
            std::chrono::seconds::rep s = seconds % 60;
            std::ostringstream oss;
            if (h > 0) oss << h << "h ";
            if (m > 0 || h > 0) oss << m << "m ";
            oss << s << "s";
            return oss.str();
        };

        std::cout << "\rProgress: " << std::setw(6) << std::fixed << std::setprecision(1)
                  << percent << "% completed" << ". Current event: " << eventID << ". Out of total events: " << totalEvents << ". Elapsed: " << formatTime(elapsed) << " | "
                  << "ETA: " << formatTime(eta) << std::flush;

        if (eventID == totalEvents)
            std::cout << std::endl;
    }
    
}
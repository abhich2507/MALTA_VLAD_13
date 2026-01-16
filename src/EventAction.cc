#include "EventAction.h"
#include "globals.hh"
#include "G4AutoLock.hh"
#include "G4Event.hh"
#include "G4Run.hh"
#include "G4RunManager.hh"
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

void EventAction::EndOfEventAction(const G4Event*) 
{
    
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
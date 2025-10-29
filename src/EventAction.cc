#include "EventAction.hh"

// Thread safe event progress 
namespace
{
    G4int g4EventCounter = 0;
    G4Mutex g4CounterMutex = G4MUTEX_INITIALIZER;
    std::chrono::time_point<std::chrono::steady_clock> g4StartTime;
    bool gStartTimeInitialized = false;
}


EventAction::EventAction()
{
    
}

EventAction::~EventAction()
{
   
}

void EventAction::BeginOfEventAction(const G4Event* event)
{

}

void EventAction::EndOfEventAction(const G4Event* event) 
{
    // Implementation of a rudimentary progress bar. Added thread safety
    G4AutoLock lock(&g4CounterMutex);
    ++g4EventCounter;
    // Initialize the timing only once in working thread space
    if (!gStartTimeInitialized) {
        g4StartTime = std::chrono::steady_clock::now();
        gStartTimeInitialized = true;
    }

    // Current event
    G4int eventID = g4EventCounter;
    // Total # of events
    G4int totalEvents = G4RunManager::GetRunManager()->GetCurrentRun()->GetNumberOfEventToBeProcessed();

    int barWidth = 50;
    // Guard against low number of events in vis.mac
    int reportInterval = std::max(1, totalEvents / 100);
    if (eventID % reportInterval == 0 || eventID == totalEvents) {
        // Timing info
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - g4StartTime).count();
        // Run percentage info
        G4double percent = 100.0 * eventID / totalEvents;
        int eta = (percent > 0.0) ? static_cast<int>((elapsed * (100 / percent))) :0;

        // Timing format
        auto formatTime = [](int seconds) {
            int h = seconds / 3600;
            int m = (seconds % 3600) / 60;
            int s = seconds % 60;
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
#include <iostream>
#include "G4RunManager.hh"
#include "G4MTRunManager.hh"
#include "G4UImanager.hh"
#include "G4VisManager.hh"
#include "G4VisExecutive.hh"
#include "G4UIExecutive.hh"

#include "PhMattPhysicsList.hh"
#include "PhMattDetectorConstruction.hh"
#include "PhMattActionInitialization.hh"
#include "PhMattTrackingAction.hh"

int main (int argc, char** argv)
// argc = argument count; argv = argument value - Command line arguments
{
    // *ui is a pointer to an object of class G4UIExecutive created with new
    // -> to access the object at the pointer
    // new allocates object in the memory heap (stack) and return pointer to its location
    // Create UI session. G4UIExecutive = Geant4 class that manages UI stuff
    // In large statistics runs better to not use gui. Commenitng the line below and initializing later is more flexible.
    G4UIExecutive *ui = new G4UIExecutive(argc, argv);
    //G4UIExecutive *ui;
    // Preprocessor directive evaluated before compilation. Depending if G4MULTITHREAD
    // is defined select between single and multi threading operation of the run manager
    #ifdef G4MULTITHREADED
        G4MTRunManager *runManager = new G4MTRunManager;
    #else
        G4RunManager *runManager = new G4RunManager;
    #endif


    // Physics List Initialization
    runManager->SetUserInitialization(new PhMattPhysicsList());

    // Detector Construction Initialization
    runManager->SetUserInitialization(new PhMattDetectorConstruction());

    // Action Initialization
    runManager->SetUserInitialization(new PhMattActionInitialization());

    // GUI mode initializes only when more than 1 command line argument is passed
    /*
    if(argc ==1)
    {   G4cout << "One argument" << G4endl;
        ui = new G4UIExecutive(argc, argv);
    }
    */

    // Sets up visualization manager.
    G4VisManager *visManager = new G4VisExecutive();
    // Initializes the visualisation drivers(OpenGL,...)
    visManager->Initialize();
    // Pointer to the global UI manager instance. :: = scope resolution operator 
    // Calls a static method GetUIpointer() from the G4UImanager class without instantiation
    // This allows for using commands such as run/beamOn or vis/open/OGL
    G4UImanager *UImanager = G4UImanager::GetUIpointer();
    // Import visualization macro vis.mac. Added logic to account for run.mac large stat runs
    if(argc > 1)
    {
        G4String command = "/control/execute ";
        // File name passed as the second command line argument
        G4String fileName =  argv[1];
        UImanager->ApplyCommand(command + fileName);
        if (fileName.substr(0, 3) == "vis") {
            G4cout << "Macro identified for visualization." << G4endl;
            ui->SessionStart(); // blocks program for visualization until GUI closes
        }
    }
    else
    {
        UImanager->ApplyCommand("/control/execute vis.mac");
        // Executes the interactive terminal i.e. starts the UI. 
        ui->SessionStart();

    }

    // Delete in reverse order of creation
    delete ui;
    delete visManager;
    delete runManager; 


    return 0;

}
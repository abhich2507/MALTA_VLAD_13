#include <iostream>
#include <filesystem>
#include <ctime>
#include "G4RunManager.hh"
#include "G4MTRunManager.hh"
#include "G4UImanager.hh"
#include "G4VisManager.hh"
#include "G4VisExecutive.hh"
#include "G4UIExecutive.hh"
#include "Config.h"
#include "PhysicsList.h"
#include "DetectorConstruction.h"
#include "ActionInitialization.h"
#include "TrackingAction.h"
#include "SubmissionTests.h"


int main (int argc, char** argv)
// argc = argument count; argv = argument value - Command line arguments
{
    // Set random seed
    G4long seed = time(NULL);
    CLHEP::HepRandom::setTheSeed(seed);
    auto flags = new SimFlags;
    bool testRun = false;

    // make the Results directory
    try 
    {
        if (std::filesystem::create_directory("../Results")) 
        {
            std::cout << "Directory created: " << "../Results" << std::endl;
        } else {
            std::cout << "Directory already exists or failed to create.\n";
        }
    } 
    catch (const std::filesystem::filesystem_error& e) 
    {
            std::cerr << "Error: " << e.what() << std::endl;
    }
    // make the Plots directory
    try 
    {
        if (std::filesystem::create_directory("../Plots")) 
        {
            std::cout << "Directory created: " << "../Plots" << std::endl;
        } else {
            std::cout << "Directory already exists or failed to create.\n";
        }
    } 
    catch (const std::filesystem::filesystem_error& e) 
    {
            std::cerr << "Error: " << e.what() << std::endl;
    }
    //std::filesystem::create_directories(CreateNextRunDirectory()); 
    // *ui is a pointer to an object of class G4UIExecutive created with new
    // -> to access the object at the pointer
    // new allocates object in the memory heap (stack) and return pointer to its location
    // Create UI session. G4UIExecutive = Geant4 class that manages UI stuff
    // In large statistics runs better to not use gui. Commenitng the line below and initializing later is more flexible.
    // G4UIExecutive *ui = new G4UIExecutive(argc, argv);
    G4UIExecutive* ui = nullptr; // Needs explicit null pointer to ensure the memory is reset
    // Preprocessor directive evaluated before compilation. Depending if G4MULTITHREAD
    // is defined select between single and multi threading operation of the run manager
    const char* configPath = std::getenv("SIMU_CONFIG");
    if (configPath) 
    {
        LoadSimFlagsFromFile(configPath, *flags);
    } 
    else 
    {
        // fallback or error
        std::cout<< std::endl<<"NO CONFIGURATION of the SIMU_CONFIG path. Reverting to DEFAULT" 
                 << std::endl<< "Configure it via: export SIMU_CONFIG=/path/to/my/file " << std::endl;
        
        LoadSimFlagsFromFile("../configs/flags.cfg", *flags);
    }
    //From local paths to absolute paths using env var:
    std::string localPath = std::getenv("LOCAL_PATH");
    std::string localName = flags->outputPathLocal;
    flags->outputPathLocal = localPath + localName;
    std::string localMacro = flags->macroFileLocal;
    flags->macroFileLocal = localPath + "build/" + localMacro;

    // Dry run test
    submissionTest(*flags);
    std::string homePath = std::getenv("HOME") ? std::getenv("HOME") : "";

    // Check if running in local or naf mode and set the fFlag runMode appropriately. This flag is not set via the flags file.
    if(homePath.empty() || homePath.find("desy.de")!= std::string::npos)
    {
        std::cout<< "\n" << "You are running from NAF!"<< "\n";
        flags->runMode = "naf";

    }
    else
    {
        std::cout<< "You are running locally!"<< "\n";
        flags->runMode = "local";
    }

    #ifdef G4MULTITHREADED
        G4MTRunManager *runManager = new G4MTRunManager;
        // Offer the user two implementation methods for number of threads. One via config file
        // easy to use for PC use. ther one via env variable. useful for cloud computing
        // The local running assumes the local machine is configured as home
        if(flags->runMode == "local") 
        {
            runManager->SetNumberOfThreads(flags->numThreadsLocal);
        }
        else 
        {
            runManager->SetNumberOfThreads(flags->numThreadsNAF);
        }


    #else
        G4RunManager *runManager = new G4RunManager;
    #endif

    // Check to see if this is a dry run.
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--test") {
            flags->macroFileLocal = "run_test.mac";
            testRun = true;
        }
    }

    // Physics List Initialization
    runManager->SetUserInitialization(new PhysicsList(flags));

    // Detector Construction Initialization
    runManager->SetUserInitialization(new DetectorConstruction(flags));

    // Action Initialization
    runManager->SetUserInitialization(new ActionInitialization(flags));

    // Sets up visualization manager.
    G4VisManager *visManager = new G4VisExecutive("quiet");
    // Initializes the visualisation drivers(OpenGL,...)
    visManager->Initialize();
    // Pointer to the global UI manager instance. :: = scope resolution operator 
    // Calls a static method GetUIpointer() from the G4UImanager class without instantiation
    // This allows for using commands such as run/beamOn or vis/open/OGL
    G4UImanager *UImanager = G4UImanager::GetUIpointer(); 
    // Import visualization macro vis.mac. Added logic to account for run.mac large stat runs
    if(flags->macroFileLocal.find("vis") != std::string::npos)
    {   
        //TODO: Implemnt exceptions to exit gracefully before job submission
        flags->isBatch = false;
        ui = new G4UIExecutive(argc, argv);
        UImanager->ApplyCommand("/control/execute " + flags->macroFileLocal);
        // Executes the interactive terminal i.e. starts the UI. 
        ui->SessionStart();
    }
    else if (testRun == false)
    {
        flags->isBatch = true;
        if(flags->runMode == "local") 
        {
            UImanager->ApplyCommand("/control/execute " + flags->macroFileLocal);
        }
        else 
        {
            std::string nafUser = std::getenv("NAF_USER");
            std::string nafPath = std::getenv("NAF_PATH");
            std::string nafFullPath = "/afs/desy.de/user/" + nafUser.substr(0,1) + "/" + nafUser + "/" + nafPath;
            std::string nafName = flags->outputPathNAF;
            flags->outputPathNAF = nafFullPath + "/" + nafName;
            std::string nafMacro = flags->macroFileNAF;
            flags->macroFileNAF = nafFullPath + + "/build/" + nafMacro;
            UImanager->ApplyCommand("/control/execute " + flags->macroFileNAF);
        }
        
        // Add configurable lines to the run.mac file. The runBeamOn is implemented below
        std::ostringstream cmd;
        cmd << "/run/beamOn " << flags->numEvents;
        UImanager->ApplyCommand(cmd.str());

    }

    std::cout << "\n" << "Simulation completed. Good day!" << "\n";
    return 0;

}
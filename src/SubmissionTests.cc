#include "SubmissionTests.hh"

void submissionTest(SimFlags& flags)
{
    int submitCpus = GetRequestCpusFromSubmitFile("/home/vlad/Documents/Simu/Geant4/DECAL_REPO/build/job.submit");
    if (submitCpus != -1 && submitCpus != flags.numThreadsNAF) 
    {
        std::cerr << "FATAL ERROR: Number of requested CPUs (" << submitCpus << ") does not match number of threads (" << flags.numThreadsNAF << ")!" << std::endl;
        std::exit(EXIT_FAILURE);        
    }

    

}

#include <TFile.h>
#include <TTree.h>
#include <iostream>
#include "EfficiencyModel.cc"

int MALTAPreProcessing(std::string inputName="ElectronBeam_withSPSBeamline_LT_e-_149.118GeV_N20_withMALTApix")
{
    bool verbose = false;
    // --- open ROOT file ---
    std::string inputFolder = "../output/";
    TFile* inFile = TFile::Open((inputFolder+inputName+".root").c_str(), "READ");
    if (!inFile || inFile->IsZombie()) {
        std::cerr << "ERROR: cannot open inFile\n";
        return 1;
    }

    // --- get tree ---
    TTree* inTree = nullptr;
    inFile->GetObject("1Y_cpNum2", inTree);
    if (!inTree) {
        std::cerr << "ERROR: inTree siliconlayer0 not found\n";
        return 1;
    }

    // --- variables ---
    Int_t  eventID;
    Float_t x_mm, y_mm, z_mm;
    Float_t fglobaltime_ns;
    Float_t energy_keV;

    // --- set branch addresses ---
    inTree->SetBranchAddress("iEvent", &eventID);
    inTree->SetBranchAddress("fPreX", &x_mm);
    inTree->SetBranchAddress("fPreY", &y_mm);
    inTree->SetBranchAddress("fPreZ", &z_mm);
    inTree->SetBranchAddress("fGlobalTime", &fglobaltime_ns);
    inTree->SetBranchAddress("fEdep", &energy_keV);

    Long64_t nEntries = inTree->GetEntries();
    std::cout << "Source entries: " << nEntries << std::endl;

    // define variables:
    double pixelSize = 0.0364; // m_flag->pixelSize *mm; // todo; read these from config ?
    // specific for Y-layer:

    double detX = 0.; // 1.86368*4; // m_flag->detectorSizeX *cm; 63.96; 
    double detY = 0.; // 1.86368*4; // m_flag->detectorSizeY *cm; 63.56; 
    
    double detXOffset = -4.42*10; // m_flag->detectorXOffset *cm; // todo: fill actual X value 
    double detYOffset = -1.288*10; // m_flag->detectorYOffset *cm; // todo: fill actual Y value

    const std::array<std::array<std::array<int, 2>, 4>, 4> deltaTable = 
    {{
        // flag = 0b00
        {{{-1, -1}, {0, -1}, {-1, 0}, {0, 0}}},
        // flag = 0b01
        {{{ 0, -1}, {1, -1}, { 0, 0}, {1, 0}}},
        // flag = 0b10
        {{{-1,  0}, {0,  0}, {-1, 1}, {0, 1}}},
        // flag = 0b11
        {{{ 0,  0}, {0,  1}, { 1, 0}, {1, 1}}}
    }};

    // --- create output file and tree ---
    TFile* outFile = new TFile(("Results/RawMALTA_"+inputName+".root").c_str(),"RECREATE");
    TTree* outTree = new TTree("RawPixelHits","Raw MALTA pixe hits");

    Int_t t_eventID, t_planeID, t_iHit;
    Float_t t_x, t_y, hitEnergy, hitTime;

    outTree->Branch("iEvent",&t_eventID);//,"eventID/I");
    outTree->Branch("iPlane",&t_planeID);//,"planeID/I");
    outTree->Branch("iHit",&t_iHit);//,"iHit/I");
    outTree->Branch("PixX",&t_x);//,"x/F");
    outTree->Branch("PixY",&t_y);//,"y/F");
    outTree->Branch("hitTime",&hitTime);//,"hitEnergy/F");
    outTree->Branch("hitEnergy",&hitEnergy);//,"hitEnergy/F");

    // --- loop over hits from Geant4 Output ---
    // todo: should this be sorted by iEvent? Maybe not, because efficiency modelling is independent of Event and other depositions.
    for (Long64_t iDep = 0; iDep < nEntries; ++iDep) {
        inTree->GetEntry(iDep);

        // --- Z selection: first 30 um ---
        //if (z_mm < 0.0 || z_mm > 0.030)  // need to subtract offset first
        //    continue;

        // --- In-pixel coordinates
        // todo: subtract golbal offset where readout pixels start. Currently no subtraction
        double x_IP = std::fmod(x_mm, pixelSize); // modulo pixel size
        if(x_IP < 0) x_IP += pixelSize;
        double y_IP = std::fmod(y_mm, pixelSize);
        if(y_IP < 0) y_IP += pixelSize;

        // --- efficiency from XY ---
        auto [effAn, quadrantFlag] =
            getEfficiencyAnalytical(x_IP, y_IP); // pass in mm

        // total collected fraction (sum of 4 pixels = 1)
        double effSum =
            effAn[0] + effAn[1] +
            effAn[2] + effAn[3];

        // todo: get seed pixel:
        int pixX = static_cast<int>((x_mm - detXOffset + detX / 2) / pixelSize);
        int pixY = static_cast<int>((y_mm - detYOffset + detY / 2) / pixelSize);

        std::array<std::array<int, 2>, 4> pixelCluster;
        for (std::array<std::array<int, 2>, 4>::size_type i = 0; i < 4; ++i) 
        {
            pixelCluster[i] = {pixX, pixY};
        }
        const auto& deltas = deltaTable[quadrantFlag];
        
        for(std::array<std::array<int, 2>, 4>::size_type i = 0; i<4; i++)
        {
            pixelCluster[i][0] +=deltas[i][0] ;
            pixelCluster[i][1] +=deltas[i][1] ;
        }

        double epsilon = 3.66; // electron-hole pair creaton energy = (3.66 +- 0.03) eV
        std::array<double,4> effAnCopy = effAn; // forces evaluation
        int iHit = 0;

        for(std::array<std::array<int, 2>, 4>::size_type i = 0; i<4; i++)
        {
            t_eventID = eventID;
            t_planeID = int(pixelCluster[i][0]/512)+10*int(pixelCluster[i][1]/512); // least significant digit for xID, and more significant digit for yID
            t_iHit = iHit;
            t_x = pixelCluster[i][0];
            t_y = pixelCluster[i][1];
            hitTime = fglobaltime_ns;
            hitEnergy = effAnCopy[i] * energy_keV * 1000 / epsilon;
            outTree->Fill();
            iHit++;
        }

        // --- example output ---
        if (verbose){
            if (eventID ==0){
            std::cout << "Evt " << eventID << "  (X,Y)= (" << pixX << "," << pixY << ")  z=" << z_mm << " mm, edep=" << energy_keV << " keV, eff0=" << effAn[0] << ",  effSum=" << effSum << "  quadrant=" << int(quadrantFlag) << std::endl;
            std::cout << "InPixPos: " << x_IP*1000. << ", " << y_IP*1000. << " --> En: " << energy_keV << " of 4 pixels: " << effAn[0] << " " << effAn[1] << " " << effAn[2] << " " << effAn[3] << " " << std::endl;
            std::cout << "Pix coordinate: " << pixX << ";" << pixY << std::endl;

            std::cout << "Cluster pixel positions: "<< std::endl
            << pixelCluster[0][0] << "," << pixelCluster[0][1] << " ; " << std::endl 
            << pixelCluster[1][0] << "," << pixelCluster[1][1] << " ; " << std::endl 
            << pixelCluster[2][0] << "," << pixelCluster[2][1] << " ; " << std::endl 
            << pixelCluster[3][0] << "," << pixelCluster[3][1] << "  " << std::endl;
            }
        }
    }

    outTree->Write();
    outFile->Close();

    inFile->Close();
    return 0;
}

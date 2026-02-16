#include "DigitalProcessing.hh"

void FIFOProcessing(double inputThreshold, int runNumber, std::string saveName)
{

    auto start = std::chrono::high_resolution_clock::now();
    // Set all the analysis flags for the digital processing
    auto analysisFlags = new SimFlags{};
    const char* configPath = std::getenv("ANALYSIS_CONFIG");
    LoadAnalysisFlagsFromFile(configPath, *analysisFlags);

    bool verbose = analysisFlags->verboseDigital;
    double T = analysisFlags->T;
    double Tdiv = analysisFlags->Tdiv;
    double TrefThr = analysisFlags->TrefThr;
    double n = analysisFlags->n;
    double x0 = analysisFlags->x0;
    double t0 = analysisFlags->t0;
    int groupSize  = analysisFlags->groupSize;
    int groupSizeX = analysisFlags->groupSizeX;
    int groupSizeY = analysisFlags->groupSizeY;
    int groupLeng  = analysisFlags->groupLeng;
    int parityLeng = analysisFlags->parityLeng;
    int dColLeng   = analysisFlags->dColLeng;
    int numThreads = analysisFlags->numThreads; 
    double wordSpacing = analysisFlags->wordSpacing;
    double relativeThresholdSmearingMean = analysisFlags->meanSmearing;
    double relativeThresholdSmearingCol = analysisFlags->colSmearing;
    int fifoMultiplicity = analysisFlags->fifoMultiplicity;
    int pixXNum = 512;
    int pixYNum = 512;
    int groupRepetition = 32;

    std::string localPath = analysisFlags->localPath;
    std::string inputPath = analysisFlags->inputPath+Form("_%04d/", runNumber);
    std::string directoryPath = localPath + "Plots/";
    std::string runPath = Form("local_%04d/", runNumber);

    std::cout << "############################# FIFO Processing started for:" << std::endl;
    std::cout << inputPath << std::endl;
    // Extract raw data
    TChain *chainPixel = new TChain("RawPixelHits");
    for (int t = 0; t <= numThreads - 1; ++t) 
    {
        chainPixel->Add(Form("%soutput0_t%d.root", inputPath.c_str() , t));
    }
    float corrEnergy_float, timeWalkHit_float;
    int rawEventID, planeID, pixX, pixY;
    chainPixel->SetBranchAddress("iEvent", &rawEventID);
    chainPixel->SetBranchAddress("iPlane", &planeID);
    chainPixel->SetBranchAddress("PixX", &pixX);
    chainPixel->SetBranchAddress("PixY", &pixY);
    chainPixel->SetBranchAddress("hitTime", &timeWalkHit_float); // TODO change var name
    chainPixel->SetBranchAddress("hitEnergy", &corrEnergy_float); // TODO change var name
    Long64_t nRawEntries = chainPixel->GetEntries();
    // Generate threshold map
    auto thresholdMap = generateThrMap(inputThreshold, pixXNum, pixYNum, groupRepetition, relativeThresholdSmearingCol, relativeThresholdSmearingMean, directoryPath, runPath, saveName);

    // Avoid O(n^2) nested loops via extra map 
    std::map<std::pair<int,std::pair<int, int>>, double> enMap; 
    std::map<std::pair<int,std::pair<int,int>>, std::vector<double>> timeMap;
    int eventIDHolder =0;
    int nPlanes = 1;

    std::vector<std::pair<__uint128_t,double>> fifo{};

    for (int i = 0; i< nPlanes; i++)
    {
        enMap.clear();
        timeMap.clear();
        // Sum up all hits in an event per pixel. This assumes all energy is collected instantly and 
        // timing cuts will be made only based on time walk and particle travel time.
        for (Long64_t j = 0; j < nRawEntries; j++)
        {
            chainPixel->GetEntry(j);
            double corrEnergy = static_cast<double>(corrEnergy_float);
            double timeWalkHit = static_cast<double>(timeWalkHit_float);
            //std::cout <<  "Float: " << corrEnergy_float << "; Double: " << corrEnergy << std::endl;
            if (planeID != i) continue;
            enMap[{rawEventID, {pixX, pixY}}] += corrEnergy;
            timeMap[{rawEventID, {pixX, pixY}}].push_back(timeWalkHit);
            eventIDHolder = rawEventID;
        }
        // Now I have energy and time maps of events in the pixel matrix
        __uint128_t maltaPixel;
        __uint128_t maltaGroup;
        __uint128_t maltaParity;
        __uint128_t maltaDelay;
        __uint128_t maltaDColumn;
        std::vector<std::vector<std::pair<__uint128_t, double>>> digitizedWords;
        std::vector<std::pair<__uint128_t, double>> merger;
        // Sort all words based on the timing. 
        std::vector<std::pair<std::pair<int,std::pair<int,int>>, double>> sortedTimings;

        for (const auto& entry : enMap) 
        {
            int eventID = entry.first.first;
            int pixX = entry.first.second.first;
            int pixY = entry.first.second.second;

            auto itThr = thresholdMap.find({pixX, pixY});
            if (itThr == thresholdMap.end()) continue;

            double threshold = itThr->second;
            double cenergy   = entry.second;
            double timing    = GetTimingOffset(cenergy, threshold, T, Tdiv, TrefThr, x0, n, t0);

            if(pixX < 0 || pixX > 511 || pixY < 0 || pixY > 511) std::cout << "Warning! Out of bounds pixels! pixX: " << pixX << "; pixY: " << pixY << std::endl; 


            auto it = timeMap.find({eventID, {pixX, pixY}});
            if(it == timeMap.end()) continue;
            // Row correction also of 7ns/ 512 rows + global GEANT4 timestamp
            //if(verbose) std::cout << "Event ID: " << eventID << "; pixX: " << pixX << ";pixY: " << pixY << "; corrEnergy: " << cenergy << "; timewalk: "<< timing << std::endl;
            timing += *std::max_element(it->second.begin(), it->second.end()) + pixX * 0.0125; 

            sortedTimings.emplace_back(entry.first,timing);
        }
        std::sort(sortedTimings.begin(), sortedTimings.end(),
                [](auto &a, auto &b){ return a.second < b.second; });

        // Now I have events sorted by the global timing. For most applications this simply sorts the hits in a cluster and retain eventID
        // However, for very high rate beams the eventIDs could not be retained anymore.

        double t0 = sortedTimings.begin()->second;

        //double timing = 0;
        //double prevTiming = 0;
        //double timeFIFO = 0;
        double FIFOFrequency = analysisFlags->fifoFrequency; //ns
        int    fifoSize = analysisFlags->fifoSize; // 1215752192 max int
        std::vector<int> vfifoFill(fifoMultiplicity, 0);
        //std::vector<double> vtiming(512, 0);
        std::vector<double> vprevTiming(fifoMultiplicity, 0);
        std::vector<double> vtimeFIFO(fifoMultiplicity,0);

        for (const auto& entry : sortedTimings) 
        {
            int eventID = entry.first.first;
            int pixX   = entry.first.second.first;
            int pixY   = entry.first.second.second;
            double timing = entry.second; 
            auto itThr = thresholdMap.find({pixX, pixY});
            if (itThr == thresholdMap.end()) 
            {
                std::cout << "Missing threshold for pixX=" << pixX
                        << " pixY=" << pixY << std::endl;
                continue;
            }
            double threshold = itThr->second;
            auto it = enMap.find({eventID, {pixX, pixY}});
            double cenergy = it->second;
        
            if (cenergy < threshold) continue;

            int lane = pixX%fifoMultiplicity;

            
            double prevTiming = vprevTiming[lane];
            
            __uint128_t word = encodeWord(pixX, pixY, groupSizeX, groupSizeY, groupLeng, parityLeng, dColLeng, verbose);
            double timeDiff = timing - prevTiming;
            vtimeFIFO[lane] += timeDiff;
            if(verbose)std::cout << "timing: " << timing << "; prevTiming: " << prevTiming << "; timeDiff: " << timeDiff << "; timeFIFO: "<< vtimeFIFO[lane] << "; div: " << vtimeFIFO[lane] / FIFOFrequency  << "; vtimeFIFO: " << vtimeFIFO[lane];
            
            int& fifoFill = vfifoFill[lane];
            int nRead = floor(vtimeFIFO[lane] / FIFOFrequency);
            if(nRead >= 1)
            {
                 
                for(int k = 0; k < nRead; k++) 
                {
                    if (fifoFill) fifoFill--;
                }
                vtimeFIFO[lane] -= nRead * FIFOFrequency;
            }

            if(verbose)std::cout << "; subt: " << vtimeFIFO[lane] <<"; lane: " << lane <<" fifoFill: " << fifoFill << std::endl;
            if(fifoFill < fifoSize)
            {
                fifo.push_back({word, timing});
                fifoFill++;
            }


            vprevTiming[lane] = timing;
            
            
            
        }
    

        mkdir((inputPath + saveName).c_str(), 0777);
        TFile *outfile = new TFile((inputPath + saveName + "/Plane" + std::to_string(i) + "ReconstructedHitsThr" + std::to_string(int(inputThreshold)) + ".root").c_str(), "RECREATE");

        // Create a TTree
        TTree *recontructedTree = new TTree("ReconstructedHits", "Reconstructed Hits");
        // Variables for branches
        int reconstructedPixX, reconstructedPixY, nHits;
        double reconstructedTiming;
        // Create branches
        recontructedTree->Branch("PixX", &reconstructedPixX, "PixX/I");
        recontructedTree->Branch("PixY", &reconstructedPixY, "PixY/I");
        recontructedTree->Branch("timing", &reconstructedTiming, "timing/D");
        recontructedTree->Branch("NHits", &nHits, "NHits/I");
        for (const auto &word: fifo)
        {
            std::vector< std::pair<std::pair<int,int>, int> > pixelPositions = decodedDigitalWord(word.first, groupSize, groupSizeX, groupSizeY, groupLeng, parityLeng, dColLeng);
            for (const auto& pos : pixelPositions) 
            {
                reconstructedTiming = word.second;
                reconstructedPixX = pos.first.first;
                reconstructedPixY = pos.first.second;
                nHits = pos.second;
                recontructedTree->Fill();
            }
        
        }
        recontructedTree->Write();
        outfile->Close();
        std::cout << "Finishing up analyzing Plane" << i << std::endl;
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> elapsed = end - start;
    std::cout << "############################# FIFO Processing stopped after " << elapsed.count() << "ms" << std::endl;
}
#include "DigitalProcessing.hh"

void PRIOFIFOFullDigitalProcessing(double inputThreshold, int runNumber, std::string saveName)
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
            timing += *std::max_element(it->second.begin(), it->second.end()) + pixY * 0.0125;  // 0.390625 for miniMALTA3?

            sortedTimings.emplace_back(entry.first,timing);
        }
        std::sort(sortedTimings.begin(), sortedTimings.end(),
                [](auto &a, auto &b){ return a.second < b.second; });

        // Now I have events sorted by the global timing. For most applications this simply sorts the hits in a cluster and retain eventID
        // However, for very high rate beams the eventIDs could not be retained anymore.

        double t0 = sortedTimings.begin()->second;

        //double FIFOFrequency = analysisFlags->fifoFrequency; //ns
        //int    fifoSize = analysisFlags->fifoSize; // 1215752192 max int
        //std::vector<double> vtiming(512, 0);
        std::vector<std::pair<__uint128_t, double>> words{};
        std::vector<std::pair<__uint128_t, double>> groupMergedWords{};
        __uint128_t groupMerger{};
        double slowcontrolDelay = analysisFlags->slowcontrolDelay;
        double busMergingThreshold = analysisFlags->busMergingThreshold;
        double SRAMFrequency = analysisFlags->SRAMFrequency;
        int sramDepth = analysisFlags->sramDepth;
        double FIFOFrequency = analysisFlags->FIFOFrequency;
        int fifoSize = analysisFlags->fifoSize;

        //std::cout << "slowcontrolDelay: " << slowcontrolDelay << " busMergingThreshold " <<busMergingThreshold << "SRAMFrequency " << SRAMFrequency << " sramDepth " << sramDepth << " FIFOFrequency " << FIFOFrequency << " fifoSize " << fifoSize << std::endl;


        double prevTiming{};
        int count = 0;
        for (const auto& entry : sortedTimings) 
        {
            int eventID = entry.first.first;
            int pixX   = entry.first.second.first;
            int pixY   = entry.first.second.second;
            double timing = entry.second; 
            count++;
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
            
            __uint128_t word = encodeWord(pixX, pixY, groupSizeX, groupSizeY, groupLeng, parityLeng, dColLeng, verbose);
            if (count == 1) 
            {
                prevTiming = timing;
                groupMerger = word;
            }
            words.push_back(std::make_pair(word, timing));
        }
        
        // I. Group level merging of hits.
        std::map<std::pair<int,int>, std::vector<std::pair<__uint128_t, double>>> groupWords;
        std::unordered_map<int, std::vector<std::pair<__uint128_t, double>>> firstBusWords;

        for (auto&[word, timing] : words)
        {
            //std::cout << std::bitset<30>(word) << std::endl;
            // This is not generalized for any bit size
            int doubleColumn = word& 0xFF;
            int parity = (word & ((__uint128_t)1 << 8)) != 0;
            //std::cout <<"DC: " << doubleColumn << "Parity: " << parity << std::endl;
            int bus = ( parity + 1) * doubleColumn;
            firstBusWords[bus].push_back({word,timing});
        }
        for (auto& [bus, words]: firstBusWords)
        {
            for (auto& [word, timing]: words)
            {
                __uint128_t group = (word >> 9) & ((__uint128_t)0x1F);
                groupWords[{group,bus}].push_back({word, timing});
            }
        }
        std::vector<std::pair<__uint128_t, double>> wordsAfterGroup{};
        for (auto& [ID, words]: groupWords)
        {
            //std::cout << "Group " << ID.first << "DC: " << ID.second << std::endl;
            std::sort(words.begin(), words.end(), [](auto &a, auto &b) {return a.second < b.second;});
            std::vector<std::pair<__uint128_t, double>> groupMerged;
            for (size_t i = 0; i < words.size(); i++)
            {
                auto [word, timing] = words[i];
                if (groupMerged.empty())
                {
                    groupMerged.push_back({word, timing});
                    continue;
                }
                auto &[lastWord, lastTime] = groupMerged.back();

                if(timing - lastTime <= slowcontrolDelay)
                {
                    //std::cout << "lastWord: " << std::bitset<30>(lastWord) << std::endl << "Word:     " << std::bitset<30>(word) << std::endl;
                    lastWord|=word;
                    //std::cout << " Merged:  " << std::bitset<30>(lastWord) << std::endl;
                }
                else
                {
                    groupMerged.push_back({word, timing});
                }
                
            }
            wordsAfterGroup.insert(wordsAfterGroup.end(), groupMerged.begin(), groupMerged.end());
            //words.swap(groupMerged);
        }

        //wordsAfterGroup = words;
        // II. Bus level merging of hits
        std::unordered_map<int, std::vector<std::pair<__uint128_t, double>>> busWords;
        for (auto&[word, timing] : wordsAfterGroup)
        {
            //std::cout << std::bitset<30>(word) << std::endl;
            // This is not generalized for any bit size
            int doubleColumn = word& 0xFF;
            int parity = (word & ((__uint128_t)1 << 8)) != 0;
            //std::cout <<"DC: " << doubleColumn << "Parity: " << parity << std::endl;
            int bus = ( parity + 1) * doubleColumn;
            busWords[bus].push_back({word,timing});
        }
        std::vector<std::pair<__uint128_t, double>> wordsAfterBus{};
        for (auto& [busID,words]: busWords)
        {
            // Sort by timing
            std::sort(words.begin(), words.end(), [](auto &a, auto &b) {return a.second < b.second;});
            std::vector<std::pair<__uint128_t, double>> busMerged;
            for (size_t i = 0; i < words.size(); i++)
            {
                auto [word, timing] = words[i];
                if (busMerged.empty())
                {
                    busMerged.push_back({word, timing});
                    continue;
                }
                auto &[lastWord, lastTime] = busMerged.back();

                if(timing - lastTime <= busMergingThreshold)
                {
                    //std::cout << "lastWord: " << std::bitset<30>(lastWord) << std::endl << ";   word: " << std::bitset<30>(word) << std::endl;
                    lastWord|=word;
                    //std::cout << "  Merged: " << std::bitset<30>(lastWord) << std::endl;
                }
                else
                {
                    busMerged.push_back({word, timing});
                }
                
            }

            wordsAfterBus.insert(wordsAfterBus.end(), busMerged.begin(), busMerged.end());
            //words.swap(busMerged);
        }
        //wordsAfterBus = words;
        std::sort(wordsAfterBus.begin(), wordsAfterBus.end(), [](auto &a, auto &b){return a.second < b.second;});
        
        // III. SYNC Memory. ROUND ROBIN Drain
        std::vector<int> memoryModule(512,0); 
        //std::vector<double> vprevTiming{512,0.};
        double prevGlobalTiming{0};
        //std::vector<double> vtimeSRAM{512,0};
        double timeMEMSYNC{0.};
        std::vector<std::pair<__uint128_t, double>> wordsAfterSRAM{};
        //int fifo{0};
        for (auto& [word,timing]: wordsAfterBus)
        {
            // This is not generalized for any bit size
            int doubleColumn = word& 0xFF;
            int parity = (word & ((__uint128_t)1 << 8)) != 0;
            //std::cout <<"DC: " << doubleColumn << "Parity: " << parity << std::endl;
            int bus = ( parity + 1) * doubleColumn;
            //std::cout << "bus: " << bus <<std::endl;
            double prevTiming = prevGlobalTiming;
            double timeDiff = timing - prevTiming;
            timeMEMSYNC += timeDiff;
            prevGlobalTiming = timing;
            //std::cout << std::bitset<30>(word) << std::endl;
            int nRead = floor(timeMEMSYNC / SRAMFrequency);
            //std::cout << nRead << std::endl;
            if(nRead >= 1)
            {
                for(int k = 0; k < nRead; k++) 
                {
                    //int currentBus{0};
                    for (int currentBus = 0; currentBus < 512; currentBus++)
                    {
                        //std::cout <<currentBus << std::endl;
                        
                        if (memoryModule[currentBus] > 0)
                        {
                            //std::cout << currentBus << std::endl;
                            memoryModule[currentBus]--;  // drain ONE word
                            break;
                        }
                        //currentBus++;
                    }
                    //if (memoryModule[bus]) memoryModule[bus]--;
                    //std::cout << timeMEMSYNC ;
                    timeMEMSYNC -= SRAMFrequency;
                    //std::cout << " " << timeMEMSYNC << std::endl;
                }
            }

            //std::cout <<bus << std::endl;

            if (memoryModule[bus] < sramDepth)
            {
                wordsAfterSRAM.push_back({word, timing});
                memoryModule[bus]++;
            }
        }
        
        std::sort(wordsAfterSRAM.begin(), wordsAfterSRAM.end(), [](auto &a, auto &b){return a.second < b.second;});
        // IV. FIFO.
        double prevFIFOTiming{};
        double timeFIFO{};
        int fifoFill{};
        std::vector<std::pair<__uint128_t, double>> wordsAfterFIFO{};  

        for (auto& [word, timing]:wordsAfterSRAM)
        {
            double prevTiming = prevFIFOTiming ;
            double timeDiff = timing - prevTiming;
            timeFIFO += timeDiff;
            prevFIFOTiming = timing;
            //if(verbose)std::cout << "timing: " << timing << "; prevTiming: " << prevTiming << "; timeDiff: " << timeDiff << "; timeFIFO: "<< vtimeFIFO[lane] << "; div: " << vtimeFIFO[lane] / FIFOFrequency  << "; vtimeFIFO: " << vtimeFIFO[lane];
            
            int nRead = floor(timeFIFO / FIFOFrequency);
            if(nRead >= 1)
            {
                 
                for(int k = 0; k < nRead; k++) 
                {
                    if (fifoFill) fifoFill--;
                }
                timeFIFO -= nRead * FIFOFrequency;
            }

            if(verbose)std::cout << "; subt: " << timeFIFO <<" fifoFill: " << fifoFill << std::endl;
            if(fifoFill < fifoSize)
            {
                wordsAfterFIFO.push_back({word, timing});
                fifoFill++;
            }
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
        for (const auto &word: wordsAfterFIFO)
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
#include "DigitalProcessing.hh"
#include "Tracking_multiPlane.hh"
#include <bit>


struct PairHash 
{
    // Raw data sorting hash
    size_t operator()(const std::pair<int, std::pair<int,int>>& k) const {
        size_t h = std::hash<int>{}(k.first);
        h ^= std::hash<int>{}(k.second.first)  + 0x9e3779b9 + (h<<6) + (h>>2);
        h ^= std::hash<int>{}(k.second.second) + 0x9e3779b9 + (h<<6) + (h>>2);
        return h;
    }
};

void PRIOFIFOHWCProcessing_multiPlane(double inputThreshold, int runNumber, std::string saveName)
{

    auto start = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> elapsed{};
    std::chrono::time_point<std::chrono::high_resolution_clock> end, end1, end2, end3, end4, end5, end6, end7, end8;
    // Set all the analysis flags for the digital processing
    auto analysisFlags = new AnaFlags{};
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
    int sectorSize = analysisFlags->sectorSize;
    int wordSize = analysisFlags->wordSize;
    std::string prioAlgo = analysisFlags->prioAlgo;
    int pixXNum = 512;
    int pixYNum = 512;
    int groupRepetition = 32;
    int nPlanes_100 = analysisFlags->nPlanes_100;
    int nPlanes_10 = analysisFlags->nPlanes_10;
    int nPlanes_1 = analysisFlags->nPlanes_1;
    int nPlanes = nPlanes_100*nPlanes_10*nPlanes_1;

    std::string localPath = analysisFlags->localPath;
    std::string inputPath = analysisFlags->inputPath + Form("_%04d/", runNumber);
    std::string directoryPath = localPath + "Plots/";
    std::string runPath = Form("local_%04d/", runNumber);

    // Load geometry file
    DetectorConfig cfg = LoadConfig(inputPath + "flags_MP_Calo.cfg"); // Needs generalization of flag input
    auto geoMaps = LoadGeometry(analysisFlags->geoFile, cfg);


    /////////////////////////////////////////////////////////
    /////////////////////////////////////////// \(.(^).)/ Save Data
    /////////////////////////////////////////////////////////
    mkdir((inputPath + saveName).c_str(), 0777);
    TFile *outfile = new TFile((inputPath + saveName + "/Plane0ReconstructedHitsThr" + std::to_string(int(inputThreshold)) + ".root").c_str(), "RECREATE");
    outfile->cd();
    // Create a TTree
    TTree *recontructedTree = new TTree("ReconstructedHits", "Reconstructed Hits");
    // Variables for branches
    int reconstructedPixX, reconstructedPixY, nHits, planeVal;
    double reconstructedTiming;
    // Create branches
    recontructedTree->Branch("planeID", &planeVal, "planeID/I");
    recontructedTree->Branch("PixX", &reconstructedPixX, "PixX/I");
    recontructedTree->Branch("PixY", &reconstructedPixY, "PixY/I");
    recontructedTree->Branch("timing", &reconstructedTiming, "timing/D");
    recontructedTree->Branch("NHits", &nHits, "NHits/I");
    // Create a TTree
    TTree *posTree = new TTree("PositionHits", "Position Hits");
    int stripSaveX, stripSaveY;
    double timingSave;
    // Create branches
    posTree->Branch("planeID", &planeVal, "planeID/I");
    posTree->Branch("stripX",   &stripSaveX, "stripX/I");
    posTree->Branch("stripY",   &stripSaveY, "stripY/I");
    posTree->Branch("timing",  &timingSave, "timing/D");


    std::cout << "############################# FIFO Processing started for:" << std::endl;
    std::cout << inputPath << std::endl;

    // Generate threshold map
    auto thresholdMap = generateThrMap(inputThreshold, pixXNum, pixYNum, groupRepetition, relativeThresholdSmearingCol, relativeThresholdSmearingMean, directoryPath, runPath, saveName);

    // Avoid O(n^2) nested loops via extra maps     
    std::unordered_map<std::pair<int,std::pair<int,int>>, double, PairHash> enMap;
    std::unordered_map<std::pair<int,std::pair<int,int>>, std::vector<double>, PairHash> timeMap;
    std::unordered_map<std::pair<int,std::pair<int,int>>, double, PairHash> maxTimeMap;

    int eventIDHolder =0;
    std::vector<std::pair<__uint128_t,double>> fifo{};
    auto multiPlanes = CaloPreProcessing(inputThreshold, runNumber, saveName); 
    //int nPlanes = static_cast<int>(multiPlanes.size());

    std::vector<int> planes;
    cout << "Adding Planes to analysis: ";
    for (int iz = 0; iz < nPlanes_100; ++iz) {
        for (int iy = 0; iy < nPlanes_10; ++iy) {
            for (int ix = 0; ix < nPlanes_1; ++ix) {
                planes.push_back(iz*10000 + iy*100 + ix); // decoded position (works for up to 10 planes in each dimension)
                cout << iz*10000 + iy*100 + ix << ", " ;
            }
        }
    }
    std::cout << std::endl;
    //std::cout << "nPlanes: " << nPlanes << std::endl;
    for (int i = 0; i< nPlanes; i++)
    //for (const int & i : planes)
    {
        enMap.clear();
        timeMap.clear();
        maxTimeMap.clear();
        planeVal = planes[i];

        TTree* planTree = multiPlanes[i];
        if (!planTree || planTree->GetEntries() == 0) {
            std::cout << "Plane " << planes[i] << " is empty, skipping." << std::endl;
            continue;
        }

        float corrEnergy_float, timeWalkHit_float;
        int rawEventID, planeID, pixX, pixY;
        planTree->SetBranchAddress("iEvent",     &rawEventID);
        planTree->SetBranchAddress("iPlane",     &planeID);
        planTree->SetBranchAddress("PixX",       &pixX);
        planTree->SetBranchAddress("PixY",       &pixY);
        planTree->SetBranchAddress("hitTime",    &timeWalkHit_float);
        planTree->SetBranchAddress("hitEnergy",  &corrEnergy_float);
        Long64_t nRawEntries = planTree->GetEntries();

        std::cout << "nRawEntries: " << nRawEntries << std::endl;

        // Extract plane offset values from input geometry file
        Offset planeOffset = geoMaps[planes[i]];
        double dutAxisRotation = planeOffset.zrot;
        std::cout << "X: " << planeOffset.x << "; Y: " << planeOffset.y << "; Z: " << planeOffset.z 
                  << "; xROT: " << planeOffset.xrot  << "; yROT: " << planeOffset.yrot  << "; zROT: " << planeOffset.zrot << std::endl;

        
        ////////////////////////////////////////////////////////////
        /////////////////////////////////////////// .1. Raw IO calls
        ////////////////////////////////////////////////////////////
        for (Long64_t j = 0; j < nRawEntries; j++)
        {
            planTree->GetEntry(j);

            double corrEnergy  = static_cast<double>(corrEnergy_float);
            double timeWalkHit = static_cast<double>(timeWalkHit_float);

            enMap[{rawEventID, {pixX, pixY}}]  += corrEnergy;
            timeMap[{rawEventID, {pixX, pixY}}].push_back(timeWalkHit);
            auto& maxRef = maxTimeMap[{rawEventID, {pixX, pixY}}];
            if (timeWalkHit > maxRef) maxRef = timeWalkHit;
        }

        end1 = std::chrono::high_resolution_clock::now();
        elapsed = end1 - start;
        std::cout << ".1. IO time: " << elapsed.count() << std::endl;

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

        //////////////////////////////////////////////////////////
        /////////////////////////////////////////// .2. Energy sum
        //////////////////////////////////////////////////////////
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
            //std::cout << "Event ID: " << eventID << "; pixX: " << pixX << ";pixY: " << pixY << "; corrEnergy: " << cenergy << "; timewalk: "<< timing << std::endl;
            timing += *std::max_element(it->second.begin(), it->second.end()) + pixY * 0.0125;  // 0.390625 for miniMALTA3?

            //timing += maxTimeMap.find({eventID, {pixX, pixY}})->second + pixY * 0.0125;
            sortedTimings.emplace_back(entry.first,timing);
        }
        std::sort(sortedTimings.begin(), sortedTimings.end(),
                [](auto &a, auto &b){ return a.second < b.second; });

        // Now I have events sorted by the global timing. For most applications this simply sorts the hits in a cluster and retain eventID
        // However, for very high rate beams the eventIDs could not be retained anymore.

        double t0 = sortedTimings.begin()->second;

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

        end2 = std::chrono::high_resolution_clock::now();
        elapsed = end2 - end1;
        std::cout << ".2. Energy sum: " << elapsed.count() << std::endl;

        double prevTiming{};
        int count = 0;

        ///////////////////////////////////////////////////////////
        /////////////////////////////////////////// .3. Word encode
        ///////////////////////////////////////////////////////////
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

        end3 = std::chrono::high_resolution_clock::now();
        elapsed = end3 - end2;
        std::cout << ".3. Word encode: " << elapsed.count() << std::endl;
        
        // I. Group level merging of hits.
        std::map<std::pair<int,int>, std::vector<std::pair<__uint128_t, double>>> groupWords;
        std::unordered_map<int, std::vector<std::pair<__uint128_t, double>>> firstBusWords;

        ///////////////////////////////////////////////////////////
        /////////////////////////////////////////// .4. Group merge
        ///////////////////////////////////////////////////////////
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
        }
        end4 = std::chrono::high_resolution_clock::now();
        elapsed = end4 - end3;
        std::cout << ".4. Group merge: " << elapsed.count() << std::endl;

        //wordsAfterGroup = words;
        // II. Bus level merging of hits


        /////////////////////////////////////////////////////////
        /////////////////////////////////////////// .5. Bus merge
        /////////////////////////////////////////////////////////
        std::unordered_map<int, std::vector<std::pair<__uint128_t, double>>> busWords;
        for (auto&[word, timing] : wordsAfterGroup)
        {
            //std::cout << std::bitset<30>(word) << std::endl;
            // This is not generalized for any bit size
            int doubleColumn = word& 0xFF;
            int parity = (word & ((__uint128_t)1 << 8)) != 0;
            //std::cout <<"DC: " << doubleColumn << "Parity: " << parity << std::endl;
            int bus = 2 * doubleColumn + parity;
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
        }
        std::sort(wordsAfterBus.begin(), wordsAfterBus.end(), [](auto &a, auto &b){return a.second < b.second;});
        
        end5 = std::chrono::high_resolution_clock::now();
        elapsed = end5 - end4;
        std::cout << ".5. Bus merge: " << elapsed.count() << std::endl;


        // III. SYNC Memory. ROUND ROBIN Drain

        ////////////////////////////////////////////////////////
        /////////////////////////////////////////// .6. SYNC MEM
        ////////////////////////////////////////////////////////
        std::vector<int> memoryModule(512,0); 
        std::vector<std::vector<std::pair<__uint128_t, double>>> memoryWordStore(512); 
        std::queue<int> occupiedSectors{};
        double prevGlobalTiming{0};
        double timeMEMSYNC{0.};
        std::vector<std::pair<__uint128_t, double>> wordsAfterSRAM{};
        // nSectors needs to handle non even division of the matrix into sectors.
        int missedHitCount{};
        int nSectors = (512 + (2 * sectorSize) - 1) / (2 * sectorSize);

        std::vector<int> sectorOccupancy(nSectors);
        for (auto& [word,timing]: wordsAfterBus)
        {
            // This is not generalized for any bit size
            //std::cout << "Word: " << std::bitset<30>(word) << std::endl;
            int doubleColumn = word& 0xFF;
            int parity = (word & ((__uint128_t)1 << 8)) != 0;
            //std::cout <<"DC: " << doubleColumn << "Parity: " << parity << std::endl;
            int bus = 2 * doubleColumn + parity;
            //std::cout << "bus: " << bus <<std::endl;
            double prevTiming = prevGlobalTiming;
            double timeDiff = timing - prevTiming;
            timeMEMSYNC += timeDiff;
            prevGlobalTiming = timing;
            //std::cout << std::bitset<30>(word) << std::endl;
            int nRead = floor(timeMEMSYNC / SRAMFrequency);

            //std::cout << "prev: " << prevTiming << "; timing: " << timing << "; timeDiff: " << timeDiff << "; timeMEMSYNC: " << timeMEMSYNC << "; SRAMF: " << SRAMFrequency << "; nRead: " << nRead << std::endl; 
            //std::cout << "nRead: " << nRead << std::endl;
            if(verbose) std::cout << nRead << std::endl;
            if(nRead >= 1)
            {
                for (int k = 0; k < nRead; k++) 
                {                
                    int sector{};
                    if (!occupiedSectors.empty())
                    {
                        __uint128_t HWCWord{};

                        if (prioAlgo == "RoundRobin") sector = occupiedSectors.front();
                        occupiedSectors.pop();
                        // Determine the first sector of buses to read based on the sector with the most non empty memory entries

                        
                        std::vector<int> filledBuses (nSectors, 0);
                        if (prioAlgo != "RoundRobin")
                        {
                            for (int i = 0; i< 512; i++)
                            {
                                int currentSector = static_cast<int>(i / (2*sectorSize));
                                // IMPORTANT: Here this implementation achieves prio readout of most filled sectors but not most filled buses. 
                                // Could be unphysical and maybe also suboptimal for high load bus occupancy
                                if (prioAlgo == "MostFilled") if (memoryModule[i] > 0) filledBuses[currentSector] ++;
                                if (prioAlgo == "MostFull") if (memoryModule[i] > 0) filledBuses[currentSector] += memoryModule[i];
                                //if (memoryModule[i] > 0) std::cout << "i: " << i << "; sector: " << sector << "; memMod: " << memoryModule[i] << "; sector Fill: " << filledBuses[sector] << std::endl;
                            }
                            //std::cout << "NREAD: " << nRead << std::endl;
                            //std::cout << std::endl;
                            // I found the sector to read out at this read cycle
                            auto it = std::max_element(filledBuses.begin(), filledBuses.end());
                            sector = std::distance(filledBuses.begin(), it);
                        }
                        // Improvise a timing value to remain consistent with the structure of the code.
                        // This info should be normally ditched in this RO scheme
                        double improvTiming{};
                        // Read all busses in the sector
                        // This for loop implementation addresses the edge case when the nPix % sectorSize!=0
                        int start = sector * 2 * sectorSize;
                        int end   = std::min(start + 2 * sectorSize, 512);
                        //std::cout << "Sector Occ: " << sectorOccupancy[sector] << std::endl;
                        int numValids{};
                        std::vector<uint32_t> vreducedWords{};
                        for (int i = start; i < end; i++)
                        {
                            //std::cout << "i: " << i << "; B memoryModule[i]: " << memoryModule[i] << "; B memoryWordStore[i]: " << memoryWordStore[i].size();
                            // Drain each bus
                            // append the drained words to a single word for the next processing step
                            if (memoryModule[i] > 0 && !memoryWordStore[i].empty()) 
                            {
                                //std::cout << "HWCWord: " << std::bitset<4>(memoryWordStore[i].front().first)  << "; Try: " << std::bitset<10>(memoryWordStore[i].front().first ) << "; Cur: " << std::bitset<4>(memoryWordStore[i].front().first & 0xF) << std::endl;
                                uint32_t fourBit  = memoryWordStore[i].front().first & 0xF;
                                vreducedWords.push_back(fourBit);
                                improvTiming = std::max(improvTiming, memoryWordStore[i].front().second);
                                //std::cout << "front: " << std::bitset<4>(memoryWordStore[i].front().first) << "; begin: " << std::bitset<4>((*memoryWordStore[i].begin()).first)<< std::endl;
                                // Erase entry after appending it to the Word
                                memoryWordStore[i].erase(memoryWordStore[i].begin());
                                memoryModule[i] --;
                                sectorOccupancy[sector]--;
                                numValids++;
                            }
                            // If one bus is empty we append an empty bit word
                            else vreducedWords.push_back(0);
                            
                            //std::cout << "; A memoryModule[i]: " << memoryModule[i] << "; A memoryWordStore[i]: " << memoryWordStore[i].size() << std::endl;
                        }
                        //std::cout << std::bitset<88>(HWCWord) << std::endl;
                        //std::cout << " A Sector Occ: " << sectorOccupancy[sector] << std::endl;
                        // Reinsert sector in queue if non empty
                        if (sectorOccupancy[sector] > 0)
                        {
                            occupiedSectors.push(sector);
                        }

                        //std::cout << "---------------------------------------" << std::endl;
                        auto vcompressedWords = CompressWords(vreducedWords, wordSize);
                        for (const auto& compWord : vcompressedWords)
                        {
                            // Add all the words from the vector into a single word to mimic the actual HW. the mask is to ensure the word size
                            // limit is respected
                            HWCWord = (HWCWord << wordSize) | (compWord & ((1u << wordSize) - 1));

                        }
                        /*
                        std::cout << "Reduced Words:";
                        
                        for (const auto& el: vreducedWords)
                        {
                            std::cout << std::bitset<5>(el) << " ; ";
                        }
                        std::cout << std::endl;
                        for (const auto& el: vcompressedWords)
                        {
                            std::cout << std::bitset<5>(el) << " ; ";
                        }
                        std::cout << std::endl;                            
                        std::cout << "---------------------------------------" << std::endl;
                        */
                        //std::cout <<"HWCWord: " << std::bitset<64>(HWCWord) << std::endl;
                        // Lastly append sector address
                        uint8_t sixBit = sector & 0x3F;
                        __uint128_t fullHWCWord = (HWCWord << 6) | sixBit;
                        //if (numValids == 22)std::cout << std::bitset<128> (fullHWCWord) << std::endl;
                        //std::cout << s << std::endl;
                        // Now I have the multiplexed word with an arbitrary timing for the next step. I only push non zero words that escape the previous checks
                        //std::cout << std::bitset<100>(HWCWord) << std::endl;
                        //std::cout << "sector: " << sector << std::endl;
                        if (HWCWord != 0) 
                        {
                            //std::cout << "PUSHED!" << std::endl;
                            wordsAfterSRAM.push_back({fullHWCWord, improvTiming});
                        }
                    
                    }
                    // Sync back the time flow
                    timeMEMSYNC -= SRAMFrequency;

                    if (verbose) std::cout << ". Time remaining: " << timeMEMSYNC << " SRAMF: " << SRAMFrequency << std::endl;
                }
            }
            //std::cout <<bus << std::endl;
            if (memoryModule[bus] < sramDepth)
            {
                int sector = bus / (2 * sectorSize);
                if (sectorOccupancy[sector] == 0)
                    occupiedSectors.push(sector);  // track it only when it goes 0 -> 1 This probably doesnt work here where we multiplex busses
                //wordsAfterSRAM.push_back({word, timing});
                int pixAddr = (word >> 14) & 0xFFFF;
                int HWCWord = __builtin_popcount(pixAddr);
                //std::cout << "word: " << std::bitset<30>(word) << "; pixAddr: " << std::bitset<16>(pixAddr) << "; Count: " << std::bitset<4>(HWCWord) << std::endl;
                memoryWordStore[bus].push_back({HWCWord, timing});
                memoryModule[bus]++;
                sectorOccupancy[sector]++;
                //std::cout << "Bus: " << bus << "; memModule[bus]: " << memoryModule[bus] << std::endl;
                if(verbose) std::cout << "Bus index " << bus << " populated. Population: " << memoryModule[bus] << std::endl; 
            }
            else
            {
                missedHitCount++;
                //std::cout << "Missed hit!" << std::endl;
            }
        }
        std::cout << "Missed HIt count: " << missedHitCount << std::endl;
        std::sort(wordsAfterSRAM.begin(), wordsAfterSRAM.end(), [](auto &a, auto &b){return a.second < b.second;});


        end6 = std::chrono::high_resolution_clock::now();
        elapsed = end6 - end5;
        std::cout << ".6. MEM SYNC: " << elapsed.count() << std::endl;

        // IV. FIFO.

        /////////////////////////////////////////////////////////
        /////////////////////////////////////////// .7. PRIO FIFO
        /////////////////////////////////////////////////////////
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


        end7 = std::chrono::high_resolution_clock::now();
        elapsed = end7 - end6;
        std::cout << ".7. PRIO FIFO: " << elapsed.count() << std::endl;


        for (const auto &word: wordsAfterFIFO)
        {
            //std::vector< std::pair<std::pair<int,int>, int> > pixelPositions = decodedDigitalWord(word.first, groupSize, groupSizeX, groupSizeY, groupLeng, parityLeng, dColLeng);
            nHits = 0;
            reconstructedTiming = word.second;
            // Sector position
            if (dutAxisRotation != 90)
            {
                reconstructedPixX =  (word.first & 0x3F) * sectorSize;
                reconstructedPixY = -1; // Only strip X info
            }
            else
            {
                reconstructedPixX =  -1;
                reconstructedPixY = (word.first & 0x3F) * sectorSize; // Only strip Y info
            }
            __uint128_t x = word.first >> 6; 
            int wrongnHits = __builtin_popcountll(x);
            //std::cout << "Word: " << std::bitset<88>(x) << std::endl;
            uint32_t mask = (1u << wordSize) - 1;
            int count = 0;
            while (x) 
            {
                //std::cout <<"x: " << std::bitset<60>(x) << std::endl;
                nHits += (x & mask);
                x >>= wordSize;

                if (dutAxisRotation != 90)
                {
                    stripSaveX = reconstructedPixX + count;
                    stripSaveY = -1;
                }
                else
                {
                    stripSaveX = -1;
                    stripSaveY = reconstructedPixY + count;
                }
                timingSave = reconstructedTiming;
                count++;

                //std::cout << "stripSaveX: " << stripSaveX << " ; stripSaveY: " << stripSaveY << std::endl;
                posTree->Fill();
            }
            //std::cout << "nHits: " << nHits << std::endl;
            //std::cout << "wrongnHits: " << wrongnHits << std::endl;

            recontructedTree->Fill();
            
        
        }
        std::cout << "Finishing up analyzing Plane" << i << std::endl;
    }
    outfile->cd();
    recontructedTree->Write();
    posTree->Write();
    outfile->Close();

    end8 = std::chrono::high_resolution_clock::now();
    elapsed = end8 - end7;
    std::cout << ".8. Save Data: " << elapsed.count() << std::endl;

    end = std::chrono::high_resolution_clock::now();
    elapsed = end - start;
    std::cout << "############################# FIFO Processing stopped after " << elapsed.count() << "ms" << std::endl;
}
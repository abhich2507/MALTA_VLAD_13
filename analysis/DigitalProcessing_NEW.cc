#include "DigitalProcessing.hh"



// TODO: We are in dire need of value checks for all user inputs.

void DigitalProcessing_NEW(double inputThreshold, int runNumber, std::string saveName, bool proteusFlag)
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

    //std::cout << groupSize << " ; " << groupSizeX << " ; " << groupSizeY << " ; " << groupLeng << std::endl;
    
    // get local path
    if (proteusFlag) saveName = ""; // Threshold 2000 is the special case of saving all planes
    ////////// Function can be used for custom analysis paths
    //std::string localPath = getVarFromConfig();
    //////////////////////////////////////////////////////////
    std::string localPath = "./";
    std::string inputPath = localPath +  Form("Results/local_%04d/", runNumber);
    std::string directoryPath = localPath + "Plots/";
    std::string runPath = Form("local_%04d/", runNumber);

    std::cout << "############################# Digital Processing started for:" << std::endl;
    std::cout << inputPath << std::endl;
    // Extract raw data
    TChain *chainPixel = new TChain("RawPixelHits");
    for (int t = 0; t <= numThreads - 1; ++t) 
    {
        chainPixel->Add(Form("%soutput0_t%d.root", inputPath.c_str() , t));
    }
    double corrEnergy, timeWalkHit;
    int rawEventID, planeID, iHit, pixX, pixY;
    chainPixel->SetBranchAddress("iEvent", &rawEventID);
    chainPixel->SetBranchAddress("iPlane", &planeID);
    chainPixel->SetBranchAddress("iHit", &iHit);
    chainPixel->SetBranchAddress("PixX", &pixX);
    chainPixel->SetBranchAddress("PixY", &pixY);
    chainPixel->SetBranchAddress("hitTime", &timeWalkHit); // TODO change var name
    chainPixel->SetBranchAddress("hitEnergy", &corrEnergy); // TODO change var name
    Long64_t nRawEntries = chainPixel->GetEntries();

    // Avoid O(n^2) nested loops via extra map 
    std::map<std::pair<int,std::pair<int, int>>, double> enMap; 
    std::map<std::pair<int,std::pair<int,int>>, std::vector<double>> timeMap;
    int eventIDHolder =0;
    int nPlanes = 1;
    // Save a single threshold value for the tracking planes. Value chosen: 2000. Should be standard for all runs
    if (inputThreshold == 2000) nPlanes = 7;

    // charge loss coefficient
    double chLoss = analysisFlags->chLoss;
    corrEnergy *=chLoss;

    // Threshold smearing. Philosophy: randomly assign a fixed smearing per pixel and keep it. 
    //This should simulate the fabrication differences leading to pixel threshold changes.
    double relativeThresholdSmearingMean = analysisFlags->meanSmearing;
    double relativeThresholdSmearingCol = analysisFlags->colSmearing;
    // These values are set by the pixel size and detector size in the GEANT4 simulation config. 
    // If generalization is desired these values should then be inherited from there
    int pixXNum = 512;
    int pixYNum = 512;
    int groupRepetition = 32;

    std::map<std::pair<int,int>, double> thresholdMap;
    // Save the threshold 
    TH1D *h1DThreshold =  new TH1D("h1DUTThreshold", "h1DThreshold", 100, inputThreshold - inputThreshold / 2,inputThreshold + inputThreshold / 2);
    TH2D *h2DThreshold    = new TH2D("h2DUTThreshold", "h2DUTThreshold", 512, 0, 512, 512, 0, 512);
    TH2D *h2MissMerged    = new TH2D("h2MissMerged", "h2MissMerged", 512, 0, 512, 512, 0, 512);

    // - threshold distrubution should be the sum of N distrubutions, one every
    // 32 columns, the width smaller than 5% each time, but the mean changes 3-5%.

    // This is an example data input for data sim threshold dispersion validation
    // Thr 967
    //std::vector<double> vThrMeanData = {959.2, 1004.8, 986.9, 1003.7, 1030.1, 1037.2, 1002.7, 983.7, 938.2, 952.7, 976.5, 943.4, 960.7, 930.8, 884.3, 875.2};
    // Thr 200
    std::vector<double> vThrMeanData = {228.594,220.652,223.642,236.398,230.506,242.855,235.05,228.462,233.891,226.979,218.171,214.329,223.578,207.099,209.558,210.827};
    for (int i = 0; i< pixXNum/groupRepetition; i++)
    {
        static std::mt19937 gen(std::random_device{}());
        std::normal_distribution<> dist(1.0, relativeThresholdSmearingMean);
        double thresholdMean = inputThreshold * dist(gen);
        //std::cout << thresholdMean << std::endl;
        //double thresholdMean = vThrMeanData[i];
        for (int x = 0; x < pixXNum; x++)
        {
            for (int y = 0; y < pixYNum; y++)
            {
                if (x / groupRepetition == i)
                {
                    static std::mt19937 gen(std::random_device{}());
                    std::normal_distribution<> dist(1.0, relativeThresholdSmearingCol);
                    double thresholdCol = thresholdMean * dist(gen);
                    thresholdMap[{x,y}] = thresholdCol;
                    h1DThreshold->Fill(thresholdCol);
                    h2DThreshold->Fill(x, y, thresholdCol);
                }
            }
        }
    }
    savePlot(directoryPath, runPath, inputThreshold, saveName, h1DThreshold, "h1DThreshold");
    savePlot(directoryPath, runPath, inputThreshold, saveName, h2DThreshold, "h2DThreshold");

    // Iterate over each plane if needed
    for (int i = 0; i< nPlanes; i++)
    {
        // Sum up all hits in an event per pixel. This assumes all energy is collected instantly and 
        // timing cuts will be made only based on time walk and particle travel time.
        for (Long64_t j = 0; j < nRawEntries; j++)
        {
            chainPixel->GetEntry(j);
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
        for (const auto& entry : sortedTimings) 
        {
            int eventID = entry.first.first;
            int pixX   = entry.first.second.first;
            int pixY   = entry.first.second.second;

            auto itThr = thresholdMap.find({pixX, pixY});
            if (itThr == thresholdMap.end()) 
            {
                std::cout << "Missing threshold for pixX=" << pixX
                        << " pixY=" << pixY << std::endl;
                continue;
            }
            double threshold = itThr->second;

            double timing = entry.second;
            auto it = enMap.find({eventID, {pixX, pixY}});
            double cenergy = it->second;
        
            if (cenergy < threshold) continue;
            
            if (verbose)
            {
                std::cout << "Event ID: " << eventID << "; pixX: " << pixX << ";pixY: " << pixY << "; corrEnergy: " << cenergy << "; Timing: " << std::setprecision(10) <<  timing << std::endl;
            }
            
            __uint128_t word = encodeWord(pixX, pixY, groupSizeX, groupSizeY, groupLeng, parityLeng, dColLeng, verbose);

            //Now I digitized all my words. Next step is merging them based on timing
            if (timing >= t0 && timing < t0 + wordSpacing)
            {
                if(verbose)std::cout << "Merging into current word bucket with timing." << timing << std::endl;
                merger.push_back(std::make_pair(word, timing));
            }
            else
            {
                t0 = timing;
                digitizedWords.push_back(merger);
                if(verbose)std::cout << "Word Size = " << merger.size() << std::endl; 
                merger.clear();
                merger.push_back(std::make_pair(word, timing)); 
            }
        }
        // Also push the very last merged word.
        if (!merger.empty()) digitizedWords.push_back(merger);
        // Now I have my words in their correct time buckets. Next is the merging logic.

        std::vector<std::pair<__uint128_t, double>> mergedWords;
        for (int i =0; i< digitizedWords.size(); i++)
        {
            // This was not initialized before leading to weird first word
            __uint128_t mergedWord = 0; 
            std::vector<double> leadingTime;
            int aux = 0;
            for (auto [word, timing] : digitizedWords[i])
            {
                if(verbose)std::cout << "Word: " << std::bitset<30>(word) << std::endl;
                mergedWord |= word;
                leadingTime.push_back(timing);
                aux++;

                // A merging has taken place. Debug information on merging.
                if (aux == 2)
                {   
                    // Check if the 14 least significant bits are the same. If not than a mismerging happened
                    if ( (mergedWord & 0x3FFF) != (word & 0x3FFF) )
                    {
                        std::vector< std::pair<std::pair<int,int>, int> > pixelPositions = decodedDigitalWord(mergedWord, groupSize, groupSizeX, groupSizeY, groupLeng, parityLeng, dColLeng);
                        for (const auto& pos : pixelPositions) 
                        {
                            int reconstructedPixX = pos.first.first;
                            int reconstructedPixY = pos.first.second;
                            h2MissMerged->Fill(reconstructedPixX, reconstructedPixY, 1);
                        }
                    }
                }
            }

            
            // Save the merged word and the fastest hit time
            if (digitizedWords[i].size() == 0) continue; // Skip empty vectors
            mergedWords.push_back({mergedWord, *std::max_element(leadingTime.begin(), leadingTime.end())});
            if (verbose) std::cout << "##############" << std::endl << "Merged word: " << std::bitset<30>(mergedWord) << std::endl;
            // TODO: Timing should be given via a clock. Find from Carlos the frequency.
            // Reset fot next iteration
            mergedWord = 0;
            leadingTime.clear();
        }
        savePlot(directoryPath, runPath, inputThreshold, saveName, h2MissMerged, "h2MissMerged");

        // Now I have merged words. Next step is decoding them back to position and time
        // However, first I need the infrastructure to save them in a root tree.
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

        for (auto [word, timing] : mergedWords) 
        {
            std::vector< std::pair<std::pair<int,int>, int> > pixelPositions = decodedDigitalWord(word, groupSize, groupSizeX, groupSizeY, groupLeng, parityLeng, dColLeng);

            for (const auto& pos : pixelPositions) 
            {
                if(verbose) std::cout << "Decoded pixel position: (" << pos.first.first << ", " << pos.first.second << ")" << "; Timing: "<< std::setprecision(8) << timing << std::endl;
                reconstructedTiming = timing; // + 100 adds a constant 100 ns delay in order to replicate the TB data 
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
    std::cout << "############################# Digital Processing stopped after " << elapsed.count() << "ms" << std::endl;
}

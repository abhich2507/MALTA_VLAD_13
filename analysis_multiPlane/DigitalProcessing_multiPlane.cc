#include "DigitalProcessing.hh"



// TODO: We are in dire need of value checks for all user inputs.

void DigitalProcessing_multiPlane(double inputThreshold, int runNumber, std::string saveName, bool proteusFlag)
{
    auto start = std::chrono::high_resolution_clock::now();
    TH1::AddDirectory(kFALSE);
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
    std::string localPath = analysisFlags->localPath;
    std::string inputPath = analysisFlags->inputPath+Form("_%04d/", runNumber);
    std::string directoryPath = localPath + "Plots/";
    std::string runPath = Form("local_%04d/", runNumber);

    std::cout << "############################# Digital Processing MultiPlane started for:" << std::endl;
    std::cout << inputPath << std::endl;    

    // Avoid O(n^2) nested loops via extra map 
    std::map<std::pair<int,std::pair<int, int>>, double> enMap; 
    std::map<std::pair<int,std::pair<int,int>>, std::vector<double>> timeMap;
    int eventIDHolder =0;
    int nPlanes_100 = analysisFlags->nPlanes_100;
    int nPlanes_10 = analysisFlags->nPlanes_10;
    int nPlanes_1 = analysisFlags->nPlanes_1;
    int nPlanes = nPlanes_100*nPlanes_10*nPlanes_1;
    
    //int nPlanes = analysisFlags->nPlanes;
    // Threshold smearing. Philosophy: randomly assign a fixed smearing per pixel and keep it. 
    //This should simulate the fabrication differences leading to pixel threshold changes.
    double relativeThresholdSmearingMean = analysisFlags->meanSmearing;
    double relativeThresholdSmearingCol = analysisFlags->colSmearing;
    // These values are set by the pixel size and detector size in the GEANT4 simulation config. 
    // If generalization is desired these values should then be inherited from there
    int pixXNum = 512;
    int pixYNum = 512;
    int groupRepetition = 32;

    //TH2D *h2MissMerged    = new TH2D("h2MissMerged", "h2MissMerged", 512, 0, 512, 512, 0, 512);
    // Generate threshold map
    auto thresholdMap = generateThrMap(inputThreshold, pixXNum, pixYNum, groupRepetition, relativeThresholdSmearingCol, relativeThresholdSmearingMean, directoryPath, runPath, saveName);
    
    //first I need the infrastructure to save them in a root tree.
    gROOT->cd();
    auto multiPlanes = CaloPreProcessing(inputThreshold, runNumber, saveName); 
    mkdir((inputPath + saveName).c_str(), 0777);   
    TFile *outfile = new TFile((inputPath + saveName + "/ReconstructedHitsThr" + std::to_string(int(inputThreshold)) + ".root").c_str(), "RECREATE");
    outfile->cd();
    // Create a TTree
    TTree *recontructedTree = new TTree("ReconstructedHits", "Reconstructed Hits");
    recontructedTree->SetDirectory(nullptr);
    // Variables for branches
    int reconstructedPixX, reconstructedPixY, nHits;
    double reconstructedTiming;

    float corrEnergy_float, timeWalkHit_float;
    int rawEventID, planeID, pixX, pixY, nRawEntries;
    // Create branches
    recontructedTree->Branch("planeID", &planeID, "planeID/I");
    recontructedTree->Branch("PixX", &reconstructedPixX, "PixX/I");
    recontructedTree->Branch("PixY", &reconstructedPixY, "PixY/I");
    recontructedTree->Branch("timing", &reconstructedTiming, "timing/D");
    recontructedTree->Branch("NHits", &nHits, "NHits/I");

    // define planeIDs to analyze:

    std::vector<int> planes;
    cout << "Adding Planes to analysis: ";
    for (int iz = 0; iz < nPlanes_100; ++iz) {
        for (int iy = 0; iy < nPlanes_10; ++iy) {
            for (int ix = 0; ix < nPlanes_1; ++ix) {
                planes.push_back(iz*100 + iy*10 + ix); // decoded position (works for up to 10 planes in each dimension)
                cout << iz*100 + iy*10 + ix << ", " ;
            }
        }
    }
    cout << endl;

    // Iterate over each plane if needed
    for (size_t p = 0; p < planes.size(); p++)
    {
        enMap.clear();
        timeMap.clear();
        
        TTree* plane = multiPlanes[p];
        plane->SetBranchAddress("iEvent", &rawEventID);
        plane->SetBranchAddress("iPlane", &planeID);
        plane->SetBranchAddress("PixX", &pixX);
        plane->SetBranchAddress("PixY", &pixY);
        plane->SetBranchAddress("hitTime", &timeWalkHit_float);
        plane->SetBranchAddress("hitEnergy", &corrEnergy_float);
        nRawEntries = plane->GetEntries();

        planeID = planes[p]; // get planeID

        if (nRawEntries ==0) {
            std::cout << "Plane " << planeID << " has no entries in TTree. Skip it!" << std::endl;
            continue; // skip empty planes because it would cause errors below.
        }

        // Sum up all hits in an event per pixel. This assumes all energy is collected instantly and 
        // timing cuts will be made only based on time walk and particle travel time.
        for (Long64_t j = 0; j < nRawEntries; j++)
        {
            plane->GetEntry(j);
            double corrEnergy = static_cast<double>(corrEnergy_float);
            double timeWalkHit = static_cast<double>(timeWalkHit_float);
            //std::cout <<  "Float: " << corrEnergy_float << "; Double: " << corrEnergy << std::endl;
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
            double timing    = GetTimingOffset(cenergy, threshold, T, Tdiv, TrefThr, x0, n, t0); // todo add timing 

            if(pixX < 0 || pixX > 511 || pixY < 0 || pixY > 511) std::cout << "Warning! Out of bounds pixels! pixX: " << pixX << "; pixY: " << pixY << std::endl; 
        
            auto it = timeMap.find({eventID, {pixX, pixY}});
            if(it == timeMap.end()) continue;
            // Row correction also of 7ns/ 512 rows + global GEANT4 timestamp
            //if(verbose) std::cout << "Event ID: " << eventID << "; pixX: " << pixX << ";pixY: " << pixY << "; corrEnergy: " << cenergy << "; timewalk: "<< timing << std::endl;
            timing += *std::max_element(it->second.begin(), it->second.end()) + pixY * 0.0125; 

            sortedTimings.emplace_back(entry.first,timing);
        }
        std::sort(sortedTimings.begin(), sortedTimings.end(),
                [](auto &a, auto &b){ return a.second < b.second; });

        // Now I have events sorted by the global timing. For most applications this simply sorts the hits in a cluster and retain eventID
        // However, for very high rate beams the eventIDs could not be retained anymore.

        if (sortedTimings.empty()) {
            std::cerr << "Plane " << planes[p] << " has no valid hits after sorting. This should not happen." << std::endl;
            //continue;
        }
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
                            //h2MissMerged->Fill(reconstructedPixX, reconstructedPixY, 1);
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
        //savePlot(directoryPath, runPath, inputThreshold, saveName, h2MissMerged, "h2MissMerged");

        // Now I have merged words. Next step is decoding them back to position and time

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
        std::cout << "Finishing up analyzing Plane" << planeID << std::endl;
    }
    outfile->cd();
    recontructedTree->Write("", TObject::kOverwrite);
    outfile->Close();

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> elapsed = end - start;
    std::cout << "############################# Digital Processing MultiPlane stopped after " << elapsed.count() << "ms" << std::endl;
}



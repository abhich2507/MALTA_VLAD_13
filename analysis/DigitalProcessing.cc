#include "DigitalProcessing.hh"


double GetTimingOffset(double amplitude, double threshold) 
{
    
    if (amplitude < threshold) // if less than than threshold 
    {
        //std::cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << std::endl;
        return 200.; // set to 200 ns delay if less than threshold (function diverges at threshold)
    }
    
    return 390.0 / pow((amplitude * 150./threshold) - 149.8, 0.65);
}

std::vector<int> getSetBitPositions(uint16_t maltaPixel)
// Checks for all flipped bits in a 16-bit word and returns their positions in a vector
{
    std::vector<int> positions;
    for (int i = 0; i < 16; i++) {
        if (maltaPixel & (1 << i)) {
            positions.push_back(i);
        }
    }
    return positions;
}

std::vector< std::pair<std::pair<int,int>, int> > decodedDigitalWord(UInt_t word)
{

    bool debug = false;

    UInt_t maltaPixel   = (word >> (5 + 1 + 8)) & 0xFFFF; // top 16 bits
    UInt_t maltaGroup   = (word >> (1 + 8))   & 0x1F;     // next 5 bits
    UInt_t maltaParity  = (word >> 8)         & 0x1;      // 1 bit
    UInt_t maltaDColumn =  word               & 0xFF;     // last 8 bits
    std::vector <std::pair<std::pair<int,int>, int> > pixelPositions;
    std::vector<int> hitInGroup = getSetBitPositions(maltaPixel);
    
    if (true) //(timing >=5.94e+09 && timing < 5.95e+09)// (timing >= 506620004.8 && timing <=506620005.7)
    {
        if(debug)
        {
            std::cout << "-------------------------------" << std::endl;
            std::cout << "Input word: " << std::bitset<32>(word) << std::endl;
            std::cout << "Decoded Pixel word: " << std::bitset<16>(maltaPixel) << std::endl;
            std::cout << "Decoded Group: " << std::bitset<5>(maltaGroup) << std::endl; 
            std::cout << "Decoded Parity: " << std::bitset<1>(maltaParity) << std::endl;
            std::cout << "Decoded DColumn: " << std::bitset<8>(maltaDColumn) << std::endl;
        }
        
        int nHits = 0;
        for (int hit :hitInGroup)
        {
            nHits ++;
            int pixX = maltaDColumn *2 + hit /8;
            int pixY = maltaGroup *16 + 8 * maltaParity + hit %8;
            pixelPositions.push_back(std::make_pair(std::make_pair(pixX, pixY), nHits));
            if (debug)    
            {
                //std::cout << "Hit bit position: " << hit << std::endl;
                std::cout << "Decoded pixel position: (" << pixX << ", " << pixY << ")" << std::endl;
            }
        }
        if (debug) std::cout << "-------------------------------" << std::endl;
    }
    
    return pixelPositions;

}




void DigitalProcessing(double inputThreshold, int runNumber, std::string saveName, bool proteusFlag)
{
    auto start = std::chrono::high_resolution_clock::now();

    bool debug = false;
    
    // get local path
    if (proteusFlag) saveName = ""; // Threshold 2000 is the special case of saving all planes
    std::string localPath = getVarFromConfig();//("../config.sh", "LOCAL_PATH");
    std::string inputPath = localPath +  Form("Results/local_%04d/", runNumber);
    std::string directoryPath = localPath + "Plots/";
    std::string runPath = Form("local_%04d/", runNumber);

    std::cout << "############################# Digital Processing started for:" << std::endl;
    std::cout << inputPath << std::endl;
    // Extract raw data
    TChain *chainPixel = new TChain("RawPixelHits");
    for (int t = 0; t <= 5; ++t) {
        chainPixel->Add(Form("%soutput0_t%d.root", inputPath.c_str() , t));
        //chainPixel->Add(Form("%soutput0_t0.root", inputPath.c_str()));
    }
    double corrEnergy, timeWalkHit;
    int rawEventID, planeID, iHit, pixX, pixY;
    chainPixel->SetBranchAddress("iEvent", &rawEventID);
    chainPixel->SetBranchAddress("iPlane", &planeID);
    chainPixel->SetBranchAddress("iHit", &iHit);
    chainPixel->SetBranchAddress("PixX", &pixX);
    chainPixel->SetBranchAddress("PixY", &pixY);
    chainPixel->SetBranchAddress("Energy", &corrEnergy);
    chainPixel->SetBranchAddress("timeWalkHit", &timeWalkHit);
    Long64_t nRawEntries = chainPixel->GetEntries();

    // Avoid O(n^2) nested loops via extra map 
    std::map<std::pair<int,std::pair<int, int>>, double> enMap; 
    std::map<std::pair<int,std::pair<int,int>>, std::vector<double>> timeMap;
    int eventIDHolder =0;
    int nPlanes = 1;
    if (inputThreshold == 2000) nPlanes = 7;

    // Hardcoded charge loss coefficient
    double chLoss = 1;
    corrEnergy *=chLoss;


    // Threshold smearing. Philosophy: randomly assign a fixed smearing per pixel and keep it. 
    //This should simulate the fabrication differences leading to pixel threshold changes.
    double relativeThresholdSmearing = 0.1; // values in /100
    std::map<std::pair<int,int>, double> thresholdMap;
    // Save the threshold 
    TH1D *h1DThreshold =  new TH1D("h1DUTThreshold", "h1DThreshold", 100, 100,200);
    TH2D *h2DThreshold    = new TH2D("h2DUTThreshold", "h2DUTThreshold", 512, 0, 512, 512, 0, 512);
    for (int x = 0; x < 512; x++)
    {
        for (int y = 0; y < 512; y++)
        {

            static std::mt19937 gen(std::random_device{}());
            std::normal_distribution<> dist(1.0, relativeThresholdSmearing);
            thresholdMap[{x,y}] = inputThreshold * dist(gen);
            //std::cout << "smeared Threshold = " << inputThreshold * dist(gen) << std::endl;
            h1DThreshold->Fill(inputThreshold * dist(gen));
            h2DThreshold->Fill(x, y, inputThreshold * dist(gen));
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

        UInt_t maltaPixel;
        UInt_t maltaGroup;
        UInt_t maltaParity;
        UInt_t maltaDelay;
        UInt_t maltaDColumn;
        std::vector<std::vector<std::pair<UInt_t, double>>> digitizedWords;
        std::vector<std::pair<UInt_t, double>> merger;
        // Hardcoded Word spacing
        double wordSpacing = 1.6;//1.6;

        // Sort all words based on the timing. 
        std::vector<std::pair<std::pair<int,std::pair<int,int>>, double>> sortedTimings;

        for (const auto& entry : enMap) 
        {
            int eventID = entry.first.first;
            int pixX = entry.first.second.first;
            int pixY = entry.first.second.second;

            auto itThr = thresholdMap.find({pixX, pixY});
            double threshold = itThr->second;
            double cenergy = entry.second;
            double timing = GetTimingOffset(cenergy, threshold); // This is the global timing
            double timeWalk = GetTimingOffset(cenergy, threshold); // This is just the time walk

            auto it = timeMap.find({eventID, {pixX, pixY}});
            // Row correction also of 7ns/ 512 rows + global GEANT4 timestamp
            timing += *std::max_element(it->second.begin(), it->second.end()) + pixX * 0.0125; 
            timeWalk += pixX * 0.0125;

            //if(debug) std::cout << "Event ID: " << eventID << "; pixX: " << pixX << ";pixY: " << pixY << "; corrEnergy: " << cenergy << "; timewalk: "<< timeWalk << std::endl;
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
            double threshold = itThr->second;

            double timing = entry.second;
            auto it = enMap.find({eventID, {pixX, pixY}});
            double cenergy = it->second;
            
            if (cenergy < threshold) continue;
            maltaDColumn = pixX / 2;
            maltaGroup = pixY / 16;
            maltaDelay = 1;
            maltaParity = (pixY /8) %2; 
            maltaPixel = 0b0000000000000000;
            //maltaPixel ^= (1 << pixY % 8 + 8 *(pixX %2)); //* (pixX % 2 +1));
            maltaPixel ^= (1 << ( (pixY % 8) + (8 * (pixX % 2)) ));
            UInt_t word = 0;
            word |= (maltaPixel & 0xFFFF) << (5 + 1 + 8); // shift left by 14 bits
            word |= (maltaGroup & 0x1F)       << (1 + 8);     // shift left by 9 bits
            word |= (maltaParity & 0x1)       << 8;           // shift left by 8 bits
            word |= (maltaDColumn & 0xFF);                    // stays in lower 8 bits
            if (debug)
            {
                std::cout << "Event ID: " << eventID << "; pixX: " << pixX << ";pixY: " << pixY << "; corrEnergy: " << cenergy << "; Timing: " << std::setprecision(10) <<  timing << std::endl;
                std::cout << "DColumn: " << std::bitset<8>(maltaDColumn) << "; Group: " << std::bitset<5>(maltaGroup) << "; Parity: " << std::bitset<1>(maltaParity) << "; MaltaPixel: " << std::bitset<16>(maltaPixel) << std::endl;
                std::cout << "Encoded word: " << std::bitset<32>(word) << std::endl;
            }
            
            //Now I digitized all my words. Next step is merging them based on timing
            if (timing >= t0 && timing < t0 + wordSpacing)
            {
                //std::cout << "Merging into current word bucket with timing." << timing << std::endl;
                merger.push_back(std::make_pair(word, timing));
            }
            else
            {
                //std::cout << "Timing: " << timing <<std::endl; 
                t0 = timing;
                digitizedWords.push_back(merger);
                if(merger.size() == 0) std::cout << "Word Size = " << merger.size() << std::endl; 
                merger.clear();
                merger.push_back(std::make_pair(word, timing)); 
            }
        }
        // Now I have my words in their correct time buckets. Next is the merging logic.

        std::vector<std::pair<UInt_t, double>> mergedWords;
        for (int i =0; i< digitizedWords.size(); i++)
        {
            UInt_t mergedWord; 
            std::vector<double> leadingTime;
            int aux = 0;
            for (auto [word, timing] : digitizedWords[i])
            {
                mergedWord |= word;
                leadingTime.push_back(timing);
                aux++;
            }
            //if (aux >1) std::cout << "A MERGING HAS OCCURED" << std::endl;
            // Save the merged word and the fastest hit time
            if (digitizedWords[i].size() == 0) continue; // Skip empty vectors
            mergedWords.push_back({mergedWord, *std::max_element(leadingTime.begin(), leadingTime.end())});
            // TODO: Timing should be given via a clock. Find from Carlos the frequency.
            
            // Reset fot next iteration
            mergedWord = 0;
            leadingTime.clear();
        }

        // Now I have merged words. Next step is decoding them back to position and time
        // However, first I need the infrastructure to save them in a root tree.
        mkdir((inputPath + saveName).c_str(), 0777);
        TFile *outfile = new TFile((inputPath + saveName + "/Plane" + std::to_string(i) + "ReconstructedHitsThr" + std::to_string(int(inputThreshold)) + ".root").c_str(), "RECREATE");

        // Create a TTree
        TTree *recontructedTree = new TTree("ReconstructedHits", "Reconstructed Hits");

        // Variables for branches
        int reconstructedPixX, reconstructedPixY, nHits;
        double reconstructedTiming, reconstructedThreshold;

        // Create branches
        recontructedTree->Branch("PixX", &reconstructedPixX, "PixX/I");
        recontructedTree->Branch("PixY", &reconstructedPixY, "PixY/I");
        recontructedTree->Branch("timing", &reconstructedTiming, "timing/D");
        recontructedTree->Branch("NHits", &nHits, "NHits/I");
        recontructedTree->Branch("threshold", &reconstructedThreshold, "threshold/I");


        for (auto [word, timing] : mergedWords) 
        {
            std::vector< std::pair<std::pair<int,int>, int> > pixelPositions = decodedDigitalWord(word);

            for (const auto& pos : pixelPositions) 
            {
                if(debug) std::cout << "Decoded pixel position: (" << pos.first.first << ", " << pos.first.second << ")" << "; Timing: "<< std::setprecision(8) << timing << std::endl;
                reconstructedTiming = timing; // + 100 adds a constant 100 ns delay in order to replicate the TB data 
                reconstructedPixX = pos.first.first;
                reconstructedPixY = pos.first.second;
                nHits = pos.second;
                recontructedTree->Fill();
            }
        }
        // Save a single threshold value for the tracking planes. Value chosen: 2000. Should be standard for all runs
        
        //if(i == 0) recontructedTree->Write();
        //else if( threshold == 2000) recontructedTree->Write();
        recontructedTree->Write();
        outfile->Close();
        std::cout << "Finishing up analyzing Plane" << i << std::endl;
    }


    auto end = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double, std::milli> elapsed = end - start;

    std::cout << "############################# Digital Processing stopped after " << elapsed.count() << "ms" << std::endl;
}
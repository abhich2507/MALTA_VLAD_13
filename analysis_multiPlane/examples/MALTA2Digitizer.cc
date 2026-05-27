#include <iostream>
#include <iomanip>
#include <bitset>
#include <vector>
#include <string>
#include <random>

#include "DigitalProcessing_multiPlane.hh"
#include "Utils.hh"

namespace UI
{
    constexpr const char* RESET  = "\033[0m";
    constexpr const char* BOLD   = "\033[1m";
    constexpr const char* CYAN   = "\033[36m";
    constexpr const char* GREEN  = "\033[32m";
    constexpr const char* YELLOW = "\033[33m";
    constexpr const char* RED    = "\033[31m";
    constexpr const char* BLUE   = "\033[34m";
    constexpr const char* MAG    = "\033[35m";

    void PrintSeparator(char c='=', int width=80)
    {
        std::cout << std::string(width, c) << "\n";
    }

    void PrintTitle(const std::string& title)
    {
        std::cout << "\n";
        PrintSeparator('=');
        std::cout << BOLD << CYAN << title << RESET << "\n";
        PrintSeparator('=');
    }

    void PrintSubTitle(const std::string& title)
    {
        std::cout << "\n"
                  << BOLD << BLUE << ">> " << title << RESET << "\n";
        PrintSeparator('-');
    }

    void PrintWordFormatted(const std::string& bits)
    {
        std::cout
            << GREEN << "|" << bits.substr(0,16) << RESET
            << YELLOW << "|" << bits.substr(16,5) << RESET
            << RED << "|" << bits.substr(21,1) << RESET
            << MAG << "|" << bits.substr(22,8) << "|" << RESET
            << "\n";
    }
}

int main()
{
    using namespace UI;

    PrintTitle("MALTA2 DIGITAL PROCESSING TEST");

    int numRawHits;

    std::cout << BOLD
              << "Choose number of raw hits: "
              << RESET;

    std::cin >> numRawHits;

    std::vector<RawHit> rawHits{};

    // ---------------------------------------------------------
    // Threshold map
    // ---------------------------------------------------------

    ThresholdMap thresholdMap{};

    for(int i=0; i<511; i++)
    {
        for(int j=0; j<511; j++)
        {
            thresholdMap[{i,j}] = 200;
        }
    }

    // ---------------------------------------------------------
    // Input hits
    // ---------------------------------------------------------

    PrintSubTitle("RAW HIT INPUT");

    for(int i=0; i<numRawHits; i++)
    {
        std::cout << "\n"
                  << BOLD << CYAN
                  << "Hit #" << i
                  << RESET << "\n";

        double energy, time;
        int x, y;

        std::cout << "  Energy [e-] : ";
        std::cin >> energy;

        std::cout << "  Time [ns]   : ";
        std::cin >> time;

        std::cout << "  X coordinate: ";
        std::cin >> x;

        std::cout << "  Y coordinate: ";
        std::cin >> y;

        RawHit rawHit = {{1, i, x, y}, energy, time};

        rawHits.push_back(rawHit);
    }

    // ---------------------------------------------------------
    // Build maps
    // ---------------------------------------------------------

    auto [enMap, timeMap] = BuildEnergyTimeMap(rawHits);

    PrintSubTitle("ENERGY/TIME MAP");

    std::cout
        << std::left
        << std::setw(10) << "X"
        << std::setw(10) << "Y"
        << std::setw(18) << "Energy [e-]"
        << std::setw(18) << "Time [ns]"
        << "\n";

    PrintSeparator('-');

    for(const auto& entry : enMap)
    {
        const HitKey& key = entry.first;

        auto it = timeMap.find(key);

        std::vector<double> time = it->second;
        double energy = entry.second;

        std::cout
            << std::setw(10) << key.x
            << std::setw(10) << key.y
            << std::setw(18) << std::fixed << std::setprecision(2) << energy
            << std::setw(18) << time[0]
            << "\n";
    }

    // ---------------------------------------------------------
    // Load configuration
    // ---------------------------------------------------------

    AnaFlags config{};

    std::string configPath = "configs/analysis_flags_SP.cfg";

    LoadAnalysisFlagsFromFile(configPath, config);

    // ---------------------------------------------------------
    // Correct timings
    // ---------------------------------------------------------

    auto sortedTimings =
        CorrectAndSortTimeMap(
            enMap,
            timeMap,
            thresholdMap,
            config,
            std::random_device{}()
        );

    PrintSubTitle("CORRECTED & SORTED TIMINGS");

    std::cout
        << std::left
        << std::setw(10) << "X"
        << std::setw(10) << "Y"
        << std::setw(18) << "Energy [e-]"
        << std::setw(18) << "Corrected t [ns]"
        << "\n";

    PrintSeparator('-');

    for(const auto& entry : sortedTimings)
    {
        const HitKey& key = entry.first;

        double timing = entry.second;

        auto it = enMap.find(key);

        double energy = it->second;

        std::cout
            << std::setw(10) << key.x
            << std::setw(10) << key.y
            << std::setw(18) << std::fixed << std::setprecision(2) << energy
            << std::setw(18) << timing
            << "\n";
    }

    // ---------------------------------------------------------
    // Digitization
    // ---------------------------------------------------------

    PrintSubTitle("DIGITIZATION");

    auto digitizedWords =
        AssignMALTA2WordBuckets(
            enMap,
            sortedTimings,
            thresholdMap,
            config
        );

    auto mergedWords = MergeMALTA2Words(digitizedWords);

    std::cout
        << GREEN
        << "Digitization complete."
        << RESET
        << "\n";

    // ---------------------------------------------------------
    // Print buckets
    // ---------------------------------------------------------

    for(int i=0; i<digitizedWords.size(); i++)
    {
        auto words = digitizedWords[i];

        std::cout << "\n";

        PrintSeparator('=');

        std::cout
            << BOLD
            << CYAN
            << "WORD BUCKET #" << i
            << RESET
            << "\n";

        PrintSeparator('=');

        std::cout
            << "Contains "
            << GREEN << words.size() << RESET
            << " words.\n";

        // -----------------------------------------------------
        // Individual words
        // -----------------------------------------------------

        for(int j=0; j<words.size(); j++)
        {
            auto word = words[j];

            std::cout << "\n"
                      << BOLD << "Word #" << j << RESET
                      << "\n";

            PrintSeparator('-');

            auto decodedHit =
                decodedDigitalWord(word.word, config);

            std::cout
                << YELLOW
                << "Decoded Hits:"
                << RESET
                << "\n";

            for(const auto& hit : decodedHit)
            {
                std::cout
                    << "  -> X = "
                    << std::setw(4) << hit.x
                    << "   Y = "
                    << std::setw(4) << hit.y
                    << "\n";
            }

            std::string bits =
                std::bitset<30>(word.word).to_string();

            std::cout
                << "\n"
                << BOLD
                << "30-bit Word:"
                << RESET
                << "\n";

            PrintWordFormatted(bits);
        }

        // -----------------------------------------------------
        // Merged word
        // -----------------------------------------------------

        auto mergedWord = mergedWords[i];

        std::string mergedBits =
            std::bitset<30>(mergedWord.word).to_string();

        std::cout << "\n";

        PrintSubTitle("MERGED WORD");

        PrintWordFormatted(mergedBits);

        // -----------------------------------------------------
        // Decode merged word
        // -----------------------------------------------------

        std::cout
            << "\n"
            << YELLOW
            << "Decoded merged hits:"
            << RESET
            << "\n";

        auto decodedHit =
            decodedDigitalWord(
                mergedWord.word,
                config
            );

        for(const auto& hit : decodedHit)
        {
            std::cout
                << "  -> X = "
                << std::setw(4) << hit.x
                << "   Y = "
                << std::setw(4) << hit.y
                << "\n";
        }
    }

    PrintTitle("PROCESSING COMPLETE");

    return 0;
}
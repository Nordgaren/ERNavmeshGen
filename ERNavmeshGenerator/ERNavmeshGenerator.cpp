#include <iostream>
#include <string>
#include <filesystem>
#include <fstream>
#include <sstream>
#include "../ERNavmeshGen/API.h"

namespace fs = std::filesystem;

void PrintUsage(const char* programName) {
    std::cerr << "Usage: " << programName << " [options]\n\n"
              << "Options:\n"
              << "  -g, --game <path>        (Required) Path to the game directory or executable.\n"
              << "  -i, --in <path>          (Required) Path to the input collision file.\n"
              << "  -o, --out <path>         (Optional) Path to the output navmesh file (auto-calculated if omitted).\n"
              << "  -c, --compendium <path>  (Optional) Path to the compendium file.\n"
              << "  -s, --settings <path>    (Optional) Path to a JSON configuration file for navmesh settings.\n";
}

int main(int argc, char* argv[]) {
    std::string gamePath;
    std::string inPathStr;
    std::string outPath;
    std::string compendiumPath;
    std::string settingsPath;

    // 1. Parse Arguments using flags
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if ((arg == "-g" || arg == "--game") && i + 1 < argc) {
            gamePath = argv[++i];
        } else if ((arg == "-i" || arg == "--in") && i + 1 < argc) {
            inPathStr = argv[++i];
        } else if ((arg == "-o" || arg == "--out") && i + 1 < argc) {
            outPath = argv[++i];
        } else if ((arg == "-c" || arg == "--compendium") && i + 1 < argc) {
            compendiumPath = argv[++i];
        } else if ((arg == "-s" || arg == "--settings") && i + 1 < argc) {
            settingsPath = argv[++i];
        } else {
            std::cerr << "Error: Unknown or incomplete argument '" << arg << "'\n\n";
            PrintUsage(argv[0]);
            return 1;
        }
    }

    // 2. Validate Required Arguments
    if (gamePath.empty() || inPathStr.empty()) {
        std::cerr << "Error: --game and --in are required arguments.\n\n";
        PrintUsage(argv[0]);
        return 1;
    }

    fs::path inPath = inPathStr;

    if (!fs::exists(gamePath)) {
        std::cerr << "Error: The game path '" << gamePath << "' does not exist.\n";
        return 1;
    }

    if (!fs::exists(inPath)) {
        std::cerr << "Error: The input file '" << inPath.string() << "' does not exist.\n";
        return 1;
    }

    // 3. Compute the output path if not provided
    if (outPath.empty()) {
        std::string fileName = inPath.filename().string();
        
        if (fileName.empty()) {
            std::cerr << "Error: Invalid input filename.\n";
            return 1;
        }

        // Change the first character to 'n'
        fileName[0] = 'n';

        // Set the calculated path as our "out" file
        outPath = (inPath.parent_path() / fileName).string();
        std::cout << "Auto-calculated output path: " << outPath << std::endl;
    } 

    // 4. Handle Settings JSON (if provided)
    if (!settingsPath.empty()) {
        if (!fs::exists(settingsPath)) {
            std::cerr << "Error: The settings file '" << settingsPath << "' does not exist.\n";
            return 1;
        }

        // Read the entire JSON file into a string
        std::ifstream settingsFile(settingsPath);
        std::stringstream buffer;
        buffer << settingsFile.rdbuf();
        std::string jsonContent = buffer.str();

        // Pass the parsed JSON string to the API we built
        // Assuming your header exposes the LoadSnapshotFromJson function
        LoadSnapshotFromJson(jsonContent.c_str());
        std::cout << "Loaded navmesh generation settings from JSON." << std::endl;
    }

    // 5. Execute API Generation calls
    SetGamePath(gamePath.c_str());
    
    const char* pCompendium = compendiumPath.empty() ? nullptr : compendiumPath.c_str();
    GenerateNavMeshFromCollisionAPI(inPath.string().c_str(), outPath.c_str(), pCompendium);

    std::cout << "Navmesh generation complete.\n";
    return 0;
}
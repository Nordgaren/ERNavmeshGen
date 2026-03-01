#include "API.h"

#include "export.h"
#include <string>

#include "HavokFunctions.h"
#include "NavGen.h"
#include "Util/fileio.h"

static std::string gamePath = "";



bool SetGamePath(const char* path)
{
    gamePath = path;
    return true;
}

NAVMA_API bool GenerateNavMeshFromCollisionAPI(const char* path, const char* outFile,const char* compendiumPath)
{
    if (path == nullptr)
    {
        return false;
    }
    
    if (!Havok::init(gamePath))
    {
        PLOG_ERROR << "Failed to init Havok";
        return false;
    }

    std::string colPath = path;
    std::string colOutPath = outFile;
    std::string compendium = compendiumPath ? compendiumPath : "";
	PLOG_INFO << "Generating navmesh from: " << colPath;
    if (!GenerateNavMeshFromCollision(colPath, colOutPath, compendium))
    {
        PLOG_INFO << "Navmesh failed to generate from" << colPath;
        return false;
    }
	PLOG_INFO << "Navmesh generated from" << colPath;

    return true;
}

static bool BatchGenerateNavMeshFromCollision(const std::string& folder, const std::string& outFolder, const char* compendiumPath) {
    if (folder.empty() || outFolder.empty()) {
        return false;
    }

    if (!Havok::init(gamePath))
    {
        PLOG_ERROR << "Failed to init Havok";
        return false;
    }
    
    std::string compendium = compendiumPath ? compendiumPath : "";
    
    PLOG_INFO << "Generating navmesh from all files in " << folder;
    for (std::string file : fileIO::GetAllFilesInFolder(folder)) {
        if (ends_with(file, ".hkx")) {
            PLOG_INFO << "Generating navmesh from: " << file;
            std::filesystem::path pathIn { file };
            const std::string inFilename = "n" + pathIn.filename().string().substr(1);
            std::filesystem::path pathOut { outFolder };
            pathOut.replace_filename(inFilename);
             if (!GenerateNavMeshFromCollision(file, pathOut.string(), compendium)) {
                 PLOG_INFO << "Navmesh failed to generate from" << file;
                 return false;
             }
        }
    }

    PLOG_INFO << "Generated navmeshes from all files in " << folder;
    
    return true;
}


bool BatchGenerateNavMeshFromCollisionAPI(const char* folder, const char* outFolder, const char* compendiumPath)
{
    if (folder == nullptr) {
        return false;
    }
    
    if (outFolder == nullptr || strlen(outFolder) == 0)
    {
        outFolder = folder;
    }
    
    std::string folderPath = folder;
    std::string outFolderPath = outFolder;
    return BatchGenerateNavMeshFromCollision(folderPath, outFolderPath, compendiumPath);
}

bool Close()
{
    HavokFunctions::denit();
    return true;
}


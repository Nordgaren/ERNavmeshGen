#include "API.h"

#include <iostream>
#include <plog/Logger.h>
#include "export.h"
#include <string>

#include "HavokFunctions.h"
#include "NavGen.h"
#include "Util/fileio.h"

static std::string gamePath = "";

void SetGamePath(const char* path)
{
    gamePath = path;
}

NAVMA_API bool GenerateNavMeshFromCollisionAPI(const char* path, const char* compendiumPath)
{
    if (path == nullptr)
    {
        return false;
    }
    
    Havok::init(gamePath);
	
    std::string colPath = path;
    std::string compendium = compendiumPath ? compendiumPath : "";
	PLOG_INFO << "Generating navmesh from: " << colPath;
    if (!GenerateNavMeshFromCollision(colPath, compendiumPath))
    {
        PLOG_INFO << "Navmesh failed to generate from" << colPath;
        return false;
    }
	PLOG_INFO << "Navmesh generated from" << colPath;

    return true;
}

static bool BatchGenerateNavMeshFromCollision(const std::string& folder, const char* compendiumPath) {
    if (folder.empty()) {
        return false;
    }

    Havok::init(gamePath);
    std::string compendium = compendiumPath ? compendiumPath : "";
    
    PLOG_INFO << "Generating navmesh from all files in " << folder;
    for (std::string file : fileIO::GetAllFilesInFolder(folder)) {
        if (ends_with(file, ".hkx")) {
            PLOG_INFO << "Generating navmesh from: " << file;
            // Sleep(5000);
             if (!GenerateNavMeshFromCollision(file, compendiumPath)) {
                 PLOG_INFO << "Navmesh failed to generate from" << file;
                 return false;
             }
        }
    }

    PLOG_INFO << "Generated navmeshes from all files in " << folder;
    
    return true;
}


bool BatchGenerateNavMeshFromCollisionAPI(const char* folder, const char* compendiumPath)
{
    if (folder == nullptr) {
        return false;
    }
    
    std::string folderPath = folder;
    BatchGenerateNavMeshFromCollision(folderPath, compendiumPath);

    return true;
}


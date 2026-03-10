#pragma once
#define NAVMA_EXPORTS
#include "export.h"

struct hkaiNavMeshGenerationSnapshot;

NAVMA_API void LoadSnapshotFromJson(const char* jsonString);
NAVMA_API void SetNavmeshGenerationSettings(hkaiNavMeshGenerationSnapshot* snapshot);
NAVMA_API bool SetGamePath(const char* path);
NAVMA_API bool GenerateNavMeshFromCollisionAPI(const char* path, const char* outFile,const char* compendiumPath);
NAVMA_API bool BatchGenerateNavMeshFromCollisionAPI(const char* folder, const char* outFolder, const char* compendiumPath);
NAVMA_API bool Close();

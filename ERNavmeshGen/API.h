#pragma once
#define NAVMA_EXPORTS
#include "export.h"

NAVMA_API void SetGamePath(const char* path);
NAVMA_API bool GenerateNavMeshFromCollisionAPI(const char* path, const char* compendiumPath);
NAVMA_API bool BatchGenerateNavMeshFromCollisionAPI(const char* folder, const char* compendiumPath);


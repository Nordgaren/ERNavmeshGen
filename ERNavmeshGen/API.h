#pragma once
#define NAVMA_EXPORTS
#include "export.h"

NAVMA_API void GenerateNavMeshFromCollisionAPI(const char* pathIn, const char* compendiumPathIn, const char* pathOut);
NAVMA_API void GenerateNavMeshFromCollisionTestHardcoded();

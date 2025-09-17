#include "API.h"

#include <iostream>
#include <plog/Logger.h>
#include "export.h"
#include <string>

#include "HavokFunctions.h"
#include "NavGen.h"


NAVMA_API void GenerateNavMeshFromCollisionAPI(const char* pathIn, const char* compendiumPathIn, const char* pathOut)
{
    std::string colPath = pathIn;
    std::string compendiumPath = compendiumPathIn;
    std::string navPathOut = pathOut;
    GenerateNavMeshFromCollision(colPath, compendiumPath, navPathOut);
}

// A function with hardcoded paths to the input/output files/folders. This is just for initial testing and will be replaced
// as soon as this call succeeds
NAVMA_API void GenerateNavMeshFromCollisionTestHardcoded()
{
    std::cout << "Before init" << "\n";
    Havok::init();

    // This index is the one that I haven't initialized properly.
    // If possible, find the constructor and call it. I have not been able to debug
    DWORD tlsIndexOne = *HavokFunctions::CSHavokMan::HavokTlsValueOne;
    void* alloc = malloc(0x30);
    int* end = reinterpret_cast<int*>(static_cast<char*>(alloc) + 0x24);
    *end = -1;
    TlsSetValue(tlsIndexOne, alloc);
    
    DWORD tlsIndexTwo = *HavokFunctions::CSHavokMan::HavokTlsValueTwo;
    HavokFunctions::CSHavokMan::CSHavokManImp* csHavokManImp = *HavokFunctions::CSHavokMan::CSHavokManImpPtr;
    TlsSetValue(tlsIndexTwo, csHavokManImp->hkLifoAllocator);
    std::cout << "After init" << "\n";
        
    GenerateNavMeshFromCollision(
        R"(E:\Users\Nordgaren\Documents\ModEngine-2.1.0.0-win64\morrowring\map\m60\m60_41_41_00\h60_41_41_00-hkxbdt\h60_41_41_00_414100.hkx)",
        R"(E:\Users\Nordgaren\Documents\ModEngine-2.1.0.0-win64\morrowring\map\m60\m60_41_41_00\h60_41_41_00-hkxbdt\h60_41_41_00.compendium)",
        R"(E:\Users\Nordgaren\Documents\ModEngine-2.1.0.0-win64\morrowring\map\m60\m60_41_41_00\)"
        );
    

    std::cout << "After GenerateNavMeshFromCollision" << "\n";
}


#include "gtest/gtest.h"
#include "../ERNavmeshGen/API.h"

// Your eldenring.exe path
const char* eldenRingGamePath = std::getenv("ERNAVMA_GAME_PATH");

// TEST(ERNAVMA_BATCH_CONVERT, EXPORTS)
// {
//     SetGamePath(eldenRingGamePath);
//     const char* in = std::getenv("ERNAVMA_BATCH_CONVERT_IN"); // ERNAVMA_BATCH_CONVERT_IN = P:\ath\to\test\folder\;
//     const char* out = std::getenv("ERNAVMA_BATCH_CONVERT_OUT"); // ERNAVMA_BATCH_CONVERT_OUT = P:\ath\to\out\folder\;
//     // Leave this empty if you are testing on collisions made with modding tools.
//     const char* compendium = std::getenv("ERNAVMA_BATCH_CONVERT_COMPENDIUM"); // ERNAVMA_BATCH_CONVERT_COMPENDIUM = P:\ath\to\test.compendium;
//     EXPECT_TRUE(BatchGenerateNavMeshFromCollisionAPI(R"(C:\Users\rscos\Documents\mod-dev\elden-scrolls\mod\cache\terrain\)", R"(C:\Users\rscos\Documents\mod-dev\elden-scrolls\mod\cache\terrain\)", compendium));
// }

TEST(ERNAVMA_CONVERT, EXPORTS)
{
    SetGamePath(eldenRingGamePath);
    const char* in = std::getenv("ERNAVMA_CONVERT_IN"); // ERNAVMA_CONVERT_IN = P:\ath\to\test.hkx;
    const char* out = std::getenv("ERNAVMA_CONVERT_OUT"); // ERNAVMA_CONVERT_OUT = P:\ath\to\out.hkx;
    // Leave this empty if you are testing on collisions made with modding tools.
    const char* compendium = std::getenv("ERNAVMA_CONVERT_COMPENDIUM"); // ERNAVMA_CONVERT_COMPENDIUM = P:\ath\to\test.compendium;

    EXPECT_TRUE(GenerateNavMeshFromCollisionAPI(
        R"(C:\Users\rscos\Documents\mod-dev\elden-scrolls\mod\cache\terrain\ext-1,-8_split1.hkx)", 
        R"(C:\Users\rscos\Documents\mod-dev\elden-scrolls\mod\cache\terrain\next-1,-8_split1.hkx)", 
        compendium));
}

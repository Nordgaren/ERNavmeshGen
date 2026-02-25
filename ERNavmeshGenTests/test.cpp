#include "gtest/gtest.h"
#include "../ERNavmeshGen/API.h"

// Your eldenring.exe path
const char* eldenRingGamePath = std::getenv("ERNAVMA_GAME_PATH");

TEST(ERNAVMA_BATCH_CONVERT, EXPORTS)
{
  SetGamePath(R"(G:\Steam\steamapps\common\ELDEN RING\Game\eldenring.exe)");
  const char* in = std::getenv("ERNAVMA_BATCH_CONVERT_IN"); // ERNAVMA_BATCH_CONVERT_IN = P:\ath\to\test\folder\;
  // Leave this empty if you are testing on collisions made with modding tools.
  const char* compendium = std::getenv("ERNAVMA_BATCH_CONVERT_COMPENDIUM"); // ERNAVMA_BATCH_CONVERT_COMPENDIUM = P:\ath\to\test.compendium;
  EXPECT_TRUE(BatchGenerateNavMeshFromCollisionAPI(R"(C:\Users\rscos\Documents\mod-dev\elden-scrolls\mods\map\m60\m60_39_50_00\l60_39_50_00-hkxbdt\Folder\)", nullptr));
}

// TEST(ERNAVMA_CONVERT, EXPORTS)
// {
//   SetGamePath(eldenRingGamePath);
//   const char* in = std::getenv("ERNAVMA_CONVERT_IN"); // ERNAVMA_CONVERT_IN = P:\ath\to\test.hkx;
//   // Leave this empty if you are testing on collisions made with modding tools.
//   const char* compendium = std::getenv("ERNAVMA_CONVERT_COMPENDIUM"); // ERNAVMA_CONVERT_COMPENDIUM = P:\ath\to\test.compendium;
//   
//   EXPECT_TRUE(GenerateNavMeshFromCollisionAPI(in, compendium));
// }


#include "gtest/gtest.h"
#include "../ERNavmeshGen/API.h"

// Your eldenring.exe path
const char* er = std::getenv("ERNAVMA_GAME_PATH");

TEST(ERNAVMA_BATCH_CONVERT, EXPORTS)
{
  SetGamePath(er);
  const char* in = std::getenv("ERNAVMA_BATCH_CONVERT_IN"); // ERNAVMA_BATCH_CONVERT_IN = P:\ath\to\test\folder\;
  // Leave this empty if you are testing on collisions made with modding tools.
  const char* compendium = std::getenv("ERNAVMA_BATCH_CONVERT_COMPENDIUM"); // ERNAVMA_BATCH_CONVERT_COMPENDIUM = P:\ath\to\test.compendium;
  
  EXPECT_TRUE(BatchGenerateNavMeshFromCollisionAPI(in, compendium));
}

TEST(ERNAVMA_CONVERT, EXPORTS)
{
  SetGamePath(er);
  const char* in = std::getenv("ERNAVMA_CONVERT_IN"); // ERNAVMA_CONVERT_IN = P:\ath\to\test.hkx;
  // Leave this empty if you are testing on collisions made with modding tools.
  const char* compendium = std::getenv("ERNAVMA_CONVERT_COMPENDIUM"); // ERNAVMA_CONVERT_COMPENDIUM = P:\ath\to\test.compendium;
  
  EXPECT_TRUE(GenerateNavMeshFromCollisionAPI(in, compendium));
}


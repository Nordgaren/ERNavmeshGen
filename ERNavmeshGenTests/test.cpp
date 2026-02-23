#include "gtest/gtest.h"
#include "../ERNavmeshGen/API.h"

// Your eldenring.exe path
const char* er = R"(G:\Steam\steamapps\common\ELDEN RING\Game\eldenring.exe)";

TEST(ERNAVMA_BATCH_CONVERT, EXPORTS)
{
  SetGamePath(er);
  const char* in = R"(P:\ath\to\test\folder\)";
  // Leave this empty if you are testing on collisions made with modding tools.
  const char* compendium = nullptr; // R"(P:\ath\to\test.compendium)";
  
  EXPECT_TRUE(BatchGenerateNavMeshFromCollisionAPI(in, compendium));
}

TEST(ERNAVMA_CONVERT, EXPORTS)
{
  SetGamePath(er);
  const char* in = R"(P:\ath\to\test.hkx)";
  // Leave this empty if you are testing on collisions made with modding tools.
  const char* compendium = nullptr; // R"(P:\ath\to\test.compendium)";
  
  EXPECT_TRUE(GenerateNavMeshFromCollisionAPI(in, compendium));
}


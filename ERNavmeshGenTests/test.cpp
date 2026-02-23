#include "gtest/gtest.h"
#include "../ERNavmeshGen/API.h"

const char* er = R"(G:\Steam\steamapps\common\ELDEN RING\Game\eldenring.exe)";

TEST(DS3NAVMA_BATCH_CONVERT, EXPORTS)
{
  SetGamePath(er);
  const char* in = R"(C:\Users\rscos\Documents\mod-dev\elden-scrolls\mods\map\m34\m34_10_00_00\l34_10_00_00-hkxbhd)";
  const char* compendium = R"(C:\Users\rscos\Downloads\l31_00_00_00.compendium)";
  //bool lol = BatchGenerateNavMeshFromCollisionAPI(in, "");
  bool lol = GenerateNavMeshFromCollisionAPI(in, "");
  
  
  
  EXPECT_TRUE(lol);
}

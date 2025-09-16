#pragma once

#ifdef NAVMA_EXPORTS
#define NAVMA_API extern "C" __declspec(dllexport)
#else
#define NAVMA_API extern "C" __declspec(dllimport)
#endif


#include <windows.h>
#define NAVMA_EXPORTS
#include <cstdio>
#include <plog/Log.h>
#include <plog/Initializers/RollingFileInitializer.h>
#include "export.h"

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        if (AllocConsole())
        {
            FILE* fpstdout = stdout;
            FILE* fpstderr = stderr;
            freopen_s(&fpstdout,"CONOUT$", "w", stdout);
            freopen_s(&fpstderr,"CONOUT$", "w", stderr);
            SetWindowText(GetConsoleWindow(), L"ERNavmeshGen");
        }
        plog::init(plog::verbose, R"(C:\Users\Nordgaren\RiderProjects\ERNavmeshGen\x64\Release\ERNavmeshGenLog.txt)");
        PLOG_INFO << "NavmeshGenLog.txt";
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
	
    return TRUE;
}


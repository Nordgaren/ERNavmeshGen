#pragma once
#include <cstdint>
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <intrin.h>
#include <fstream>

#define WIN_X64

#include <vector>

typedef uintptr_t NAKED_ENTRY_POINT(void);
using PNAKED_ENTRY_POINT = NAKED_ENTRY_POINT *;

typedef uintptr_t ENTRY_POINT(int, char**);
using PENTRY_POINT = ENTRY_POINT*;

typedef int WINAPI WIN_MAIN(HINSTANCE, HINSTANCE, PSTR, int);
using PWIN_MAIN = WIN_MAIN*;

typedef BOOL WINAPI DLL_MAIN(
    HINSTANCE,  // handle to DLL module
    DWORD,     // reason for calling function
    LPVOID     // reserved
);  
using PDLL_MAIN = DLL_MAIN*;

class PEHelper {
public:
    PEHelper(uintptr_t baseAddress) : _baseAddress(baseAddress), _isMapped(checkMapped()) {}
    explicit PEHelper(HMODULE hmodule) : PEHelper(reinterpret_cast<uintptr_t>(hmodule)) {}

    PIMAGE_DOS_HEADER GetDosHeader() const;
    PIMAGE_NT_HEADERS GetNTHeaders() const;
    PIMAGE_SECTION_HEADER GetSectionHeaders() const;
    PNAKED_ENTRY_POINT GetNakedEntryPoint() const;
    PIMAGE_DATA_DIRECTORY GetImageDataDirectory(int directory) const;
    PENTRY_POINT GetEntryPoint() const;
    PWIN_MAIN GetWinMain() const;
    PDLL_MAIN GetDLLMain() const;
    inline uintptr_t GetBaseAddress() const;
    bool FixImportAddressTable() const;
    bool FixMemPermissions() const;
    static const PEHelper *ManualLoadPE(std::string& path);
    bool FixReloc() const;
    bool PatchIAT() const;
    bool HookIATEntry(const std::string& moduleName, const std::string& functionName, uintptr_t newFunc) const;
    bool HookIATEntry(const char* moduleName, const char* functionName, uintptr_t newFunc) const;
    inline bool IsMapped() const;
    bool IsDLL() const;
    bool IsExecutable() const;
    bool ResolveExceptionHandlers() const;

private:
    static std::vector<uint8_t> loadFileFromDisk(std::string& path);
    uintptr_t _baseAddress;
    bool _isMapped;
    bool checkMapped() const;
    bool checkMappedExports() const;
    uint32_t rva2Foa(uint32_t rva) const;
};

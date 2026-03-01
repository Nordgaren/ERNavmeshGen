#include "Pattern.h"
#include <Psapi.h>
#include <unordered_map>

static HANDLE hProcess = GetCurrentProcess();
static std::unordered_map<const wchar_t*, MODULEINFO> mModuleInfoMap = {};

DWORD64 Pattern::defaultModule = 0;
DWORD Pattern::defaultModuleSize = 0;

LPMODULEINFO Pattern::GetModuleInfo(const wchar_t* szModule) {
	if (mModuleInfoMap.contains(szModule)) {
		return &mModuleInfoMap[szModule];
	}
	HMODULE hModule = GetModuleHandle(szModule);
	MODULEINFO moduleInfo = {};
	if (GetModuleInformation(hProcess, hModule, &moduleInfo, sizeof(MODULEINFO)) == 0) {
		return nullptr;
	}
	
	mModuleInfoMap.emplace(std::make_pair(szModule, moduleInfo));

	return &mModuleInfoMap[szModule];
}

DWORD64 Pattern::BaseAddress(const wchar_t* szModule)
{
	if  (defaultModule != 0)
	{
		return defaultModule;
	}
	
	LPMODULEINFO lpmInfo = GetModuleInfo(szModule);
	if (lpmInfo != nullptr)
	{
		defaultModule = (DWORD64)lpmInfo->lpBaseOfDll;
		PLOG_INFO << "EXE Loaded";
		return defaultModule;
	}
	
	return defaultModule;
}


DWORD64 Pattern::Scan(const wchar_t* szModule, const std::span<const int> sPattern)
{
	auto lpmInfo = GetModuleInfo(szModule);
	DWORD64 base = (DWORD64)lpmInfo->lpBaseOfDll;
	DWORD64 sizeOfImage = (DWORD64)lpmInfo->SizeOfImage;

	unsigned int patternLength = sPattern.size();
	for (DWORD64 i = 0; i < sizeOfImage - patternLength; i++)
	{
		bool found = true;
		for (DWORD64 j = 0; j < patternLength && found; j++)
		{
			char b = *(char*)(base + i + j);
			found &= sPattern[j] == -1 || static_cast<char>(sPattern[j]) == b;
		}
		if (found)
		{
			return base + i;
		}
	}
	return NULL;
}

DWORD64 Pattern::Scan(const DWORD64 base, const DWORD sizeOfImage, const std::span<const int> sPattern)
{
	unsigned int patternLength = sPattern.size();
	for (DWORD64 i = 0; i < sizeOfImage - patternLength; i++)
	{
		bool found = true;
		for (DWORD64 j = 0; j < patternLength && found; j++)
		{
			char b = *(char*)(base + i + j);
			found &= sPattern[j] == -1 || static_cast<char>(sPattern[j]) == b;
		}
		if (found)
		{
			return base + i;
		}
	}
	return NULL;
}

DWORD64 Pattern::ScanRef(const wchar_t* szModule, const std::span<const int> sPattern, int32_t relAddressOffset, int32_t nextInstruction)
{
	auto lpmInfo = GetModuleInfo(szModule);
	DWORD64 base = (DWORD64)lpmInfo->lpBaseOfDll;
	DWORD64 sizeOfImage = (DWORD64)lpmInfo->SizeOfImage;

	unsigned int patternLength = sPattern.size();

	for (DWORD64 i = 0; i < sizeOfImage - patternLength; i++)
	{
		bool found = true;
		for (DWORD64 j = 0; j < patternLength && found; j++)
		{
			char b = *(char*)(base + i + j);
			found &= sPattern[j] == -1 || static_cast<char>(sPattern[j]) == b;
		}
		if (found)
		{
			//generally the size of what your looking for is a dword. relative addr to a func/variable.
			const int32_t relativeAddress = *(int32_t*)(base + i + relAddressOffset);
			return base + i + nextInstruction + relativeAddress;
		}
	}
	return NULL;
}

DWORD64 Pattern::ScanRef(const DWORD64 base, const DWORD sizeOfImage, const std::span<const int> sPattern, int32_t relAddressOffset, int32_t nextInstruction)
{
	unsigned int patternLength = sPattern.size();

	for (DWORD64 i = 0; i < sizeOfImage - patternLength; i++)
	{
		bool found = true;
		for (DWORD64 j = 0; j < patternLength && found; j++)
		{
			char b = *(char*)(base + i + j);
			found &= sPattern[j] == -1 || static_cast<char>(sPattern[j]) == b;
		}
		if (found)
		{
			//generally the size of what your looking for is a dword. relative addr to a func/variable.
			const int32_t relativeAddress = *(int32_t*)(base + i + relAddressOffset);
			return base + i + nextInstruction + relativeAddress;
		}
	}
	return NULL;
}
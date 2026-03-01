#pragma once
#include <Windows.h>
#include <cstdint>
#include <Psapi.h>
#include <span>
#include <plog/Log.h>
#include "stb.h"

// Scanner impl
// https://www.unknowncheats.me/forum/general-programming-and-reversing/502738-ida-style-pattern-scanner.html
#define FUNC_DEFCONV(ret, conv, name, args) \
	typedef ret (conv *name##_t)##args##; \
	extern __declspec(selectany) name##_t name##;

#define FUNC_DEF(ret, name, args) FUNC_DEFCONV(ret, __fastcall, name, args)

#define VAR_DEF(type, name) extern __declspec(selectany) type* name##;

#define PATTERN_SETMODULE(mod) Pattern::defaultModule = mod;
#define PATTERN_SETMODULESIZE(size) Pattern::defaultModuleSize = size;

#define PATTERN_SCAN(name, signature) \
	{ \
		constexpr auto _patternBytes = stb::compiletime_string_to_byte_array_data::getter<##signature##>::value; \
		name = reinterpret_cast<decltype(##name##)>(Pattern::Scan(Pattern::defaultModule, Pattern::defaultModuleSize, _patternBytes)); \
		if (name) { \
			PLOG_DEBUG << "Found " << #name << " at " << (void*)(reinterpret_cast<uintptr_t>(name) - Pattern::defaultModule); \
		} \
		else { \
			PLOG_ERROR << "Pattern for " << #name << " was not found"; \
		} \
	}


#define PATTERN_SCAN_OLD(name, signature) \
	{ \
	constexpr auto _patternBytes = stb::compiletime_string_to_byte_array_data::getter<##signature##>::value; \
	name = reinterpret_cast<decltype(##name##)>(Pattern::Scan(Pattern::defaultModule, _patternBytes)); \
	if (name) { \
		PLOG_DEBUG << "Found " << #name << " at " << (void*)(reinterpret_cast<uintptr_t>(name) - Pattern::BaseAddress(Pattern::defaultModule)); \
	} \
	else { \
		PLOG_ERROR << "Pattern for " << #name << " was not found"; \
	} \
}


#define PATTERN_SCANREF(name, signature, relAddressOffset, nextInstruction) \
	{ \
		constexpr auto _patternBytes = stb::compiletime_string_to_byte_array_data::getter<##signature##>::value; \
		name = reinterpret_cast<decltype(##name##)>(Pattern::ScanRef(Pattern::defaultModule, Pattern::defaultModuleSize, _patternBytes, relAddressOffset, nextInstruction)); \
		if (name) { \
			PLOG_DEBUG << "Found " << #name << " at " << (void*)(reinterpret_cast<uintptr_t>(name) - Pattern::defaultModule); \
		} \
		else { \
			PLOG_ERROR << "Pattern for " << #name << " was not found"; \
		} \
	}

#define PATTERN_SCANREF_OLD(name, signature, relAddressOffset, nextInstruction) \
{ \
	constexpr auto _patternBytes = stb::compiletime_string_to_byte_array_data::getter<##signature##>::value; \
	name = reinterpret_cast<decltype(##name##)>(Pattern::ScanRef(Pattern::defaultModule,  _patternBytes, relAddressOffset, nextInstruction)); \
	if (name) { \
		PLOG_DEBUG << "Found " << #name << " at " << (void*)(reinterpret_cast<uintptr_t>(name) - Pattern::BaseAddress(Pattern::defaultModule)); \
	} \
	else { \
		PLOG_ERROR << "Pattern for " << #name << " was not found"; \
	} \
}

class Pattern
{
public:
	static DWORD64 defaultModule;
	static DWORD defaultModuleSize;
	static DWORD64 BaseAddress(const wchar_t* szModule);
	static LPMODULEINFO GetModuleInfo(const wchar_t* szModule);
	static DWORD64 Scan(const wchar_t* szModule, const std::span<const int> sPattern);
	static DWORD64 Scan(DWORD64 base, DWORD sizeOfImage, const std::span<const int> sPattern);
	/*
	Extracts a relative address at an offset to the supplied pattern
	*/
	static DWORD64 ScanRef(const wchar_t* szModule, const std::span<const int> sPattern, int32_t relAddressOffset, int32_t nextInstruction);
	static DWORD64 ScanRef(DWORD64 base, DWORD sizeOfImage, const std::span<const int> sPattern, int32_t relAddressOffset, int32_t nextInstruction);
};


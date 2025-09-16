#include "HookUtil.h"
#include "Singleton.inl"

#include <Windows.h>
#include <plog/Log.h>

#include "detours/detours.h"

void HookManager::AddHook(void** func, void* detour, const char* funcName, const char* detourName)
{
	m_hooks.erase(func);
	m_hooks.try_emplace(func, func, detour, funcName, detourName);
}

void HookManager::RemoveHook(void** func)
{
	m_hooks.erase(func);
}

void HookManager::RemoveAllHooks()
{
	m_hooks.clear();
}

ScopedHook::ScopedHook(void** func, void* detour, const char* funcName, const char* detourName) : m_func(func), m_detour(detour)
{
	m_funcName = funcName ? funcName : std::to_string((uintptr_t)*func).c_str();
	m_detourName = detourName ? detourName : std::to_string((uintptr_t)detour).c_str();
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourAttach(func, detour);
	DetourTransactionCommit();

	PLOG_DEBUG << "Added detour for function " << funcName << " to " << detourName;
}

ScopedHook::~ScopedHook()
{
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourDetach(m_func, m_detour);
	DetourTransactionCommit();

	PLOG_DEBUG << "Removed detour for function " << m_funcName << " from " << m_detourName;
}

#pragma once
#include <unordered_map>

#include "Singleton.h"


// indirection necessary for macro expansion
#define CONCAT_(a, b) a##b
#define CONCAT(a, b) CONCAT_(a, b)

#define HOOK(func, detour) HookManager::GetInstance()->AddHook((void**)&(func), detour, #func, #detour);
#define UNHOOK(func) HookManager::GetInstance()->RemoveHook((void**)&(func));
#define HOOK_SCOPED(func, detour) ScopedHook CONCAT(_scoped_hook_, __COUNTER__)((void**)&(func), detour, #func, #detour);

struct [[nodiscard]] ScopedHook {
	ScopedHook(void** func, void* detour, const char* funcName = nullptr, const char* detourName = nullptr);
	~ScopedHook();

	ScopedHook& operator=(const ScopedHook&& other) const = delete;
	ScopedHook(ScopedHook&& other) = delete;

	ScopedHook& operator=(const ScopedHook& other) const = delete;
	ScopedHook(const ScopedHook& other) = delete;
private:
	void** m_func;
	void* m_detour;
	const char* m_funcName;
	const char* m_detourName;
};

class HookManager : public Singleton<HookManager> {
public:
	void AddHook(void** func, void* detour, const char* funcName = nullptr, const char* detourName = nullptr);
	void RemoveHook(void** func);
	void RemoveAllHooks();

protected:
	HookManager() = default;

private:
	std::unordered_map<void**, ScopedHook> m_hooks;
};

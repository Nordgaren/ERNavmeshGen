#pragma once
#include "../GhidraStructs/hkaiNavMeshGenerationSnapshot.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include <string.h>
using json = nlohmann::json;
namespace JsonHelpers 
{
    // Safely reads a primitive value (float, int, bool)
    template<typename T>
    void Read(const json& j, const char* key, T& outField) 
    {
        if (j.contains(key) && !j[key].is_null()) {
            outField = j[key].get<T>();
        }
    }

    // Safely reads enums by casting from int
    template<typename TEnum>
    void ReadEnum(const json& j, const char* key, TEnum& outField) 
    {
        if (j.contains(key) && !j[key].is_null()) {
            outField = static_cast<TEnum>(j[key].get<int>());
        }
    }

    // 1. Handles 'const char*' (for hkaiNavMeshGenerationUtilsSettings)
    void ReadString(const json& j, const char* key, const char*& outField) 
    {
        if (j.contains(key) && j[key].is_string()) {
            if (outField != nullptr) {
                free((void*)outField);
            }
            std::string val = j[key].get<std::string>();
            outField = _strdup(val.c_str());
        }
    }

    // 2. Handles standard 'char*' (for hkaiNavMeshSimplificationUtils)
    void ReadString(const json& j, const char* key, char*& outField) 
    {
        if (j.contains(key) && j[key].is_string()) {
            if (outField != nullptr) {
                free((void*)outField);
            }
            std::string val = j[key].get<std::string>();
            outField = _strdup(val.c_str());
        }
    }

    // Safely sets arrays to empty / DONT_DEALLOCATE (0x80000000)
    void ClearArray(hkArrayGeneric& arr) 
    {
        arr.data = nullptr;
        arr.size = 0;
        arr.capacityAndFlags = hkArrayFlags::DONT_DEALLOCATE; // 0x80000000
    }
}

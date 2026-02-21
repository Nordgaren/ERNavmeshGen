#pragma once
#include <windows.h>
#include <commdlg.h>
#include <shlobj.h>
#include <plog/Log.h>
#include <fstream>
#include <sstream>
#include <algorithm>

inline bool ends_with(std::string const& value, std::string const& ending)
{
	if (ending.size() > value.size()) return false;
	return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

inline bool ends_with(std::wstring const& value, std::wstring const& ending)
{
	if (ending.size() > value.size()) return false;
	return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}


namespace fileIO {
	std::vector<std::wstring> GetAllFilesInFolder(std::wstring folder) {
		WIN32_FIND_DATA data;
		HANDLE hFind;
		std::vector<std::wstring> Ret = {};
		std::wstring TempName = L"";
		std::wstring SearchFolder = L"";

		if (!ends_with(folder, L"\\")) {
			folder = folder + L"\\";
		}

		SearchFolder = folder + L"*";

		if ((hFind = FindFirstFile((LPCWSTR)SearchFolder.c_str(), &data)) != INVALID_HANDLE_VALUE) {
			do {
				TempName = folder + data.cFileName;
				Ret.push_back(TempName);
			} while (FindNextFile(hFind, &data) != 0);
			FindClose(hFind);
		}
		return Ret;
	}
	std::vector<std::string> GetAllFilesInFolder(std::string folder) {
		WIN32_FIND_DATAA data;
		HANDLE hFind;
		std::vector<std::string> Ret = {};
		std::string TempName = "";
		std::string SearchFolder = "";

		if (!ends_with(folder, "\\")) {
			folder = folder + "\\";
		}

		SearchFolder = folder + "*";

		if ((hFind = FindFirstFileA((LPCSTR)SearchFolder.c_str(), &data)) != INVALID_HANDLE_VALUE) {
			do {
				TempName = folder + data.cFileName;
				Ret.push_back(TempName);
			} while (FindNextFileA(hFind, &data) != 0);
			FindClose(hFind);
		}
		return Ret;
	}
}
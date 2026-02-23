#pragma once
#include <windows.h>
#include <commdlg.h>
#include <fstream>
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
	inline std::vector<uint8_t> readFile(const std::string& filename) {
		// 1. Open the file in binary mode, and position the pointer at the end (std::ios::ate)
		std::ifstream file(filename, std::ios::binary | std::ios::ate);

		if (!file.is_open()) {
			throw std::runtime_error("Failed to open file: " + filename);
		}

		// 2. Get the file size and seek back to the beginning
		std::streamsize size = file.tellg();
		file.seekg(0, std::ios::beg);

		// 3. Pre-allocate the vector to the exact file size
		std::vector<uint8_t> buffer(size);

		// 4. Read the data into the vector
		// We must cast the uint8_t* to char* because std::ifstream::read expects a char*
		if (file.read(reinterpret_cast<char*>(buffer.data()), size)) {
			return buffer;
		} else {
			throw std::runtime_error("Failed to read the complete file: " + filename);
		}
	}
	
	inline void writeFile(const std::string& filename, const std::vector<uint8_t>& data) {
		// 1. Open the file in binary mode for writing
		// Note: This will overwrite an existing file. If you want to append, 
		// add `| std::ios::app` to the open mode.
		std::ofstream file(filename, std::ios::binary);

		if (!file.is_open()) {
			throw std::runtime_error("Failed to open file for writing: " + filename);
		}

		// 2. Write the data in one go
		// We must cast the const uint8_t* to const char* because std::ofstream::write expects a const char*
		if (!file.write(reinterpret_cast<const char*>(data.data()), data.size())) {
			throw std::runtime_error("Failed to write data to file: " + filename);
		}
    
		// 3. File closes automatically when the std::ofstream object goes out of scope,
		// but you can explicitly call file.close() if you need to check for closing errors.
	}
	
	inline std::vector<std::wstring> GetAllFilesInFolder(std::wstring folder) {
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
	inline std::vector<std::string> GetAllFilesInFolder(std::string folder) {
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
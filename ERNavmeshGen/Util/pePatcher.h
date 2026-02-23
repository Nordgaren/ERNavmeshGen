#pragma once
#include <Zydis/Zydis.h>

#include "FunctionContext.h"
#include "InstructionCursor.h"
#include <windows.h>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <Zydis/Zydis.h>
#include <keystone/keystone.h>

class PEPatcher
{
public:
    std::vector<uint8_t> buffer;
    uint64_t imageBase = 0;
    PIMAGE_NT_HEADERS64 ntHeaders = nullptr;

    ZydisDecoder decoder;
    ks_engine* ks = nullptr;

    // --- 1. INITIALIZATION ---
    bool Load(const std::string& filepath)
    {
        // Read file into buffer
        std::ifstream file(filepath, std::ios::binary | std::ios::ate);
        if (!file) return false;
        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);
        buffer.resize(size);
        if (!file.read((char*)buffer.data(), size)) return false;

        // Parse basic PE Headers
        PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)buffer.data();
        if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE) return false;
        ntHeaders = (PIMAGE_NT_HEADERS64)(buffer.data() + dosHeader->e_lfanew);
        imageBase = ntHeaders->OptionalHeader.ImageBase;

        // Initialize Zydis (Disassembler)
        ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_STACK_WIDTH_64);

        // Initialize Keystone (Assembler)
        ks_err err = ks_open(KS_ARCH_X86, KS_MODE_64, &ks);
        if (err != KS_ERR_OK)
        {
            PLOG_ERROR << "[-] Failed to initialize Keystone: " << ks_strerror(err) << "\n";
            return false;
        }

        return true;
    }

    ~PEPatcher()
    {
        if (ks) ks_close(ks);
    }

    // --- 2. PE MATH ---
    uint32_t RvaToFileOffset(uint32_t rva)
    {
        PIMAGE_SECTION_HEADER section = IMAGE_FIRST_SECTION(ntHeaders);
        for (WORD i = 0; i < ntHeaders->FileHeader.NumberOfSections; i++, section++)
        {
            uint32_t size = section->VirtualAddress + section->Misc.VirtualSize;
            if (rva >= section->VirtualAddress && rva < size)
            {
                return rva - section->VirtualAddress + section->PointerToRawData;
            }
        }
        return 0;
    }

    // --- 3. THE KEYSTONE ASSEMBLER WRAPPER ---
    std::vector<uint8_t> Assemble(const std::string& asmCode, uint64_t runtimeAddress)
    {
        unsigned char* encode;
        size_t size;
        size_t count;

        // ks_asm takes your string ("jmp 0x..."), the absolute memory address where 
        // the instruction will live, and spits out the raw x64 bytes.
        if (ks_asm(ks, asmCode.c_str(), runtimeAddress, &encode, &size, &count) == KS_ERR_OK)
        {
            std::vector<uint8_t> machineCode(encode, encode + size);
            ks_free(encode); // Keystone allocates memory, we must free it
            return machineCode;
        }

        PLOG_ERROR << "[-] Keystone failed to assemble: " << asmCode << "\n";
        return {};
    }

    // --- 4. THE PATCH WRITER ---
    bool WritePatch(uint32_t targetRva, const std::string& asmCode)
    {
        // 1. Convert the destination RVA to an absolute memory address
        uint64_t runtimeAddress = imageBase + targetRva;

        // 2. Compile the string into raw bytes
        std::vector<uint8_t> machineCode = Assemble(asmCode, runtimeAddress);
        if (machineCode.empty()) return false;

        // 3. Find where this is physically located in our file buffer
        uint32_t fileOffset = RvaToFileOffset(targetRva);
        if (fileOffset == 0) return false;

        // 4. Overwrite the file buffer with our new bytes
        memcpy(&buffer[fileOffset], machineCode.data(), machineCode.size());
        return true;
    }

    // --- 5. SAVE EXECUTABLE ---
    bool Save(const std::string& outPath)
    {
        std::ofstream outFile(outPath, std::ios::binary);
        if (!outFile) return false;
        outFile.write((char*)buffer.data(), buffer.size());
        return true;
    }
};


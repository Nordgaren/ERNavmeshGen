#pragma once
#include <Zydis/Zydis.h>
#include <vector>
#include <iostream>

class InstructionCursor {
private:
    std::vector<uint8_t>& peBuffer;
    uint64_t imageBase;
    ZydisDecoder decoder;
    
    // Function to translate RVA to file offset (you already have this)
    uint32_t RvaToFileOffset(uint32_t rva) {
        
        PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)peBuffer.data();
        if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
            std::cerr << "[-] Invalid DOS signature.\n";
            return false;
        }

        PIMAGE_NT_HEADERS64 ntHeaders = (PIMAGE_NT_HEADERS64)(peBuffer.data() + dosHeader->e_lfanew);
        if (ntHeaders->Signature != IMAGE_NT_SIGNATURE) {
            std::cerr << "[-] Invalid NT signature.\n";
            return false;
        }
        
        PIMAGE_SECTION_HEADER section = IMAGE_FIRST_SECTION(ntHeaders);
        for (WORD i = 0; i < ntHeaders->FileHeader.NumberOfSections; i++, section++) {
            uint32_t size = section->VirtualAddress + section->Misc.VirtualSize;
            if (rva >= section->VirtualAddress && rva < size) {
                return rva - section->VirtualAddress + section->PointerToRawData;
            }
        }
        return 0; 
    }

public:
    uint32_t rva;                     // Current location
    ZydisDecodedInstruction instr;    // Current instruction details
    ZydisDecodedOperand operands[ZYDIS_MAX_OPERAND_COUNT];

    InstructionCursor(std::vector<uint8_t>& buffer, uint64_t base, uint32_t startRva) 
        : peBuffer(buffer), imageBase(base), rva(startRva) {
        ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_STACK_WIDTH_64);
        DecodeCurrent(); // Decode the first instruction immediately
    }

    // Decode the instruction at the current RVA
    bool DecodeCurrent() {
        uint32_t fileOffset = RvaToFileOffset(rva);
        return ZYAN_SUCCESS(ZydisDecoderDecodeFull(&decoder, 
                                                   peBuffer.data() + fileOffset, 
                                                   peBuffer.size() - fileOffset, 
                                                   &instr, operands));
    }

    // Step to the very next instruction
    bool Next() {
        rva += instr.length;
        return DecodeCurrent();
    }
    
    // Returns a copy of the cursor advanced by one instruction
    InstructionCursor Peek() const {
        InstructionCursor lookahead = *this; // Copy the current state
        lookahead.Next();                    // Advance only the copy
        return lookahead;                    // Return the copy
    }

    // Search forward for a specific mnemonic (e.g., ZYDIS_MNEMONIC_CALL)
    // We add a maxBytes limit so it doesn't search the whole 50MB file if it's missing
    bool FindNextMnemonic(ZydisMnemonic mnemonic, uint32_t maxBytes = 0x1000) {
        uint32_t startRva = rva;
        while ((rva - startRva) < maxBytes) {
            if (!Next()) return false; // End of file or decoding error
            if (instr.mnemonic == mnemonic) return true;
        }
        return false;
    }

    // Helper: Safely calculate absolute addresses for RIP-relative operands
    uint64_t GetAbsoluteAddress(int operandIndex) {
        uint64_t targetAddr = 0;
        uint64_t runtimeAddr = imageBase + rva;
        ZydisCalcAbsoluteAddress(&instr, &operands[operandIndex], runtimeAddr, &targetAddr);
        return targetAddr;
    }

    void SetRVA(uint32_t rva)
    {
        this->rva = rva;
        this->DecodeCurrent();
    }
};
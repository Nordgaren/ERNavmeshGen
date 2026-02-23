#pragma once
#include "InstructionCursor.h"
#include <vector>
#include <algorithm>

struct PrologueContext
{
    struct SavedReg
    {
        ZydisRegister reg;
        int32_t prologueOffset;
        uint32_t stackDepthAtSave;
    };

    uint32_t finalStackDepth = 0; // Total frame size (Push + Sub) for your MOV math
    uint32_t allocatedStackSpace = 0; // Space allocated ONLY by SUB RSP
    std::vector<ZydisRegister> pushedRegs;
    std::vector<SavedReg> savedRegs;

    static PrologueContext AnalyzePrologue(InstructionCursor cursor)
    {
        PrologueContext ctx;
        uint32_t currentDepth = 0;

        for (int i = 0; i < 20; i++)
        {
            if (!cursor.DecodeCurrent()) break;

            if (cursor.instr.mnemonic == ZYDIS_MNEMONIC_PUSH)
            {
                ctx.pushedRegs.push_back(cursor.operands[0].reg.value);
                currentDepth += 8;
            }
            else if (cursor.instr.mnemonic == ZYDIS_MNEMONIC_SUB &&
                cursor.operands[0].reg.value == ZYDIS_REGISTER_RSP)
            {
                uint32_t subSize = (uint32_t)cursor.operands[1].imm.value.u;
                currentDepth += subSize;
                ctx.allocatedStackSpace += subSize; // Track this separately!
            }
            else if ((cursor.instr.mnemonic == ZYDIS_MNEMONIC_MOV ||
                    cursor.instr.mnemonic == ZYDIS_MNEMONIC_MOVAPS ||
                    cursor.instr.mnemonic == ZYDIS_MNEMONIC_MOVUPS ||
                    cursor.instr.mnemonic == ZYDIS_MNEMONIC_MOVAPD ||
                    cursor.instr.mnemonic == ZYDIS_MNEMONIC_MOVDQA ||
                    cursor.instr.mnemonic == ZYDIS_MNEMONIC_MOVDQU) &&
                cursor.operands[0].type == ZYDIS_OPERAND_TYPE_MEMORY &&
                cursor.operands[0].mem.base == ZYDIS_REGISTER_RSP &&
                cursor.operands[1].type == ZYDIS_OPERAND_TYPE_REGISTER)
            {
                ctx.savedRegs.push_back({
                    cursor.operands[1].reg.value,
                    (int32_t)cursor.operands[0].mem.disp.value,
                    currentDepth
                });
            }
            else if (cursor.instr.mnemonic == ZYDIS_MNEMONIC_CALL || cursor.instr.mnemonic == ZYDIS_MNEMONIC_JMP) break;

            cursor.Next();
        }

        ctx.finalStackDepth = currentDepth;
        return ctx;
    }

    // Changed return type to uint32_t (returns 0 if not found)
    static uint32_t FindDynamicEpilogue(InstructionCursor cursor, const PrologueContext& ctx)
    {
        PLOG_INFO << "[*] Searching for dynamic epilogue...\n";

        // Calculate expected stack shrink size upfront
        uint32_t expectedStackAdd = ctx.finalStackDepth - (ctx.pushedRegs.size() * 8);

        while (cursor.Next())
        {
            uint32_t potentialEpilogueRva = cursor.rva;
            InstructionCursor checkCursor = cursor;

            // The sequence MUST begin with an epilogue-related instruction.
            bool isValidStart = false;
            if (!ctx.savedRegs.empty())
            {
                // Must start with a stack restore instruction
                if ((checkCursor.instr.mnemonic == ZYDIS_MNEMONIC_MOV ||
                        checkCursor.instr.mnemonic == ZYDIS_MNEMONIC_MOVAPS ||
                        checkCursor.instr.mnemonic == ZYDIS_MNEMONIC_MOVUPS ||
                        checkCursor.instr.mnemonic == ZYDIS_MNEMONIC_MOVAPD ||
                        checkCursor.instr.mnemonic == ZYDIS_MNEMONIC_MOVDQA ||
                        checkCursor.instr.mnemonic == ZYDIS_MNEMONIC_MOVDQU) &&
                    checkCursor.operands[1].type == ZYDIS_OPERAND_TYPE_MEMORY &&
                    checkCursor.operands[1].mem.base == ZYDIS_REGISTER_RSP)
                {
                    isValidStart = true;
                }
            }
            else if (expectedStackAdd > 0)
            {
                // If no saves, must start with ADD RSP
                if (checkCursor.instr.mnemonic == ZYDIS_MNEMONIC_ADD &&
                    checkCursor.operands[0].reg.value == ZYDIS_REGISTER_RSP)
                {
                    isValidStart = true;
                }
            }
            else if (!ctx.pushedRegs.empty())
            {
                // If no saves/adds, must start with POP
                if (checkCursor.instr.mnemonic == ZYDIS_MNEMONIC_POP) isValidStart = true;
            }
            else
            {
                // Empty function frame? Must be RET
                if (checkCursor.instr.mnemonic == ZYDIS_MNEMONIC_RET) isValidStart = true;
            }

            if (!isValidStart) continue; // Instantly reject without sliding the window

            std::vector<SavedReg> pendingRestores = ctx.savedRegs;
            bool badSequence = false;

            for (int step = 0; step < 10; ++step)
            {
                // THE FIX: Break out of Phase 1 ONLY when we reach the exact boundary of the next expected phase
                if (ctx.allocatedStackSpace > 0)
                {
                    if (checkCursor.instr.mnemonic == ZYDIS_MNEMONIC_ADD &&
                        checkCursor.operands[0].reg.value == ZYDIS_REGISTER_RSP)
                        break;
                }
                else if (!ctx.pushedRegs.empty())
                {
                    if (checkCursor.instr.mnemonic == ZYDIS_MNEMONIC_POP) break;
                }
                else
                {
                    if (checkCursor.instr.mnemonic == ZYDIS_MNEMONIC_RET) break;
                }

                // Is it a stack restore? (MOV, MOVAPS, etc.)
                if ((checkCursor.instr.mnemonic == ZYDIS_MNEMONIC_MOV ||
                        checkCursor.instr.mnemonic == ZYDIS_MNEMONIC_MOVAPS ||
                        checkCursor.instr.mnemonic == ZYDIS_MNEMONIC_MOVUPS ||
                        checkCursor.instr.mnemonic == ZYDIS_MNEMONIC_MOVAPD ||
                        checkCursor.instr.mnemonic == ZYDIS_MNEMONIC_MOVDQA ||
                        checkCursor.instr.mnemonic == ZYDIS_MNEMONIC_MOVDQU) &&
                    checkCursor.operands[1].type == ZYDIS_OPERAND_TYPE_MEMORY &&
                    checkCursor.operands[1].mem.base == ZYDIS_REGISTER_RSP)
                {
                    ZydisRegister foundReg = checkCursor.operands[0].reg.value;
                    int32_t foundOffset = (int32_t)checkCursor.operands[1].mem.disp.value;

                    auto it = std::find_if(pendingRestores.begin(), pendingRestores.end(), [&](const SavedReg& r)
                    {
                        int32_t expectedOffset = r.prologueOffset + (ctx.finalStackDepth - r.stackDepthAtSave);
                        return r.reg == foundReg && expectedOffset == foundOffset;
                    });

                    if (it != pendingRestores.end())
                    {
                        pendingRestores.erase(it);
                    }
                    else
                    {
                        badSequence = true;
                        break; // Invalid RSP read
                    }
                }
                else if (checkCursor.instr.mnemonic == ZYDIS_MNEMONIC_RET ||
                    checkCursor.instr.mnemonic == ZYDIS_MNEMONIC_CALL ||
                    checkCursor.instr.mnemonic == ZYDIS_MNEMONIC_JMP)
                {
                    badSequence = true;
                    break; // Control flow means this isn't an epilogue
                }

                checkCursor.Next();
            }

            if (badSequence || !pendingRestores.empty()) continue;

            // --- PHASE 2: Verify ADD RSP ---
            // Using your new allocatedStackSpace variable makes this super clean!
            if (ctx.allocatedStackSpace > 0)
            {
                if (checkCursor.instr.mnemonic != ZYDIS_MNEMONIC_ADD ||
                    checkCursor.operands[0].reg.value != ZYDIS_REGISTER_RSP ||
                    checkCursor.operands[1].imm.value.u != ctx.allocatedStackSpace)
                {
                    continue;
                }
                checkCursor.Next();
            }
            // --- PHASE 3: Verify POPs ---
            bool popsMatch = true;
            for (auto it = ctx.pushedRegs.rbegin(); it != ctx.pushedRegs.rend(); ++it)
            {
                if (checkCursor.instr.mnemonic != ZYDIS_MNEMONIC_POP ||
                    checkCursor.operands[0].reg.value != *it)
                {
                    popsMatch = false;
                    break;
                }
                checkCursor.Next();
            }
            if (!popsMatch) continue;

            // --- PHASE 4: Verify RET ---
            if (checkCursor.instr.mnemonic == ZYDIS_MNEMONIC_RET)
            {
                return potentialEpilogueRva;
            }
        }

        return 0; // Not found
    }
};

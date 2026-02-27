#pragma once
#include <plog/Log.h>
#include "Util/PEPatcher.h"

namespace pePatcher
{
    static uint32_t AppendToTextSection(PEPatcher& patcher, uint32_t requiredSize)
    {
        PIMAGE_SECTION_HEADER section = IMAGE_FIRST_SECTION(patcher.ntHeaders);

        for (WORD i = 0; i < patcher.ntHeaders->FileHeader.NumberOfSections; i++, section++)
        {
            // Find the .text section
            if (strncmp((char*)section->Name, ".text", 5) == 0)
            {
                // Grab the RVA exactly at the end of the current virtual code
                uint32_t targetRva = section->VirtualAddress + section->Misc.VirtualSize;

                // Align it to 16 bytes (0x10) so the CPU executes the assembly cleanly
                uint32_t alignedRva = (targetRva + 0xF) & ~0xF;

                // Expand the VirtualSize to encompass our new stub
                uint32_t currentVirtualEnd = section->VirtualAddress + section->Misc.VirtualSize;
                uint32_t paddingBytes = alignedRva - currentVirtualEnd;
                uint32_t totalBytesToAdd = paddingBytes + requiredSize;
                section->Misc.VirtualSize += totalBytesToAdd;

                // SizeOfRawData should round up to the nearest FileAlignment
                uint32_t fileAlign = patcher.ntHeaders->OptionalHeader.FileAlignment;
                section->SizeOfRawData = (section->Misc.VirtualSize + fileAlign - 1) & ~(fileAlign - 1);

                // Fill the space we claimed with INT 3 before we write our actual payload into it
                uint32_t offset = patcher.RvaToFileOffset(alignedRva);
                memset(&patcher.buffer[offset], 0xCC, requiredSize);

                return alignedRva;
            }
        }
        return 0;
    }

    static bool InjectDllMainStub(PEPatcher& patcher)
    {
        PLOG_INFO << "Injecting Custom DllMain Stub";

        // Add space at the end of the .text section
        uint32_t stubRva = AppendToTextSection(patcher, 0x40);
        if (stubRva == 0)
        {
            PLOG_ERROR << "[-] Failed to add to the end of .text section!";
            return false;
        }
        PLOG_INFO << "[+] Allocated Code Cave at RVA: 0x" << std::hex << stubRva;

        // Fetch the Original Entry Point
        uint32_t origEpRva = patcher.ntHeaders->OptionalHeader.AddressOfEntryPoint;
        uint64_t absOrigEp = patcher.imageBase + origEpRva;

        // Write the DllMain assembly logic
        // EDX holds fdwReason. DLL_PROCESS_ATTACH is 1.
        std::stringstream asmStream;
        asmStream << "cmp edx, 1;\n" // Is it DLL_PROCESS_ATTACH?
            << "jne skip;\n"
            << "sub rsp, 0x28;\n"
            << "call 0x" << std::hex << absOrigEp << ";\n" // Call the original entry point
            << "add rsp, 0x28;\n" // Clean up the shadow space
            << "skip:\n"
            << "mov eax, 1;\n" // Return TRUE
            << "ret;\n";

        PLOG_INFO << "[*] Assembling Stub: " << asmStream.str();

        // Compile and write the patch
        if (!patcher.WritePatch(stubRva, asmStream.str()))
        {
            PLOG_ERROR << "[-] Failed to write stub to code cave.";
            return false;
        }

        // Hijack the entry point
        patcher.ntHeaders->OptionalHeader.AddressOfEntryPoint = stubRva;
        PLOG_INFO << "[+] Hijacked PE Entry Point to point to new DllMain Stub!";

        return true;
    }

    static bool InitFunctionBypasses(PEPatcher& patcher, uint32_t initEngineRva)
    {
        PLOG_INFO << "Engine Init Bypasses";

        InstructionCursor cursor(patcher.buffer, patcher.imageBase, initEngineRva);

        uint32_t targetJnzRva = 0;
        uint32_t epilogueLandingRva = 0;

        // We will keep track of the most recent conditional jump we see.
        // Because the check is the very last singleton checked before the epilogue,
        // the last JNZ we see before the stack cookie check is guaranteed to be our target!
        uint32_t lastJnzRva = 0;

        PLOG_INFO << "[*] Scanning for last init block...";

        while (cursor.Next())
        {
            // Keep track of the last conditional jump (JNZ / JNE is opcode 0x75 or 0x0F 0x85)
            if (cursor.instr.mnemonic == ZYDIS_MNEMONIC_JNZ)
            {
                lastJnzRva = cursor.rva;
                // Also save where this jump lands, because that's the start of the epilogue
                epilogueLandingRva = cursor.GetAbsoluteAddress(0) - patcher.imageBase;
            }

            // Look for the Stack Cookie Check (XOR RCX, RSP followed by CALL)
            if (cursor.instr.mnemonic == ZYDIS_MNEMONIC_XOR &&
                cursor.operands[0].reg.value == ZYDIS_REGISTER_RCX &&
                cursor.operands[1].reg.value == ZYDIS_REGISTER_RSP)
            {
                InstructionCursor peek1 = cursor.Peek();
                if (peek1.instr.mnemonic == ZYDIS_MNEMONIC_CALL)
                {
                    // We found the end of the function!
                    // Therefore, the last JNZ we saw MUST be the EOS bypass jump.
                    targetJnzRva = lastJnzRva;
                    break;
                }
            }

            if (cursor.instr.mnemonic == ZYDIS_MNEMONIC_INT3)
            {
                PLOG_ERROR << "[-] Aborting: Could not find last JNZ instruction.";
                return false;
            }
        }

        PLOG_INFO << "[+] Found Bypass JNZ at RVA: 0x" << std::hex << targetJnzRva;
        PLOG_INFO << "[+] Verified Landing Zone at RVA: 0x" << std::hex << epilogueLandingRva;

        // Change `75 37` to `EB 37` (Jump short unconditionally).
        uint32_t fileOffset = patcher.RvaToFileOffset(targetJnzRva);

        // Safety check: Ensure the byte is actually 0x75 (JNZ) before we overwrite it
        if (patcher.buffer[fileOffset] != 0x75)
        {
            PLOG_ERROR << "[-] Unexpected opcode at target RVA. Aborting patch.";
            return false;
        }

        patcher.buffer[fileOffset] = 0xEB; // Overwrite with JMP
        PLOG_INFO << "[+] Successfully patched EOS Init check to always skip!";
        return true;
    }

    static bool InjectBypassJump(PEPatcher& patcher, InstructionCursor& cursor, uint32_t epilogueRva)
    {
        // Calculate the exact Patch Location
        uint32_t patchLocationRva = cursor.rva + cursor.instr.length;

        // Calculate the Absolute Target Address for the Jump
        uint64_t absoluteTargetAddress = patcher.imageBase + epilogueRva;

        // Format the Assembly String
        std::stringstream asmStream;
        asmStream << "jmp 0x" << std::hex << absoluteTargetAddress;
        std::string asmCode = asmStream.str();

        PLOG_INFO << "[*] Compiling Assembly: " << asmCode;

        // Assemble and Write
        return patcher.WritePatch(patchLocationRva, asmCode);
    }

    static bool BypassCSWindowInit(PEPatcher& patcher, uint32_t initEngineRva)
    {
        PLOG_INFO << "[***] Processing Engine Init (Bypassing CSWindow) [***]";

        InstructionCursor cursor(patcher.buffer, patcher.imageBase, initEngineRva);

        uint32_t patchLocationRva = 0;
        uint32_t landingZoneRva = 0;

        PLOG_INFO << "[*] Scanning for CSWindow init block...";

        while (cursor.Next())
        {
            // 1. Look for: MOV EAX, [mem] (Fetching WINDOW_HEIGHT)
            if (cursor.instr.mnemonic == ZYDIS_MNEMONIC_MOV &&
                cursor.operands[0].type == ZYDIS_OPERAND_TYPE_REGISTER &&
                cursor.operands[0].reg.value == ZYDIS_REGISTER_EAX &&
                cursor.operands[1].type == ZYDIS_OPERAND_TYPE_MEMORY)
            {
                // 2. Peek 1: MOV [mem], EAX (Saving to stack)
                InstructionCursor peek1 = cursor.Peek();
                if (peek1.instr.mnemonic == ZYDIS_MNEMONIC_MOV &&
                    peek1.operands[0].type == ZYDIS_OPERAND_TYPE_MEMORY &&
                    peek1.operands[1].type == ZYDIS_OPERAND_TYPE_REGISTER &&
                    peek1.operands[1].reg.value == ZYDIS_REGISTER_EAX)
                {
                    // 3. Peek 2: MOV RAX, [mem] (Fetching INS_CSWindowImp) -> THIS IS OUR PATCH LOCATION!
                    InstructionCursor peek2 = peek1.Peek();
                    if (peek2.instr.mnemonic == ZYDIS_MNEMONIC_MOV &&
                        peek2.operands[0].type == ZYDIS_OPERAND_TYPE_REGISTER &&
                        peek2.operands[0].reg.value == ZYDIS_REGISTER_RAX &&
                        peek2.operands[1].type == ZYDIS_OPERAND_TYPE_MEMORY)
                    {
                        // 4. Peek 3: TEST RAX, RAX
                        InstructionCursor peek3 = peek2.Peek();
                        if (peek3.instr.mnemonic == ZYDIS_MNEMONIC_TEST &&
                            peek3.operands[0].reg.value == ZYDIS_REGISTER_RAX)
                        {
                            // 5. Peek 4: JNZ (The jump to the post-init block)
                            InstructionCursor peek4 = peek3.Peek();
                            if (peek4.instr.mnemonic == ZYDIS_MNEMONIC_JNZ)
                            {
                                patchLocationRva = peek2.rva; // We will overwrite the MOV RAX instruction

                                // Extract where the original code skips to if the window already exists
                                uint32_t postInitRva = peek4.GetAbsoluteAddress(0) - patcher.imageBase;

                                PLOG_INFO << "[+] Found CSWindow block at RVA: 0x" << std::hex << patchLocationRva;
                                PLOG_INFO << "[*] Original Post-Init lands at RVA: 0x" << postInitRva;

                                // --- Find the Safe Landing Zone ---
                                // Drop a cursor at the post-init, and step past the CALL
                                InstructionCursor landingCursor(patcher.buffer, patcher.imageBase, postInitRva);
                                while (landingCursor.Next())
                                {
                                    if (landingCursor.instr.mnemonic == ZYDIS_MNEMONIC_CALL)
                                    {
                                        landingCursor.Next(); // Step past the CALL instruction
                                        landingZoneRva = landingCursor.rva; // This is the safe zone!
                                        break;
                                    }
                                }
                                break; // We found everything, get out of the main loop
                            }
                        }
                    }
                }
            }

            if (cursor.instr.mnemonic == ZYDIS_MNEMONIC_INT3) break; // Safety net
        }

        if (patchLocationRva == 0 || landingZoneRva == 0)
        {
            PLOG_ERROR << "[-] Failed to find the CSWindow initialization block or landing zone.";
            return false;
        }

        PLOG_INFO << "[+] Calculated Safe Landing Zone at RVA: 0x" << std::hex << landingZoneRva;

        // --- Inject the Bypass ---
        uint64_t absoluteLandingTarget = patcher.imageBase + landingZoneRva;

        std::stringstream asmStream;
        asmStream << "jmp 0x" << std::hex << absoluteLandingTarget;

        PLOG_INFO << "[*] Patching CSWindow: " << asmStream.str();
        patcher.WritePatch(patchLocationRva, asmStream.str());

        // Optional: Pad the remaining 2 bytes of the original 7-byte MOV instruction with NOPs 
        // so disassemblers like Ghidra/IDA don't get confused by the dead code fracture.
        uint32_t fileOffset = patcher.RvaToFileOffset(patchLocationRva + 5); // jmp rel32 is 5 bytes
        patcher.buffer[fileOffset] = 0x90;
        patcher.buffer[fileOffset + 1] = 0x90;

        PLOG_INFO << "[+] CSWindow Init successfully bypassed!";
        return true;
    }

    static uint32_t FindCookieEpilogue(InstructionCursor cursor)
    {
        uint32_t lastMovRva = 0;

        while (cursor.Next())
        {
            // Track the instruction right before the cookie check (usually MOV RCX, [RBP+...])
            if (cursor.instr.mnemonic == ZYDIS_MNEMONIC_MOV)
            {
                lastMovRva = cursor.rva;
            }

            // Signature: XOR RCX, RSP
            if (cursor.instr.mnemonic == ZYDIS_MNEMONIC_XOR &&
                cursor.operands[0].reg.value == ZYDIS_REGISTER_RCX &&
                cursor.operands[1].reg.value == ZYDIS_REGISTER_RSP)
            {
                InstructionCursor peek = cursor.Peek();
                if (peek.instr.mnemonic == ZYDIS_MNEMONIC_CALL)
                {
                    // Return the RVA of the MOV right before the XOR
                    return lastMovRva;
                }
            }

            if (cursor.instr.mnemonic == ZYDIS_MNEMONIC_INT3) break;
        }
        return 0; // Not found
    }

    static bool AbortInitAtCSWindow(PEPatcher& patcher, uint32_t initEngineRva)
    {
        PLOG_INFO << "Processing Engine Init (Aborting at CSWindow)";

        InstructionCursor cursor(patcher.buffer, patcher.imageBase, initEngineRva);

        uint32_t epilogueRva = FindCookieEpilogue(cursor);
        if (epilogueRva == 0)
        {
            PLOG_ERROR << "[-] Failed to find InitEngine Cookie Epilogue.";
            return false;
        }

        PLOG_INFO << "[*] Scanning for CSWindow block...";

        // Now we only need to scan for the CSWindow signature
        while (cursor.Next())
        {
            if (cursor.instr.mnemonic == ZYDIS_MNEMONIC_MOV &&
                cursor.operands[0].type == ZYDIS_OPERAND_TYPE_REGISTER &&
                cursor.operands[0].reg.value == ZYDIS_REGISTER_EAX &&
                cursor.operands[1].type == ZYDIS_OPERAND_TYPE_MEMORY)
            {
                // Peek 1: MOV [RBP-X], EAX
                InstructionCursor peek1 = cursor.Peek();
                if (peek1.instr.mnemonic == ZYDIS_MNEMONIC_MOV &&
                    peek1.operands[0].type == ZYDIS_OPERAND_TYPE_MEMORY &&
                    peek1.operands[1].type == ZYDIS_OPERAND_TYPE_REGISTER &&
                    peek1.operands[1].reg.value == ZYDIS_REGISTER_EAX)
                {
                    // Peek 2: MOV RAX, [INS_CSWindowImp] -> THIS IS OUR PATCH LOCATION!
                    InstructionCursor peek2 = peek1.Peek();
                    if (peek2.instr.mnemonic == ZYDIS_MNEMONIC_MOV &&
                        peek2.operands[0].type == ZYDIS_OPERAND_TYPE_REGISTER &&
                        peek2.operands[0].reg.value == ZYDIS_REGISTER_RAX &&
                        peek2.operands[1].type == ZYDIS_OPERAND_TYPE_MEMORY)
                    {
                        cursor.SetRVA(peek2.rva); // Found it!
                        break;
                    }
                }
            }
            if (cursor.instr.mnemonic == ZYDIS_MNEMONIC_INT3) {
                PLOG_ERROR << "[-] Failed to find the CSWindow block.";
                return false;
            };
        }

        InjectBypassJump(patcher, cursor, epilogueRva);

        // std::stringstream asmStream;
        // asmStream << "jmp 0x" << std::hex << absoluteEpilogueTarget;
        //
        // patcher.WritePatch(patchLocationRva, asmStream.str());
        //
        // // Pad the remaining 2 bytes of the fractured MOV
        // uint32_t fileOffset = patcher.RvaToFileOffset(patchLocationRva + 5);
        // patcher.buffer[fileOffset] = 0x90;
        // patcher.buffer[fileOffset + 1] = 0x90;

        PLOG_INFO << "[+] Init Engine successfully aborted at CSWindow!";
    }


    static bool PatchMutexCheck(PEPatcher& patcher, uint32_t winMainRva)
    {
        PLOG_INFO << "Processing Mutex Check Bypass";

        InstructionCursor cursor(patcher.buffer, patcher.imageBase, winMainRva);
        uint32_t targetFuncRva = 0;

        // In x64 Windows, wide strings (L"") are UTF-16LE (2 bytes per character).
        // We define the byte pattern for "Global\SekiroMutex" to search for.
        const uint8_t mutexStr[] = {
            'G', 0, 'l', 0, 'o', 0, 'b', 0, 'a', 0, 'l', 0, '\\', 0,
            'S', 0, 'e', 0, 'k', 0, 'i', 0, 'r', 0, 'o', 0, 'M', 0, 'u', 0, 't', 0, 'e', 0, 'x', 0
        };

        PLOG_INFO << "[*] Scanning WinMain for Mutex Initialization...";

        // Scan the first 20 instructions of WinMain to find its startup calls
        for (int i = 0; i < 20; ++i)
        {
            if (!cursor.Next()) break;

            if (cursor.instr.mnemonic == ZYDIS_MNEMONIC_CALL)
            {
                uint32_t callTargetRva = cursor.GetAbsoluteAddress(0) - patcher.imageBase;

                // Drop a temporary cursor into the called function
                InstructionCursor targetCursor(patcher.buffer, patcher.imageBase, callTargetRva);

                // Scan the first 30 instructions of this target function
                for (int j = 0; j < 30; ++j)
                {
                    if (!targetCursor.Next()) break;

                    // Look for LEA RCX, [RIP + disp] (Loading a string pointer)
                    if (targetCursor.instr.mnemonic == ZYDIS_MNEMONIC_LEA &&
                        targetCursor.operands[1].type == ZYDIS_OPERAND_TYPE_MEMORY &&
                        targetCursor.operands[1].mem.base == ZYDIS_REGISTER_RIP)
                    {
                        // Calculate exactly where this string lives in the file
                        uint32_t strRva = targetCursor.GetAbsoluteAddress(1) - patcher.imageBase;
                        uint32_t strOffset = patcher.RvaToFileOffset(strRva);

                        // Verify the string matches "Global\SekiroMutex"
                        if (strOffset > 0 && strOffset + sizeof(mutexStr) <= patcher.buffer.size())
                        {
                            if (memcmp(&patcher.buffer[strOffset], mutexStr, sizeof(mutexStr)) == 0)
                            {
                                targetFuncRva = callTargetRva; // We found the Mutex function!
                                break;
                            }
                        }
                    }

                    // Don't scan past the end of small functions
                    if (targetCursor.instr.mnemonic == ZYDIS_MNEMONIC_RET) break;
                }
            }

            if (targetFuncRva != 0) break; // Found it, stop scanning WinMain
        }

        if (targetFuncRva == 0)
        {
            PLOG_ERROR << "[-] Failed to locate IsGameAlreadyOpen via mutex string.";
            return false;
        }

        PLOG_INFO << "[+] Found IsGameAlreadyOpen at RVA: 0x" << std::hex << targetFuncRva;

        // We want the function to immediately return TRUE (AL = 1).
        // In x64, 'mov eax, 1' zero-extends to RAX, setting AL to 1.
        std::string asmCode = "mov eax, 1; ret;";
        PLOG_INFO << "[*] Patching IsGameAlreadyOpen: " << asmCode;

        patcher.WritePatch(targetFuncRva, asmCode);

        PLOG_INFO << "[+] Mutex check successfully neutralized!";
        return true;
    }

    static bool ApplyPatches(const std::string& inPath, const std::string& outPath)
    {
        PEPatcher patcher = PEPatcher();
        patcher.Load(inPath);

        PIMAGE_NT_HEADERS64 ntHeaders = patcher.ntHeaders;

        // Add IMAGE_FILE_DLL to Characteristics
        ntHeaders->FileHeader.Characteristics |= IMAGE_FILE_DLL;

        uint64_t imageBase = ntHeaders->OptionalHeader.ImageBase;
        uint32_t entryPointRva = ntHeaders->OptionalHeader.AddressOfEntryPoint;

        std::vector<uint8_t> peBuffer = patcher.buffer;

        /* Bypass startup decryption */
        // Drop the cursor at the PE Entry Point
        InstructionCursor cursor(peBuffer, imageBase, entryPointRva);

        // Find the first JMP (This jumps to __scrt_common_main_seh)
        if (!cursor.FindNextMnemonic(ZYDIS_MNEMONIC_JMP))
        {
            PLOG_ERROR << "[-] Failed to find initial JMP.";
            return false;
        }

        // Follow the JMP by updating the cursor's RVA
        cursor.SetRVA(cursor.GetAbsoluteAddress(0) - imageBase);

        /* CRT patch */
        // Analyze the prologue to get the context and find the epiogue
        InstructionCursor crtCursor(peBuffer, imageBase, cursor.rva);
        PrologueContext prologueCTX = PrologueContext::AnalyzePrologue(crtCursor);

        uint32_t epilogueRva = PrologueContext::FindDynamicEpilogue(crtCursor, prologueCTX);
        if (epilogueRva == 0)
        {
            PLOG_ERROR << "[-] Aborting: Could not find CRT epilogue.";
            return false;
        }

        // cursor.DecodeCurrent(); // Decode the first instruction of the CRT
        PLOG_INFO << "[+] Entered __scrt_common_main_seh at RVA: 0x" << std::hex << cursor.rva;

        // Search for LEA RCX, [ImageBase]
        while (cursor.FindNextMnemonic(ZYDIS_MNEMONIC_LEA))
        {
            // Is it LEA RCX?
            if (cursor.operands[0].type == ZYDIS_OPERAND_TYPE_REGISTER &&
                cursor.operands[0].reg.value == ZYDIS_REGISTER_RCX)
            {
                // Is it [ImageBase]?
                if (cursor.GetAbsoluteAddress(1) == imageBase)
                {
                    // THE LOOKAHEAD: Is the very next instruction a CALL?
                    InstructionCursor peeked = cursor.Peek();
                    if (peeked.instr.mnemonic == ZYDIS_MNEMONIC_CALL)
                    {
                        cursor.Next();
                        break; // We found the exact sequence we wanted!
                    }
                }
            }
        }

        if (!InjectBypassJump(patcher, cursor, epilogueRva))
        {
            PLOG_ERROR << "[-] Failed to inject CRT bypass jump.";
            return false;
        }

        /* WinMain patch */
        // Extract WinMain's RVA and drop a NEW cursor inside WinMain
        uint32_t winMainRva = cursor.GetAbsoluteAddress(0) - imageBase;
        PLOG_INFO << "[+] Found WinMain at RVA: 0x" << std::hex << winMainRva;

        InstructionCursor winMainCursor(peBuffer, imageBase, winMainRva);
        PrologueContext winMainCtx = PrologueContext::AnalyzePrologue(winMainCursor);
        uint32_t winMainEpilogueRva = PrologueContext::FindDynamicEpilogue(winMainCursor, winMainCtx);
        if (winMainEpilogueRva == 0)
        {
            PLOG_ERROR << "[-] Aborting: Could not find WinMain epilogue.";
            return false;
        }

        // Reset a new cursor back to the start of WinMain so we can search forward
        InstructionCursor searchCursor(peBuffer, imageBase, winMainRva);

        while (searchCursor.Next())
        {
            // Look for the setup: MOV EDX, 2 (Thread Priority argument)
            if (searchCursor.instr.mnemonic == ZYDIS_MNEMONIC_MOV &&
                searchCursor.operands[0].type == ZYDIS_OPERAND_TYPE_REGISTER &&
                searchCursor.operands[0].reg.value == ZYDIS_REGISTER_EDX &&
                searchCursor.operands[1].type == ZYDIS_OPERAND_TYPE_IMMEDIATE &&
                searchCursor.operands[1].imm.value.u == 0x2)
            {
                // Peek 1: CALL KERNEL32.DLL::SetThreadPriority
                InstructionCursor peek1 = searchCursor.Peek();
                if (peek1.instr.mnemonic == ZYDIS_MNEMONIC_CALL)
                {
                    // Peek 2: CALL MainLoop
                    InstructionCursor peek2 = peek1.Peek();
                    if (peek2.instr.mnemonic == ZYDIS_MNEMONIC_CALL)
                    {
                        searchCursor.SetRVA(peek2.rva);
                        break;
                    }
                }
            }

            if (searchCursor.instr.mnemonic == ZYDIS_MNEMONIC_INT3)
            {
                PLOG_ERROR << "[-] Aborting: Could not find WinMain epilogue.";
                return false;
            }
        }

        if (!InjectBypassJump(patcher, searchCursor, winMainEpilogueRva))
        {
            PLOG_ERROR << "[-] Failed to inject WinMain bypass jump.";
            return false;
        }

        /* MainLoop patch */
        uint32_t mainLoopTargetRva = searchCursor.GetAbsoluteAddress(0) - imageBase;
        PLOG_INFO << "[+] Found MainLoop at RVA: 0x" << std::hex << mainLoopTargetRva;

        // Analyze the prologue to get the stack delta and saved registers
        InstructionCursor mainLoopCursor(peBuffer, imageBase, mainLoopTargetRva);
        PrologueContext mainLoopPrologueCTX = PrologueContext::AnalyzePrologue(mainLoopCursor);
        uint32_t mainLoopEpilogueRVA = PrologueContext::FindDynamicEpilogue(mainLoopCursor, mainLoopPrologueCTX);

        if (mainLoopEpilogueRVA == 0)
        {
            PLOG_ERROR << "[-] Failed to find MainLoop Epilogue!";
            return false;
        }

        // Find the first CALL
        InstructionCursor mainLoopSearchCursor(peBuffer, imageBase, mainLoopTargetRva);
        // Start fresh from the top of MainLoop
        uint32_t patchLocationRva = 0;

        PLOG_INFO << "[*] Searching for first CALL in MainLoop...";

        while (mainLoopSearchCursor.Next())
        {
            if (mainLoopSearchCursor.instr.mnemonic == ZYDIS_MNEMONIC_CALL)
            {
                // The patch goes IMMEDIATELY after this CALL finishes
                patchLocationRva = mainLoopSearchCursor.rva + mainLoopSearchCursor.instr.length;

                PLOG_INFO << "[+] Found first CALL at RVA: 0x" << std::hex << mainLoopSearchCursor.rva;
                PLOG_INFO << "[*] Patch Location RVA: 0x" << patchLocationRva;
                break;
            }

            // Safety net: don't search forever if something goes horribly wrong
            if (mainLoopSearchCursor.instr.mnemonic == ZYDIS_MNEMONIC_INT3) break;
        }

        if (patchLocationRva == 0)
        {
            PLOG_ERROR << "[-] Failed to locate a CALL inside MainLoop.";
            return false;
        }

        if (!InjectBypassJump(patcher, mainLoopSearchCursor, mainLoopEpilogueRVA))
        {
            PLOG_ERROR << "[-] Failed to inject MainLoop bypass jump.";
            return false;
        }

        uint32_t initEngineRva = mainLoopSearchCursor.GetAbsoluteAddress(0) - imageBase;

        if (!PatchMutexCheck(patcher, winMainRva))
        {
            PLOG_ERROR << "[-] Failed to patch Mutex in WinMain patch.";
            return false;
        }


        if (!AbortInitAtCSWindow(patcher, initEngineRva))
        {
            PLOG_ERROR << "[-] Failed to bypass CSWindow init in init engine.";
            return false;
        }
        if (!InitFunctionBypasses(patcher, initEngineRva))
        {
            PLOG_ERROR << "[-] Failed to bypass functions in final function patch.";
            return false;
        }
        //
        // if (!BypassCSWindowInit(patcher, initEngineRva))
        // {
        //     PLOG_ERROR << "[-] Failed to bypass CS window init.";
        // }

        if (!InjectDllMainStub(patcher))
        {
            PLOG_ERROR << "[-] Failed to locate a CALL inside MainLoop.";
            return false;
        }

        // Save the patched binary
        patcher.Save(outPath);

        return true;
    }
}

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

    static bool AbortInitAtCSWindow(PEPatcher& patcher, uint32_t initEngineRva)
    {
        PLOG_INFO << "[***] Processing Engine Init (Aborting at CSWindow) [***]";

        InstructionCursor cursor(patcher.buffer, patcher.imageBase, initEngineRva);

        uint32_t patchLocationRva = 0;
        uint32_t epilogueRva = 0;
        uint32_t lastMovRva = 0; // Tracks the instruction immediately preceding the XOR

        PLOG_INFO << "[*] Scanning for CSWindow block and Epilogue...";

        while (cursor.Next())
        {
            // --- 1. Track the Epilogue ---
            // The epilogue starts with `MOV RCX, [RBP+0xD0]` right before the cookie check
            if (cursor.instr.mnemonic == ZYDIS_MNEMONIC_MOV)
            {
                lastMovRva = cursor.rva;
            }

            // Look for: XOR RCX, RSP followed by CALL (The Stack Cookie Check)
            if (epilogueRva == 0 &&
                cursor.instr.mnemonic == ZYDIS_MNEMONIC_XOR &&
                cursor.operands[0].reg.value == ZYDIS_REGISTER_RCX &&
                cursor.operands[1].reg.value == ZYDIS_REGISTER_RSP)
            {
                InstructionCursor peek = cursor.Peek();
                if (peek.instr.mnemonic == ZYDIS_MNEMONIC_CALL)
                {
                    // We found the end of the function! The epilogue starts at the last MOV.
                    epilogueRva = lastMovRva;
                }
            }

            // --- 2. Track the Patch Location ---
            // Look for: MOV EAX, [WINDOW_HEIGHT]
            if (patchLocationRva == 0 &&
                cursor.instr.mnemonic == ZYDIS_MNEMONIC_MOV &&
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
                        patchLocationRva = peek2.rva;
                    }
                }
            }

            // If we found both, we can stop scanning
            if (patchLocationRva != 0 && epilogueRva != 0) break;

            if (cursor.instr.mnemonic == ZYDIS_MNEMONIC_INT3) break; // Safety net
        }

        if (patchLocationRva == 0 || epilogueRva == 0)
        {
            PLOG_ERROR << "[-] Failed to find the CSWindow block or the Epilogue.";
            return false;
        }

        PLOG_INFO << "[+] Found CSWindow Patch Location at RVA: 0x" << std::hex << patchLocationRva;
        PLOG_INFO << "[+] Found Function Epilogue at RVA: 0x" << epilogueRva;

        // --- 3. Inject the Abort JMP ---
        uint64_t absoluteEpilogueTarget = patcher.imageBase + epilogueRva;

        std::stringstream asmStream;
        asmStream << "jmp 0x" << std::hex << absoluteEpilogueTarget;

        PLOG_INFO << "[*] Compiling Abort Patch: " << asmStream.str();
        patcher.WritePatch(patchLocationRva, asmStream.str());

        // Clean up the fractured bytes of the 7-byte MOV instruction
        uint32_t fileOffset = patcher.RvaToFileOffset(patchLocationRva + 5);
        patcher.buffer[fileOffset] = 0x90;
        patcher.buffer[fileOffset + 1] = 0x90;

        PLOG_INFO << "[+] Init Engine successfully aborted at CSWindow!";
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
        if (!AbortInitAtCSWindow(patcher, initEngineRva))
        {
            PLOG_ERROR << "[-] Failed to bypass functions in final function patch.";
            return false;
        }
        // if (!InitFunctionBypasses(patcher, initEngineRva))
        // {
        //     PLOG_ERROR << "[-] Failed to bypass functions in final function patch.";
        //     return false;
        // }
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

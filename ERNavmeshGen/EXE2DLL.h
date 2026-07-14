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
        PLOG_INFO << "[***] Processing Engine Init (Bypassing CSWindow)";

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
            if (cursor.instr.mnemonic == ZYDIS_MNEMONIC_INT3)
            {
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
        
        return true;
    }


    static bool BypassMutexAndSteam(PEPatcher& patcher, uint32_t winMainRva)
    {
        PLOG_INFO << "Processing Mutex and Steam Bypasses";

        InstructionCursor cursor(patcher.buffer, patcher.imageBase, winMainRva);

        uint32_t mutexFuncRva = 0;
        uint32_t steamFuncRva = 0;

        // Use u"..." to guarantee 2-byte UTF-16 characters exactly as they appear in the PE file
        std::u16string mutexStr = u"Global\\SekiroMutex";

        // Calculate the physical size in bytes (17 characters * 2 bytes = 34 bytes)
        size_t compareBytes = mutexStr.length() * sizeof(char16_t);

        PLOG_INFO << "[*] Scanning WinMain for Initialization sequence...";

        // --- Step 1: Find IsGameAlreadyOpen (Mutex) ---
        while (cursor.Next())
        {
            if (cursor.instr.mnemonic == ZYDIS_MNEMONIC_CALL)
            {
                uint32_t callTargetRva = cursor.GetAbsoluteAddress(0) - patcher.imageBase;
                InstructionCursor targetCursor(patcher.buffer, patcher.imageBase, callTargetRva);

                for (int j = 0; j < 30; ++j)
                {
                    if (!targetCursor.Next()) break;

                    if (targetCursor.instr.mnemonic == ZYDIS_MNEMONIC_LEA &&
                        targetCursor.operands[1].type == ZYDIS_OPERAND_TYPE_MEMORY &&
                        targetCursor.operands[1].mem.base == ZYDIS_REGISTER_RIP)
                    {
                        uint32_t strRva = targetCursor.GetAbsoluteAddress(1) - patcher.imageBase;
                        uint32_t strOffset = patcher.RvaToFileOffset(strRva);

                        if (strOffset > 0 && strOffset + compareBytes <= patcher.buffer.size())
                        {
                            // We compare against mutexStr.data() using our calculated byte length
                            if (memcmp(&patcher.buffer[strOffset], mutexStr.data(), compareBytes) == 0)
                            {
                                mutexFuncRva = callTargetRva; // We found the Mutex function!
                                break;
                            }
                        }
                    }
                    if (targetCursor.instr.mnemonic == ZYDIS_MNEMONIC_RET) break;
                }
            }

            if (mutexFuncRva != 0) break; // Found the Mutex call, stop scanning!
            if (cursor.instr.mnemonic == ZYDIS_MNEMONIC_INT3)
            {
                PLOG_ERROR << "[-] Failed to locate IsGameAlreadyOpen.";
                return false;
            }
        }

        // --- Step 2: Find SetUpSteamAPI ---
        // The cursor is currently sitting exactly on `CALL IsGameAlreadyOpen`.
        // The very next CALL instruction in WinMain is guaranteed to be SetUpSteamAPI.
        while (cursor.Next())
        {
            if (cursor.instr.mnemonic == ZYDIS_MNEMONIC_CALL)
            {
                steamFuncRva = cursor.GetAbsoluteAddress(0) - patcher.imageBase;
                break;
            }
            // If we hit another control flow abstraction before a CALL, something is wrong
            if (cursor.instr.mnemonic == ZYDIS_MNEMONIC_JMP || cursor.instr.mnemonic == ZYDIS_MNEMONIC_RET) break;
        }

        if (steamFuncRva == 0)
        {
            PLOG_ERROR << "[-] Failed to locate SetUpSteamAPI.";
            return false;
        }

        PLOG_INFO << "[+] Found IsGameAlreadyOpen at RVA: 0x" << std::hex << mutexFuncRva;
        PLOG_INFO << "[+] Found SetUpSteamAPI at RVA: 0x" << steamFuncRva;

        // --- Step 3: Apply the Bypasses ---
        // We want both functions to immediately return TRUE (EAX = 1) without doing any work.
        std::string asmCode = "mov eax, 1; ret;";

        PLOG_INFO << "[*] Patching IsGameAlreadyOpen...";
        patcher.WritePatch(mutexFuncRva, asmCode);

        PLOG_INFO << "[*] Patching SetUpSteamAPI...";
        patcher.WritePatch(steamFuncRva, asmCode);

        PLOG_INFO << "[+] Mutex and Steam DRM checks successfully neutralized!";
        return true;
    }

    static bool BypassSteamInit(PEPatcher& patcher, uint32_t initEngineRva)
    {
        PLOG_INFO << "Processing Engine Init (Bypassing Steam)";

        InstructionCursor cursor(patcher.buffer, patcher.imageBase, initEngineRva);

        uint64_t steamSystemObjAddr = 0;
        uint32_t firstCallRva = 0;
        uint32_t secondCallRva = 0;

        PLOG_INFO << "[*] Scanning for Steam System object references...";

        while (cursor.Next())
        {
            // --- Step 1: Find the first Steam Call (with the JZ check) ---
            if (steamSystemObjAddr == 0)
            {
                if (cursor.instr.mnemonic == ZYDIS_MNEMONIC_LEA &&
                    cursor.operands[0].reg.value == ZYDIS_REGISTER_RCX &&
                    cursor.operands[1].type == ZYDIS_OPERAND_TYPE_MEMORY &&
                    cursor.operands[1].mem.base == ZYDIS_REGISTER_RIP)
                {
                    uint64_t candidateAddr = cursor.GetAbsoluteAddress(1);

                    InstructionCursor peek1 = cursor.Peek();
                    if (peek1.instr.mnemonic == ZYDIS_MNEMONIC_CALL)
                    {
                        InstructionCursor peek2 = peek1.Peek();
                        if (peek2.instr.mnemonic == ZYDIS_MNEMONIC_TEST &&
                            peek2.operands[0].reg.value == ZYDIS_REGISTER_AL)
                        {
                            InstructionCursor peek3 = peek2.Peek();
                            if (peek3.instr.mnemonic == ZYDIS_MNEMONIC_JZ)
                            {
                                // We found the first Steam setup sequence!
                                steamSystemObjAddr = candidateAddr;
                                firstCallRva = peek1.rva;
                                PLOG_INFO << "[+] Steam Singleton found at absolute address: 0x" << std::hex <<
                                    steamSystemObjAddr;
                                PLOG_INFO << "[+] Found 1st Steam CALL at RVA: 0x" << firstCallRva;
                            }
                        }
                    }
                }
            }
            // --- Step 2: Find the second Steam Call (using the tracked address) ---
            else
            {
                if (cursor.instr.mnemonic == ZYDIS_MNEMONIC_LEA &&
                    cursor.operands[0].reg.value == ZYDIS_REGISTER_RCX &&
                    cursor.operands[1].type == ZYDIS_OPERAND_TYPE_MEMORY &&
                    cursor.operands[1].mem.base == ZYDIS_REGISTER_RIP)
                {
                    // Is it loading the exact same Steam object?
                    if (cursor.GetAbsoluteAddress(1) == steamSystemObjAddr)
                    {
                        InstructionCursor peek1 = cursor.Peek();
                        if (peek1.instr.mnemonic == ZYDIS_MNEMONIC_CALL)
                        {
                            secondCallRva = peek1.rva;
                            PLOG_INFO << "[+] Found 2nd Steam CALL at RVA: 0x" << secondCallRva;
                            break; // We have both targets, get out of the loop
                        }
                    }
                }
            }

            if (cursor.instr.mnemonic == ZYDIS_MNEMONIC_INT3)
            {
                std::cerr << "[-] Failed to find both Steam initialization calls.";
                return false;
            };
        }

        // --- Step 3: Apply the Patches ---

        // First CALL: We must force AL to 1 to bypass the JZ trap.
        // 'mov al, 1' is 2 bytes. The original CALL is 5 bytes. 
        // We pad the remaining 3 bytes with NOPs.
        std::string patch1 = "mov al, 1; nop; nop; nop;";
        PLOG_INFO << "[*] Patching 1st Call: " << patch1;
        patcher.WritePatch(firstCallRva, patch1);

        // Second CALL: We just need to completely erase the 5-byte call.
        std::string patch2 = "nop; nop; nop; nop; nop;";
        PLOG_INFO << "[*] Patching 2nd Call: " << patch2;
        patcher.WritePatch(secondCallRva, patch2);

        PLOG_INFO << "[+] Steam Initialization successfully neutralized!";
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


        if (!BypassMutexAndSteam(patcher, winMainRva))
        {
            PLOG_ERROR << "[-] Failed to patch Mutex in WinMain patch.";
            return false;
        }

        uint32_t initEngineRva = mainLoopSearchCursor.GetAbsoluteAddress(0) - imageBase;
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
        if (!BypassSteamInit(patcher, initEngineRva))
        {
            PLOG_ERROR << "[-] Failed to bypass steam init functions in final function patch.";
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

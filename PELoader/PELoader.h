#pragma once

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <intrin.h>
#include <fstream>

#define WIN_X64

#include <windows.h>


#define DEREF(name)*(UINT_PTR *)(name)
#define DEREF_64(name)*(DWORD64 *)(name)
#define DEREF_32(name)*(DWORD *)(name)
#define DEREF_16(name)*(WORD *)(name)
#define DEREF_8(name)*(BYTE *)(name)
typedef DWORD (NTAPI *NTFLUSHINSTRUCTIONCACHE)(HANDLE, PVOID, ULONG);
typedef uint64_t NAKED_ENTRY_POINT(void);

typedef struct {
    WORD offset : 12;
    WORD type : 4;
} IMAGE_RELOC, *PIMAGE_RELOC;

class PELoader {
public:
    //===============================================================================================//

    //===============================================================================================//
    //===============================================================================================//

    //===============================================================================================//
    static uintptr_t WINAPI Load(const char* path, uintptr_t* entry_point) {
        if (path == nullptr) {
            return 0;
        }

        std::ifstream file(path, std::ios::binary | std::ios::ate);
        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);
        std::vector<char> buffer(size);
        const uintptr_t image = (uintptr_t)buffer.data();
        if (!file.read(buffer.data(), size)) {
            return 0;
        }
        return Load(image, entry_point);
    }

    // This is our DLL loader/injector
    static uintptr_t WINAPI Load(uintptr_t uiLibraryAddress, uintptr_t* entry_point) {
        // the function we need
        HMODULE ntdll = GetModuleHandle(L"ntdll.dll");
        const NTFLUSHINSTRUCTIONCACHE pNtFlushInstructionCache = (NTFLUSHINSTRUCTIONCACHE)GetProcAddress(ntdll,
                                                                                                         "NtFlushInstructionCache");

        // the kernels base address and later this images newly loaded base address
        ULONG_PTR uiBaseAddress;

        // variables for processing the kernels export table
        ULONG_PTR uiAddressArray;
        ULONG_PTR uiNameArray;
        ULONG_PTR uiExportDir;

        // variables for loading this image
        ULONG_PTR uiHeaderValue;
        ULONG_PTR uiValueA;
        ULONG_PTR uiValueB;
        ULONG_PTR uiValueC;
        ULONG_PTR uiValueD;
        ULONG_PTR uiValueE;

        // STEP 1: load our image into a new permanent location in memory...

        // get the VA of the NT Header for the PE to be loaded
        uiHeaderValue = uiLibraryAddress + ((PIMAGE_DOS_HEADER)uiLibraryAddress)->e_lfanew;

        // allocate all the memory for the DLL to be loaded into. we can load at any address because we will
        // relocate the image. Also zeros all memory and marks it as READ, WRITE and EXECUTE to avoid any problems.
        uiBaseAddress = (ULONG_PTR)VirtualAlloc(NULL, ((PIMAGE_NT_HEADERS)uiHeaderValue)->OptionalHeader.SizeOfImage,
                                                MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);

        // we must now copy over the headers
        uiValueA = ((PIMAGE_NT_HEADERS)uiHeaderValue)->OptionalHeader.SizeOfHeaders;
        uiValueB = uiLibraryAddress;
        uiValueC = uiBaseAddress;

        while (uiValueA--)
            *(BYTE*)uiValueC++ = *(BYTE*)uiValueB++;

        // STEP 2: load in all of our sections...

        // uiValueA = the VA of the first section
        uiValueA = ((ULONG_PTR)&((PIMAGE_NT_HEADERS)uiHeaderValue)->OptionalHeader +
            ((PIMAGE_NT_HEADERS)uiHeaderValue)->FileHeader.SizeOfOptionalHeader);

        // itterate through all sections, loading them into memory.
        uiValueE = ((PIMAGE_NT_HEADERS)uiHeaderValue)->FileHeader.NumberOfSections;
        while (uiValueE--) {
            // uiValueB is the VA for this section
            uiValueB = (uiBaseAddress + ((PIMAGE_SECTION_HEADER)uiValueA)->VirtualAddress);

            // uiValueC if the VA for this sections data
            uiValueC = (uiLibraryAddress + ((PIMAGE_SECTION_HEADER)uiValueA)->PointerToRawData);

            // copy the section over
            uiValueD = ((PIMAGE_SECTION_HEADER)uiValueA)->SizeOfRawData;

            while (uiValueD--)
                *(BYTE*)uiValueB++ = *(BYTE*)uiValueC++;

            // get the VA of the next section
            uiValueA += sizeof(IMAGE_SECTION_HEADER);
        }

        // STEP 3: process our images import table...

        // uiValueB = the address of the import directory
        uiValueB = (ULONG_PTR)&((PIMAGE_NT_HEADERS)uiHeaderValue)->OptionalHeader.DataDirectory[
            IMAGE_DIRECTORY_ENTRY_IMPORT];

        // we assume their is an import table to process
        // uiValueC is the first entry in the import table
        uiValueC = (uiBaseAddress + ((PIMAGE_DATA_DIRECTORY)uiValueB)->VirtualAddress);

        // itterate through all imports
        while (((PIMAGE_IMPORT_DESCRIPTOR)uiValueC)->Name) {
            // use LoadLibraryA to load the imported module into memory
            uiLibraryAddress = (ULONG_PTR)LoadLibraryA(
                (LPCSTR)(uiBaseAddress + ((PIMAGE_IMPORT_DESCRIPTOR)uiValueC)->Name));

            // uiValueD = VA of the OriginalFirstThunk
            uiValueD = (uiBaseAddress + ((PIMAGE_IMPORT_DESCRIPTOR)uiValueC)->OriginalFirstThunk);

            // uiValueA = VA of the IAT (via first thunk not origionalfirstthunk)
            uiValueA = (uiBaseAddress + ((PIMAGE_IMPORT_DESCRIPTOR)uiValueC)->FirstThunk);

            // itterate through all imported functions, importing by ordinal if no name present
            while (DEREF(uiValueA)) {
                // sanity check uiValueD as some compilers only import by FirstThunk
                if (uiValueD && ((PIMAGE_THUNK_DATA)uiValueD)->u1.Ordinal & IMAGE_ORDINAL_FLAG) {
                    // get the VA of the modules NT Header
                    uiExportDir = uiLibraryAddress + ((PIMAGE_DOS_HEADER)uiLibraryAddress)->e_lfanew;

                    // uiNameArray = the address of the modules export directory entry
                    uiNameArray = (ULONG_PTR)&((PIMAGE_NT_HEADERS)uiExportDir)->OptionalHeader.DataDirectory[
                        IMAGE_DIRECTORY_ENTRY_EXPORT];

                    // get the VA of the export directory
                    uiExportDir = (uiLibraryAddress + ((PIMAGE_DATA_DIRECTORY)uiNameArray)->VirtualAddress);

                    // get the VA for the array of addresses
                    uiAddressArray = (uiLibraryAddress + ((PIMAGE_EXPORT_DIRECTORY)uiExportDir)->AddressOfFunctions);

                    // use the import ordinal (- export ordinal base) as an index into the array of addresses
                    uiAddressArray += ((IMAGE_ORDINAL(((PIMAGE_THUNK_DATA) uiValueD)->u1.Ordinal) -
                        ((PIMAGE_EXPORT_DIRECTORY)uiExportDir)->Base) * sizeof(DWORD));

                    // patch in the address for this imported function
                    DEREF(uiValueA) = (uiLibraryAddress + DEREF_32(uiAddressArray));
                }
                else {
                    // get the VA of this functions import by name struct
                    uiValueB = (uiBaseAddress + DEREF(uiValueA));

                    // use GetProcAddress and patch in the address for this imported function
                    DEREF(uiValueA) = (ULONG_PTR)GetProcAddress((HMODULE)uiLibraryAddress,
                                                                (LPCSTR)((PIMAGE_IMPORT_BY_NAME)uiValueB)->Name);
                }
                // get the next imported function
                uiValueA += sizeof(ULONG_PTR);
                if (uiValueD)
                    uiValueD += sizeof(ULONG_PTR);
            }

            // get the next import
            uiValueC += sizeof(IMAGE_IMPORT_DESCRIPTOR);
        }

        // STEP 4: process all of our images relocations...

        // calculate the base address delta and perform relocations (even if we load at desired image base)
        uiLibraryAddress = uiBaseAddress - ((PIMAGE_NT_HEADERS)uiHeaderValue)->OptionalHeader.ImageBase;

        // uiValueB = the address of the relocation directory
        uiValueB = (ULONG_PTR)&((PIMAGE_NT_HEADERS)uiHeaderValue)->OptionalHeader.DataDirectory[
            IMAGE_DIRECTORY_ENTRY_BASERELOC];

        // check if there are any relocations present
        if (((PIMAGE_DATA_DIRECTORY)uiValueB)->Size) {
            // uiValueC is now the first entry (IMAGE_BASE_RELOCATION)
            uiValueC = (uiBaseAddress + ((PIMAGE_DATA_DIRECTORY)uiValueB)->VirtualAddress);

            // and we itterate through all entries...
            while (((PIMAGE_BASE_RELOCATION)uiValueC)->SizeOfBlock) {
                // uiValueA = the VA for this relocation block
                uiValueA = (uiBaseAddress + ((PIMAGE_BASE_RELOCATION)uiValueC)->VirtualAddress);

                // uiValueB = number of entries in this relocation block
                uiValueB = (((PIMAGE_BASE_RELOCATION)uiValueC)->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) /
                    sizeof(IMAGE_RELOC);

                // uiValueD is now the first entry in the current relocation block
                uiValueD = uiValueC + sizeof(IMAGE_BASE_RELOCATION);

                // we itterate through all the entries in the current block...
                while (uiValueB--) {
                    // perform the relocation, skipping IMAGE_REL_BASED_ABSOLUTE as required.
                    // we dont use a switch statement to avoid the compiler building a jump table
                    // which would not be very position independent!
                    if (((PIMAGE_RELOC)uiValueD)->type == IMAGE_REL_BASED_DIR64)
                        *(ULONG_PTR*)(uiValueA + ((PIMAGE_RELOC)uiValueD)->offset) += uiLibraryAddress;
                    else if (((PIMAGE_RELOC)uiValueD)->type == IMAGE_REL_BASED_HIGHLOW)
                        *(DWORD*)(uiValueA + ((PIMAGE_RELOC)uiValueD)->offset) += (DWORD)uiLibraryAddress;
                    else if (((PIMAGE_RELOC)uiValueD)->type == IMAGE_REL_BASED_HIGH)
                        *(WORD*)(uiValueA + ((PIMAGE_RELOC)uiValueD)->offset) += HIWORD(uiLibraryAddress);
                    else if (((PIMAGE_RELOC)uiValueD)->type == IMAGE_REL_BASED_LOW)
                        *(WORD*)(uiValueA + ((PIMAGE_RELOC)uiValueD)->offset) += LOWORD(uiLibraryAddress);

                    // get the next entry in the current relocation block
                    uiValueD += sizeof(IMAGE_RELOC);
                }

                // get the next entry in the relocation directory
                uiValueC = uiValueC + ((PIMAGE_BASE_RELOCATION)uiValueC)->SizeOfBlock;
            }
        }

        // STEP 5: call our images entry point

        // if the pointer to entry point isn't NULL, write the entry point to the pointer.
        if (entry_point != NULL) {
            *entry_point = (uiBaseAddress + ((PIMAGE_NT_HEADERS)uiHeaderValue)->OptionalHeader.AddressOfEntryPoint);
        }

        ((PIMAGE_NT_HEADERS)uiHeaderValue)->OptionalHeader.BaseOfCode = uiBaseAddress;
        // We must flush the instruction cache to avoid stale code being used which was updated by our relocation processing.
        pNtFlushInstructionCache((HANDLE)-1, NULL, 0);

        // STEP 6: return the base address of the newly loaded module.
        return uiBaseAddress;
    }

    //===============================================================================================//
};

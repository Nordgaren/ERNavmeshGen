#include "PEHelper.h"

#define PRINT_WINAPI_ERR(cApiName) printf("[!] %s Failed With Error: %d\n", cApiName, GetLastError())

typedef struct _BASE_RELOCATION_ENTRY {
    WORD Offset : 12; // Specifies where the base relocation is to be applied.
    WORD Type : 4; // Indicates the type of base relocation to be applied.
} BASE_RELOCATION_ENTRY, *PBASE_RELOCATION_ENTRY;

PIMAGE_DOS_HEADER PEHelper::GetDosHeader() const {
    return reinterpret_cast<PIMAGE_DOS_HEADER>(GetBaseAddress());
}

PIMAGE_NT_HEADERS PEHelper::GetNTHeaders() const {
    const PIMAGE_DOS_HEADER dosHeader = GetDosHeader();
    return reinterpret_cast<PIMAGE_NT_HEADERS>(GetBaseAddress() + dosHeader->e_lfanew);
}

PIMAGE_SECTION_HEADER PEHelper::GetSectionHeaders() const {
    return reinterpret_cast<PIMAGE_SECTION_HEADER>(reinterpret_cast<uintptr_t>(GetNTHeaders()) + sizeof(
            IMAGE_NT_HEADERS)
    );
}

PNAKED_ENTRY_POINT PEHelper::GetNakedEntryPoint() const {
    const PIMAGE_NT_HEADERS ntHeaders = GetNTHeaders();
    return reinterpret_cast<PNAKED_ENTRY_POINT>(GetBaseAddress() + ntHeaders->OptionalHeader.AddressOfEntryPoint);
}

PENTRY_POINT PEHelper::GetEntryPoint() const {
    return reinterpret_cast<PENTRY_POINT>(GetNakedEntryPoint());
}

PWIN_MAIN PEHelper::GetWinMain() const {
    return reinterpret_cast<PWIN_MAIN>(GetNakedEntryPoint());
}

PDLL_MAIN PEHelper::GetDLLMain() const {
    return reinterpret_cast<PDLL_MAIN>(GetNakedEntryPoint());
}

PIMAGE_DATA_DIRECTORY PEHelper::GetImageDataDirectory(int directory) const {
    const PIMAGE_NT_HEADERS ntHeaders = GetNTHeaders();
    return &ntHeaders->OptionalHeader.DataDirectory[directory];
}

inline uintptr_t PEHelper::GetBaseAddress() const {
    return _baseAddress;
}

std::vector<uint8_t> PEHelper::loadFileFromDisk(std::string& path) {
    std::vector<uint8_t> buffer;
    std::ifstream f(path, std::ios::binary);
    f.seekg(0, std::ios::end);
    buffer.resize(f.tellg());
    f.seekg(0);
    f.read(reinterpret_cast<char*>(buffer.data()), buffer.size());
    
    return buffer;
}

const PEHelper* PEHelper::ManualLoadPE(std::string& path) {
    std::vector<uint8_t> buffer = loadFileFromDisk(path);

    const PEHelper fileOnDisk(reinterpret_cast<uintptr_t>(buffer.data()));

    const uintptr_t pFileBuffer = fileOnDisk.GetBaseAddress();

    uintptr_t pPeBaseAddress = reinterpret_cast<uintptr_t>(VirtualAlloc(
        reinterpret_cast<LPVOID>(fileOnDisk.GetNTHeaders()->OptionalHeader.ImageBase),
        fileOnDisk.GetNTHeaders()->OptionalHeader.SizeOfImage,
        MEM_RESERVE | MEM_COMMIT,
        PAGE_READWRITE
    ));

    if (pPeBaseAddress == NULL) {
        PRINT_WINAPI_ERR("VirtualAlloc");
        return nullptr;
    }

    PIMAGE_NT_HEADERS pImgNtHdrs = fileOnDisk.GetNTHeaders();
    memcpy(reinterpret_cast<PVOID>(pPeBaseAddress), reinterpret_cast<PVOID>(pFileBuffer), pImgNtHdrs->OptionalHeader.SizeOfHeaders);
    // Map sections to the new memory using the VirtualAddress and PointerToRawData offsets
    PIMAGE_SECTION_HEADER pImgSecHdr = fileOnDisk.GetSectionHeaders();
    for (int i = 0; i < pImgNtHdrs->FileHeader.NumberOfSections; i++) {
        memcpy(
            (PVOID)(pPeBaseAddress +
                pImgSecHdr[i].VirtualAddress), // Distination: pPeBaseAddress + RVA
            (PVOID)(pFileBuffer +
                pImgSecHdr[i].PointerToRawData), // Source: pPeHdrs->pFileBuffer + RVA
            pImgSecHdr[i].SizeOfRawData // Size
        );
    }

    const PEHelper* pe = new PEHelper(pPeBaseAddress);
    // pPeHdrs->pEntryBaseRelocDataDir: Equal to &pPeHdrs->pImgNtHdrs->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC] - Initialized in "InitializePeStruct".
    // pPeBaseAddress: Is the current base address of the PE payload - returned by the "VirtualAlloc" call.
    // pPeHdrs->pImgNtHdrs->OptionalHeader.ImageBase: Is the preferable address for the PE payload to be mapped to.
    if (!pe->FixReloc()) {
        printf("Failed reloc");
        return nullptr;
    }

    // pPeHdrs->pEntryImportDataDir: Equals to &pPeHdrs->pImgNtHdrs->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT] - Initialized in "InitializePeStruct".
    // pPeBaseAddress: Is the base address of the PE payload - returned by the "VirtualAlloc" call.
    if (!pe->FixImportAddressTable()) {
        printf("Failed IAT Patch\n");
        return nullptr;
    }

    if (!pe->FixMemPermissions()) {
        printf("Failed mem permissions");
        return nullptr;
    }

    if (!pe->ResolveExceptionHandlers()) {
        printf("Failed exception handler");
        return nullptr;
    }


    return pe;
}

bool PEHelper::FixReloc() const {
    uintptr_t pPeBaseAddress = GetBaseAddress();
    PIMAGE_DATA_DIRECTORY pEntryBaseRelocDataDir = GetImageDataDirectory(IMAGE_DIRECTORY_ENTRY_BASERELOC);
    uintptr_t pPreferableAddress = GetNTHeaders()->OptionalHeader.ImageBase;
    // Pointer to the beginning of the base relocation block.
    PIMAGE_BASE_RELOCATION pImgBaseRelocation = reinterpret_cast<PIMAGE_BASE_RELOCATION>(pPeBaseAddress +
        pEntryBaseRelocDataDir->VirtualAddress);
    // The difference between the current PE image base address and its preferable base address.
    ULONG_PTR uDeltaOffset = pPeBaseAddress - pPreferableAddress;

    // Pointer to individual base relocation entries.
    PBASE_RELOCATION_ENTRY pBaseRelocEntry = NULL;

    // Iterate through all the base relocation blocks.
    while (pImgBaseRelocation->VirtualAddress) {
        // Pointer to the first relocation entry in the current block.
        pBaseRelocEntry = reinterpret_cast<PBASE_RELOCATION_ENTRY>(pImgBaseRelocation + 1);

        // Iterate through all the relocation entries in the current block.
        while (reinterpret_cast<PBYTE>(pBaseRelocEntry) != reinterpret_cast<PBYTE>(pImgBaseRelocation) + pImgBaseRelocation->SizeOfBlock) {
            // Process the relocation entry based on its type.
            switch (pBaseRelocEntry->Type) {
            case IMAGE_REL_BASED_DIR64:
                // Adjust a 64-bit field by the delta offset.
                *reinterpret_cast<ULONG_PTR*>(pPeBaseAddress + pImgBaseRelocation->VirtualAddress +
                    pBaseRelocEntry->Offset) += uDeltaOffset;
                break;
            case IMAGE_REL_BASED_HIGHLOW:
                // Adjust a 32-bit field by the delta offset.
                *reinterpret_cast<DWORD*>(pPeBaseAddress + pImgBaseRelocation->VirtualAddress +
                    pBaseRelocEntry->Offset) += static_cast<DWORD>(uDeltaOffset);
                break;
            case IMAGE_REL_BASED_HIGH:
                // Adjust the high 16 bits of a 32-bit field.
                *reinterpret_cast<WORD*>(pPeBaseAddress + pImgBaseRelocation->VirtualAddress +
                    pBaseRelocEntry->Offset) += HIWORD(uDeltaOffset);
                break;
            case IMAGE_REL_BASED_LOW:
                // Adjust the low 16 bits of a 32-bit field.
                *reinterpret_cast<WORD*>(pPeBaseAddress + pImgBaseRelocation->VirtualAddress +
                    pBaseRelocEntry->Offset) += LOWORD(uDeltaOffset);
                break;
            case IMAGE_REL_BASED_ABSOLUTE:
                // No relocation is required.
                break;
            default:
                // Handle unknown relocation types.
                printf("[!] Unknown relocation type: %d | Offset: 0x%08X \n", pBaseRelocEntry->Type,
                       pBaseRelocEntry->Offset);
                return FALSE;
            }
            // Move to the next relocation entry.
            pBaseRelocEntry++;
        }

        // Move to the next relocation block.
        pImgBaseRelocation = reinterpret_cast<PIMAGE_BASE_RELOCATION>(pBaseRelocEntry);
    }

    return TRUE;
}

bool PEHelper::FixImportAddressTable() const {
    PIMAGE_DATA_DIRECTORY pEntryImportDataDir = GetImageDataDirectory(IMAGE_DIRECTORY_ENTRY_IMPORT);
    uintptr_t pPeBaseAddress = GetBaseAddress();
    // Pointer to an import descriptor for a DLL
    // Iterate over the import descriptors
    for (SIZE_T i = 0; i < pEntryImportDataDir->Size; i += sizeof(IMAGE_IMPORT_DESCRIPTOR)) {
        // Get the current import descriptor
        PIMAGE_IMPORT_DESCRIPTOR pImgDescriptor = reinterpret_cast<PIMAGE_IMPORT_DESCRIPTOR>(pPeBaseAddress + pEntryImportDataDir->VirtualAddress + i);
        // If both thunks are NULL, we've reached the end of the import descriptors list
        if (pImgDescriptor->OriginalFirstThunk == NULL && pImgDescriptor->FirstThunk == NULL)
            break;

        // Retrieve information from the current import descriptor
        LPSTR cDllName = reinterpret_cast<LPSTR>(pPeBaseAddress + pImgDescriptor->Name);
        ULONG_PTR uOriginalFirstThunkRVA = pImgDescriptor->OriginalFirstThunk;
        ULONG_PTR uFirstThunkRVA = pImgDescriptor->FirstThunk;
        SIZE_T ImgThunkSize = 0x00; // Used to move to the next function (iterating through the IAT and INT)
        HMODULE hModule = NULL;

        // Try to load the DLL referenced by the current import descriptor
        hModule = LoadLibraryA(cDllName);
        if (!hModule) {
            PRINT_WINAPI_ERR("LoadLibraryA");
            return false;
        }

        // Iterate over the imported functions for the current DLL
        while (true) {
            // Get pointers to the first thunk and original first thunk data
            PIMAGE_THUNK_DATA pOriginalFirstThunk = reinterpret_cast<PIMAGE_THUNK_DATA>(pPeBaseAddress + uOriginalFirstThunkRVA +
                ImgThunkSize);
            PIMAGE_THUNK_DATA pFirstThunk = reinterpret_cast<PIMAGE_THUNK_DATA>(pPeBaseAddress + uFirstThunkRVA + ImgThunkSize);
            PIMAGE_IMPORT_BY_NAME pImgImportByName = NULL;
            ULONG_PTR pFuncAddress = (ULONG_PTR)NULL;

            // At this point both 'pOriginalFirstThunk' & 'pFirstThunk' will have the same values
            // However, to populate the IAT (pFirstThunk), one should use the INT (pOriginalFirstThunk) to retrieve the
            // functions addresses and patch the IAT (pFirstThunk->u1.Function) with the retrieved address.
            if (pOriginalFirstThunk->u1.Function == NULL && pFirstThunk->u1.Function == NULL) {
                break;
            }

            // If the ordinal flag is set, import the function by its ordinal number
            if (IMAGE_SNAP_BY_ORDINAL(pOriginalFirstThunk->u1.Ordinal)) {
                pFuncAddress = reinterpret_cast<ULONG_PTR>(GetProcAddress(hModule, reinterpret_cast<LPCSTR>(IMAGE_ORDINAL(
                                                                              pOriginalFirstThunk->u1.Ordinal))));
                if (!pFuncAddress) {
                    printf("[!] Could Not Import !%s#%d \n", cDllName, static_cast<int>(pOriginalFirstThunk->u1.Ordinal));
                    return false;
                }
            }
            // Import function by name
            else {
                pImgImportByName = reinterpret_cast<PIMAGE_IMPORT_BY_NAME>(pPeBaseAddress + pOriginalFirstThunk->u1.AddressOfData);
                pFuncAddress = reinterpret_cast<ULONG_PTR>(GetProcAddress(hModule, pImgImportByName->Name));
                if (!pFuncAddress) {
                    printf("[!] Could Not Import !%s.%s \n", cDllName, pImgImportByName->Name);
                    return false;
                }
            }

            // Install the function address in the IAT
            pFirstThunk->u1.Function = (ULONGLONG)pFuncAddress;

            // Move to the next function in the IAT/INT array
            ImgThunkSize += sizeof(IMAGE_THUNK_DATA);
        }
    }

    return true;
}

bool PEHelper::PatchIAT() const {
    const uintptr_t baseAddress = GetBaseAddress();
    const PIMAGE_DATA_DIRECTORY importDataDirectory = GetImageDataDirectory(IMAGE_DIRECTORY_ENTRY_IMPORT);
    const auto importAddressTable = reinterpret_cast<PIMAGE_IMPORT_DESCRIPTOR>(
        baseAddress + importDataDirectory->VirtualAddress
    );
    const size_t count = importDataDirectory->Size / sizeof(IMAGE_IMPORT_DESCRIPTOR) - 1;

    // return false if the size of the import directory is 0
    if (importDataDirectory->Size == 0) {
        return false;
    }

    for (size_t i = 0; i < count; ++i) {
        const char* name = reinterpret_cast<char*>(baseAddress + importAddressTable[i].Name);
        const auto module = PEHelper(LoadLibraryA(name));
        const uintptr_t moduleAddress = module.GetBaseAddress();
        auto thunk = reinterpret_cast<PIMAGE_THUNK_DATA>(baseAddress + importAddressTable[i].FirstThunk);
        PIMAGE_THUNK_DATA originalFirstThunk =
            reinterpret_cast<PIMAGE_THUNK_DATA>(baseAddress + importAddressTable[i].OriginalFirstThunk);

        while (thunk->u1.AddressOfData != 0) {
            if (originalFirstThunk->u1.Ordinal & IMAGE_ORDINAL_FLAG) {
                // I actually think we can just use GetProcAddress with the ordinal. Will try to fix that, later.
                const PIMAGE_DATA_DIRECTORY moduleExportDataDir = module.GetImageDataDirectory(
                    IMAGE_DIRECTORY_ENTRY_EXPORT
                );

                const auto exportDir = reinterpret_cast<PIMAGE_EXPORT_DIRECTORY>(
                    moduleAddress + moduleExportDataDir->VirtualAddress
                );

                const uint32_t* rvaArray = reinterpret_cast<uint32_t*>(
                    moduleAddress + exportDir->AddressOfFunctions
                );

                rvaArray += (IMAGE_ORDINAL(originalFirstThunk->u1.Ordinal) - exportDir->Base) * sizeof(uint32_t);
                thunk->u1.Function = moduleAddress + *rvaArray;
            }
            else {
                const auto imageImportByName = reinterpret_cast<PIMAGE_IMPORT_BY_NAME>(
                    baseAddress + thunk->u1.Function
                );

                thunk->u1.Function = reinterpret_cast<uintptr_t>(GetProcAddress(
                        reinterpret_cast<HMODULE>(moduleAddress),
                        reinterpret_cast<LPCSTR>(&imageImportByName->Name)
                    )
                );
            }

            thunk++;
            originalFirstThunk++;
        }
    }

    return true;
}

bool PEHelper::FixMemPermissions() const {
    // Loop through each section of the PE image.
    const PIMAGE_NT_HEADERS pImgNtHdrs = this->GetNTHeaders();
    const PIMAGE_SECTION_HEADER pImgSecHdr = this->GetSectionHeaders();
    const uintptr_t pPeBaseAddress = this->GetBaseAddress();
    for (DWORD i = 0; i < pImgNtHdrs->FileHeader.NumberOfSections; i++) {
        // Variables to store the new and old memory protections.
        DWORD dwProtection = 0x00,
              dwOldProtection = 0x00;

        // Skip the section if it has no data or no associated virtual address.
        if (!pImgSecHdr[i].SizeOfRawData || !pImgSecHdr[i].VirtualAddress)
            continue;

        // Determine memory protection based on section characteristics.
        // These characteristics dictate whether the section is readable, writable, executable, etc.

        if (pImgSecHdr[i].Characteristics & IMAGE_SCN_MEM_WRITE)
            dwProtection = PAGE_WRITECOPY;

        if (pImgSecHdr[i].Characteristics & IMAGE_SCN_MEM_READ)
            dwProtection = PAGE_READONLY;

        if ((pImgSecHdr[i].Characteristics & IMAGE_SCN_MEM_WRITE) &&
            (pImgSecHdr[i].Characteristics & IMAGE_SCN_MEM_READ))
            dwProtection = PAGE_READWRITE;

        if (pImgSecHdr[i].Characteristics & IMAGE_SCN_MEM_EXECUTE)
            dwProtection = PAGE_EXECUTE;

        if ((pImgSecHdr[i].Characteristics & IMAGE_SCN_MEM_EXECUTE) &&
            (pImgSecHdr[i].Characteristics & IMAGE_SCN_MEM_WRITE))
            dwProtection = PAGE_EXECUTE_WRITECOPY;

        if ((pImgSecHdr[i].Characteristics & IMAGE_SCN_MEM_EXECUTE) &&
            (pImgSecHdr[i].Characteristics & IMAGE_SCN_MEM_READ))
            dwProtection = PAGE_EXECUTE_READ;

        if ((pImgSecHdr[i].Characteristics & IMAGE_SCN_MEM_EXECUTE) &&
            (pImgSecHdr[i].Characteristics & IMAGE_SCN_MEM_WRITE) &&
            (pImgSecHdr[i].Characteristics & IMAGE_SCN_MEM_READ))
            dwProtection = PAGE_EXECUTE_READWRITE;

        // Apply the determined memory protection to the section.
        if (!VirtualProtect((PVOID)(pPeBaseAddress + pImgSecHdr[i].VirtualAddress), pImgSecHdr[i].SizeOfRawData,
                            dwProtection, &dwOldProtection)) {
            PRINT_WINAPI_ERR("VirtualProtect");
            return false;
        }
    }

    return true;
}

bool PEHelper::HookIATEntry(const std::string& moduleName, const std::string& functionName, uintptr_t newFunc) const {
    if (functionName.empty() || moduleName.empty()) {
        return false;
    }

    const uintptr_t baseAddress = GetBaseAddress();
    const PIMAGE_DATA_DIRECTORY importDataDirectory = GetImageDataDirectory(IMAGE_DIRECTORY_ENTRY_IMPORT);
    const auto importAddressTable = reinterpret_cast<PIMAGE_IMPORT_DESCRIPTOR>(
        baseAddress + importDataDirectory->VirtualAddress
    );
    const size_t count = importDataDirectory->Size / sizeof(IMAGE_IMPORT_DESCRIPTOR);

    // Return false if the size of the import directory is 0
    if (importDataDirectory->Size == 0) {
        return false;
    }

    bool found = false;

    for (size_t i = 0; i < count; ++i) {
        const auto importModuleName(reinterpret_cast<char*>(baseAddress + importAddressTable[i].Name));

        if (moduleName != importModuleName) {
            continue;
        }

        auto thunk = reinterpret_cast<PIMAGE_THUNK_DATA>(baseAddress + importAddressTable[i].FirstThunk);
        auto originalFirstThunk =
            reinterpret_cast<PIMAGE_THUNK_DATA>(baseAddress + importAddressTable[i].OriginalFirstThunk);

        while (thunk->u1.AddressOfData != 0) {
            const auto imageImportByName = reinterpret_cast<PIMAGE_IMPORT_BY_NAME>(baseAddress + originalFirstThunk->u1
                .Function);
            const std::string importName(reinterpret_cast<const char*>(&imageImportByName->Name));

            // We don't return early, because compilers can put the import in multiple places.
            // Multiple entries for the same module with the function.
            if (importName == functionName) {
                found = true;
                thunk->u1.Function = newFunc;
            }

            thunk++;
            originalFirstThunk++;
        }
    }

    return found;
}

bool PEHelper::HookIATEntry(const char* moduleName, const char* functionName, const uintptr_t newFunc) const {
    return HookIATEntry(std::string(moduleName), std::string(functionName), newFunc);
}

inline bool PEHelper::IsMapped() const {
    return _isMapped;
}

bool PEHelper::IsDLL() const {
    return GetNTHeaders()->FileHeader.Characteristics & IMAGE_FILE_DLL;
}

bool PEHelper::IsExecutable() const {
    return GetNTHeaders()->FileHeader.Characteristics & IMAGE_FILE_EXECUTABLE_IMAGE;
}

bool PEHelper::ResolveExceptionHandlers() const {
    // Set exception handlers of the injected PE (if exists)
    PIMAGE_DATA_DIRECTORY exceptionDir = GetImageDataDirectory(IMAGE_DIRECTORY_ENTRY_EXCEPTION);
    if (exceptionDir->Size) {
        uintptr_t pBaseAddress = GetBaseAddress();
        // Retrieve the function table entry
        PIMAGE_RUNTIME_FUNCTION_ENTRY pImgRuntimeFuncEntry = (PIMAGE_RUNTIME_FUNCTION_ENTRY)(pBaseAddress + exceptionDir
            ->VirtualAddress);
        // Register the function table
        if (!RtlAddFunctionTable(pImgRuntimeFuncEntry, (exceptionDir->Size / sizeof(IMAGE_RUNTIME_FUNCTION_ENTRY)),
                                 pBaseAddress)) {
            return false;
            //PRINT_WINAPI_ERR("RtlAddFunctionTable");
        }
    }
    return true;
}

bool PEHelper::checkMapped() const {
    return true;
}

bool PEHelper::checkMappedExports() const {
    return true;
}

uint32_t PEHelper::rva2Foa(const uint32_t rva) const {
    const PIMAGE_SECTION_HEADER sectionHeaders = GetSectionHeaders();

    if (rva < sectionHeaders[0].PointerToRawData) {
        return rva;
    }

    const PIMAGE_NT_HEADERS ntHeaders = GetNTHeaders();
    for (size_t i = 0; i < ntHeaders->FileHeader.NumberOfSections; ++i) {
        if (rva >= sectionHeaders[i].VirtualAddress
            && rva <= sectionHeaders[i].VirtualAddress + sectionHeaders[i].SizeOfRawData) {
            return sectionHeaders[i].PointerToRawData + (rva - sectionHeaders[i].VirtualAddress);
        }
    }

    return 0;
}

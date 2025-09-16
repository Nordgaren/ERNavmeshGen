//
// Created by malware on 5/18/2023.
//

#ifndef PELOADER_HELPERS_H
#define PELOADER_HELPERS_H

FARPROC WINAPI hlpGetProcAddress(HMODULE hMod, char* sProcName) {
    char* pBaseAddr = (char*)hMod;

    // get pointers to main headers/structures
    const IMAGE_DOS_HEADER* pDosHdr = (IMAGE_DOS_HEADER*)pBaseAddr;
    const IMAGE_NT_HEADERS* pNTHdr = (IMAGE_NT_HEADERS*)(pBaseAddr + pDosHdr->e_lfanew);
    const IMAGE_OPTIONAL_HEADER* pOptionalHdr = &pNTHdr->OptionalHeader;
    const IMAGE_DATA_DIRECTORY* pExportDataDir = (&pOptionalHdr->DataDirectory[
        IMAGE_DIRECTORY_ENTRY_EXPORT]);
    IMAGE_EXPORT_DIRECTORY* pExportDirAddr = (IMAGE_EXPORT_DIRECTORY*)(pBaseAddr + pExportDataDir->VirtualAddress);

    // resolve addresses to Export Address Table, table of function names and "table of ordinals"
    const DWORD* pEAT = (DWORD*)(pBaseAddr + pExportDirAddr->AddressOfFunctions);
    const DWORD* pFuncNameTbl = (DWORD*)(pBaseAddr + pExportDirAddr->AddressOfNames);
    const WORD* pHintsTbl = (WORD*)(pBaseAddr + pExportDirAddr->AddressOfNameOrdinals);

    // function address we're looking for
    void* pProcAddr = NULL;

    // resolve function by ordinal
    if (((DWORD_PTR)sProcName >> 16) == 0) {
        const WORD ordinal = (WORD)sProcName & 0xFFFF; // convert to WORD
        const DWORD Base = pExportDirAddr->Base; // first ordinal number

        // check if ordinal is not out of scope
        if (ordinal < Base || ordinal >= Base + pExportDirAddr->NumberOfFunctions)
            return NULL;

        // get the function virtual address = RVA + BaseAddr
        pProcAddr = (FARPROC)(pBaseAddr + (DWORD_PTR)pEAT[ordinal - Base]);
    }
    // resolve function by name
    else {
        // parse through table of function names
        for (DWORD i = 0; i < pExportDirAddr->NumberOfNames; i++) {
            const char* sTmpFuncName = pBaseAddr + (DWORD_PTR)pFuncNameTbl[i];

            if (strcmp(sProcName, sTmpFuncName) == 0) {
                // found, get the function virtual address = RVA + BaseAddr
                pProcAddr = (FARPROC)(pBaseAddr + (DWORD_PTR)pEAT[pHintsTbl[i]]);
                break;
            }
        }
    }

    return (FARPROC)pProcAddr;
}
#endif //PELOADER_HELPERS_H

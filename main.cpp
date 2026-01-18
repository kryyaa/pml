#include <windows.h>
#include <iostream>

extern void loadbytes();

#ifdef DEBUG_CONSOLE
void AllocDebugConsole()
{
    AllocConsole();
    SetConsoleTitleA("kryyaa razvlekaetsa");
    FILE* fDummy;
    freopen_s(&fDummy, "CONOUT$", "w", stdout);
    freopen_s(&fDummy, "CONOUT$", "w", stderr);
    freopen_s(&fDummy, "CONIN$", "r", stdin);
    std::cout.clear();
    std::clog.clear();
    std::cerr.clear();
    std::cin.clear();
}
#endif

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        
#ifdef DEBUG_CONSOLE
        AllocDebugConsole();
        std::cout << "[+] DLL Loaded (DEBUG MODE)" << std::endl;
        std::cout << "[*] Credits: t.me/kryyaasoft" << std::endl;
#endif
        
        CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)loadbytes, NULL, 0, NULL);
        break;
        
    case DLL_PROCESS_DETACH:
#ifdef DEBUG_CONSOLE
        std::cout << "[+] DLL Unloaded" << std::endl;
#endif
        break;
        
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;
    }
    return TRUE;
}
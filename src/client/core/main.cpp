#include <windows.h>
#include <memory>

#include "runtime.hpp"

static std::unique_ptr<Runtime> runtime;

DWORD WINAPI AppInitializationThread(LPVOID)
{
	runtime = Runtime::CreateDefault();
    runtime->Start();
	return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);

        HANDLE hThread = CreateThread(nullptr, 0, AppInitializationThread, nullptr, 0, nullptr);
        if (hThread) {
            CloseHandle(hThread);
        } 
        else {
            MessageBoxA(nullptr, "Failed to create initialization thread.", "Error", MB_ICONERROR);
        }
    }
    else if (reason == DLL_PROCESS_DETACH) {
        if (runtime) {
            runtime->Stop();
            runtime.reset();
        }
        
    }

    return TRUE;
}
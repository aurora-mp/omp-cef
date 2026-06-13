#include <Windows.h>
#include <filesystem>

std::wstring GetErrorMessage(DWORD errorCode)
{
    if (errorCode == 0) {
        return L"No error message generated.";
    }

    LPWSTR messageBuffer = nullptr;
    size_t size = FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        errorCode,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPWSTR)&messageBuffer,
        0,
        NULL);

    std::wstring message(messageBuffer, size);
    LocalFree(messageBuffer);
    return message;
}

void LoadCefClientDll()
{
    wchar_t exePath[MAX_PATH];
    if (!GetModuleFileNameW(nullptr, exePath, MAX_PATH))
        return;

    std::filesystem::path path(exePath);
    path = path.parent_path();

    std::filesystem::path cefPath = path / L"nsy";

    SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_DEFAULT_DIRS | LOAD_LIBRARY_SEARCH_USER_DIRS);
    AddDllDirectory(cefPath.wstring().c_str());

    std::filesystem::path clientDllPath = cefPath / L"client.dll";

    HMODULE clientModule = LoadLibraryW(clientDllPath.wstring().c_str());
    if (!clientModule)
    {
        DWORD err = GetLastError();
        std::wstring errMsg = GetErrorMessage(err);

        std::wstring fullMessage = L"Failed to load cef/client.dll\n\n";
        fullMessage += L"Reason: " + errMsg;
        fullMessage += L"(Error Code: " + std::to_wstring(err) + L")\n\n";
        fullMessage += L"Please ensure all required files (like libcef.dll, renderer.exe, etc.) are present and that your antivirus is not blocking the file.";

        MessageBoxW(nullptr, fullMessage.c_str(), L"omp-cef - Fatal Error", MB_ICONERROR);
        ExitProcess(0);
    }
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID)
{
    if (ul_reason_for_call == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hModule);
        LoadCefClientDll();
    }

    return TRUE;
}

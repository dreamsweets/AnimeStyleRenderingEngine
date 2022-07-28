#include "AnimeStyleRendering.h"
#include "Scene/Scene.h"
#include <Shlwapi.h>
#pragma comment(lib, "shlwapi")

#define MAX_LOADSTRING 100

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    try {
        WCHAR path[MAX_PATH];
        HMODULE hModule = GetModuleHandleW(NULL);
        if (GetModuleFileNameW(hModule, path, MAX_PATH) > 0)
        {
            PathRemoveFileSpecW(path);
            SetCurrentDirectoryW(path);
        }
        HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);

        if (FAILED(hr)) throw;

        AnimeStyleRendering::Inst().Init();
        AnimeStyleRendering::Inst().Run();
        AnimeStyleRendering::Inst().Destroy();
    }
    catch (std::exception e) {
        MessageBoxA(nullptr, e.what(), "Throw Exception!", 0);
    }
}
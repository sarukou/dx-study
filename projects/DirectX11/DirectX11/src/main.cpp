#include "Application.h"
#include "Utility.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "windowscodecs.lib")


// エントリーポイント
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow)
{
    // COM 初期化
    ThrowIfFailed(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED), "CoInitializeEx failed");

    try {
        Application app;

        if (!app.Initialize(hInstance, nCmdShow)) {
            CoUninitialize();
            return -1;
        }

        return app.Run();
    }
    catch (const std::exception& e) {
        // COM 解除
        CoUninitialize();
        MessageBoxA(nullptr, e.what(), "Fatal Error", MB_ICONERROR | MB_OK);
        return -1;
    }
}

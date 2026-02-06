// main.cpp : DirectX11 minimal "clear color" sample (Win32 + D3D11)
// Build: Debug|x64 recommended
// Link: d3d11.lib, dxgi.lib, d3dcompiler.lib

#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>

#include <wrl/client.h>
#include <stdexcept>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
// #pragma comment(lib, "d3dcompiler.lib") // optional

using Microsoft::WRL::ComPtr;

static const wchar_t* kWndClassName = L"Week0_DX11_Clear_Window";
static const int kClientWidth = 1280;
static const int kClientHeight = 720;

struct Dx11Context
{
    ComPtr<ID3D11Device>           device;
    ComPtr<ID3D11DeviceContext>    context;
    ComPtr<IDXGISwapChain>         swapChain;
    ComPtr<ID3D11RenderTargetView> rtv;
};

static Dx11Context g_dx;

static void ThrowIfFailed(HRESULT hr, const char* msg)
{
    if (FAILED(hr))
    {
        throw std::runtime_error(msg);
    }
}

static void CreateRenderTarget()
{
    g_dx.rtv.Reset();

    ComPtr<ID3D11Texture2D> backBuffer;
    ThrowIfFailed(
        g_dx.swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer)),
        "GetBuffer(backBuffer) failed"
    );

    ThrowIfFailed(
        g_dx.device->CreateRenderTargetView(backBuffer.Get(), nullptr, &g_dx.rtv),
        "CreateRenderTargetView failed"
    );
}

static void InitD3D11(HWND hwnd)
{
    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferDesc.Width = kClientWidth;
    sd.BufferDesc.Height = kClientHeight;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;

    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;

    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.BufferCount = 2;                 // double buffer
    sd.OutputWindow = hwnd;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD; // widely compatible
    sd.Flags = 0;

    UINT createFlags = 0;
#if defined(_DEBUG)
    createFlags |= D3D11_CREATE_DEVICE_DEBUG; // if Graphics Tools installed
#endif

    D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0
    };

    D3D_FEATURE_LEVEL createdFL = D3D_FEATURE_LEVEL_11_0;

    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr,                    // adapter (nullptr = default)
        D3D_DRIVER_TYPE_HARDWARE,   // hardware GPU
        nullptr,
        createFlags,
        featureLevels,
        _countof(featureLevels),
        D3D11_SDK_VERSION,
        &sd,
        &g_dx.swapChain,
        &g_dx.device,
        &createdFL,
        &g_dx.context
    );
    ThrowIfFailed(hr, "D3D11CreateDeviceAndSwapChain failed");

    CreateRenderTarget();
}

static void Resize(UINT width, UINT height)
{
    if (!g_dx.swapChain) return;
    if (width == 0 || height == 0) return; // minimized

    g_dx.rtv.Reset();

    // 0,0 => keep buffer count/format
    ThrowIfFailed(
        g_dx.swapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0),
        "ResizeBuffers failed"
    );

    CreateRenderTarget();
}

static void Render()
{
    if (!g_dx.context || !g_dx.rtv) return;

    // Set RTV
    ID3D11RenderTargetView* rtvs[] = { g_dx.rtv.Get() };
    g_dx.context->OMSetRenderTargets(1, rtvs, nullptr);

    // Clear color (RGBA)
    const float clear[4] = { 0.05f, 0.15f, 0.35f, 1.0f };
    g_dx.context->ClearRenderTargetView(g_dx.rtv.Get(), clear);

    // Present
    g_dx.swapChain->Present(1, 0); // vsync on
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_SIZE:
    {
        const UINT w = LOWORD(lParam);
        const UINT h = HIWORD(lParam);
        Resize(w, h);
        return 0;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
}

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, PWSTR, int nCmdShow)
{
    try
    {
        // Register window class
        WNDCLASSEXW wc{};
        wc.cbSize = sizeof(wc);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = WndProc;
        wc.hInstance = hInst;
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.lpszClassName = kWndClassName;
        RegisterClassExW(&wc);

        // Create window
        RECT rc{ 0, 0, kClientWidth, kClientHeight };
        AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

        HWND hwnd = CreateWindowExW(
            0, kWndClassName, L"Week0 DX11 Clear",
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT,
            rc.right - rc.left, rc.bottom - rc.top,
            nullptr, nullptr, hInst, nullptr
        );
        if (!hwnd) return 0;

        ShowWindow(hwnd, nCmdShow);
        UpdateWindow(hwnd);

        // Init D3D11
        InitD3D11(hwnd);

        // Main loop
        MSG msg{};
        while (msg.message != WM_QUIT)
        {
            if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
            else
            {
                Render();
            }
        }
        return (int)msg.wParam;
    }
    catch (const std::exception& e)
    {
        MessageBoxA(nullptr, e.what(), "Fatal Error", MB_ICONERROR | MB_OK);
        return -1;
    }
}
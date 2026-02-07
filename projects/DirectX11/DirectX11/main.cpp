#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>

#include <wrl/client.h>
#include <stdexcept>
#include <array>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

using Microsoft::WRL::ComPtr;

static const wchar_t* windowClassName = L"DirectX11-study";
static const int clientWidth = 1280;
static const int clientHeight = 720;

// クリアカラー（参照渡し学習用）
using Color4 = std::array<float, 4>;
static const Color4 clearColor = { 0.05f, 0.15f, 0.35f, 1.0f };

// COMオブジェクトの構造体
struct Dx11Context
{
    ComPtr<ID3D11Device>           device;
    ComPtr<ID3D11DeviceContext>    context;
    ComPtr<IDXGISwapChain>         swapChain;
    ComPtr<ID3D11RenderTargetView> rtv;
};

static Dx11Context g_dx;

// 例外処理
static void ThrowIfFailed(HRESULT hr, const char* msg)
{
    if (FAILED(hr))
    {
        throw std::runtime_error(msg);
    }
}

// レンダーターゲット作成
static void CreateRenderTarget()
{
    ComPtr<ID3D11Texture2D> backBuffer;

    ThrowIfFailed(
        g_dx.swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer)),
        "GetBuffer(backBuffer) failed"
    );

    ThrowIfFailed(
        g_dx.device->CreateRenderTargetView(backBuffer.Get(), nullptr, g_dx.rtv.ReleaseAndGetAddressOf()),
        "CreateRenderTargetView failed"
    );
}

// 画面クリア（参照渡し）
static void ClearRTV(const Color4& color)
{
    g_dx.context->ClearRenderTargetView(g_dx.rtv.Get(), color.data());
}

// 初期化処理
static void InitD3D11(HWND hwnd)
{
    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferDesc.Width = clientWidth;
    sd.BufferDesc.Height = clientHeight;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;

    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;

    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.BufferCount = 2;
    sd.OutputWindow = hwnd;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
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
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        createFlags,
        featureLevels,
        _countof(featureLevels),
        D3D11_SDK_VERSION,
        &sd,
        g_dx.swapChain.ReleaseAndGetAddressOf(),
        g_dx.device.ReleaseAndGetAddressOf(),
        &createdFL,
        g_dx.context.ReleaseAndGetAddressOf()
    );
    ThrowIfFailed(hr, "D3D11CreateDeviceAndSwapChain failed");

    CreateRenderTarget();
}

// 画面の大きさ変更
static void Resize(UINT width, UINT height)
{
    if (!g_dx.swapChain) return;
    if (width == 0 || height == 0) return;

    g_dx.rtv.Reset();

    ThrowIfFailed(
        g_dx.swapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0),
        "ResizeBuffers failed"
    );

    CreateRenderTarget();
}

// 描画処理
static void Render()
{
    if (!g_dx.context || !g_dx.rtv) return;

    // RTVセット
    ID3D11RenderTargetView* rtvs[] = { g_dx.rtv.Get() };
    g_dx.context->OMSetRenderTargets(1, rtvs, nullptr);

    ClearRTV(clearColor);

    // Present
    g_dx.swapChain->Present(1, 0);
}

// ウィンドウプロシージャ
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

// エントリーポイント
int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, PWSTR, int nCmdShow)
{
    try
    {
        // Register window クラス
        WNDCLASSEXW wc{};
        wc.cbSize = sizeof(wc);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = WndProc;
        wc.hInstance = hInst;
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.lpszClassName = windowClassName;
        RegisterClassExW(&wc);

        // ウィンドウ作成
        RECT rc{ 0, 0, clientWidth, clientHeight };
        AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

        HWND hwnd = CreateWindowExW(
            0, windowClassName, L"Week0 DX11 Clear",
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT,
            rc.right - rc.left, rc.bottom - rc.top,
            nullptr, nullptr, hInst, nullptr
        );
        if (!hwnd) return 0;

        ShowWindow(hwnd, nCmdShow);
        UpdateWindow(hwnd);

        // D3D11初期化
        InitD3D11(hwnd);

        // メインループ
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
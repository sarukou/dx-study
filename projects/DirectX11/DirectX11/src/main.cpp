#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>

#include <wrl/client.h>
#include <stdexcept>

#include <iterator>
#include <string>

#include "BasicVertexShader.h"	// シェーダーをコンパイルしたヘッダーファイル
#include "BasicPixelShader.h"


#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

using Microsoft::WRL::ComPtr;


// プロジェクト設定
struct ProjectSettings
{
    const std::wstring title = L"DirectX11-study";
    const int width = 1280;
    const int height = 720;
};

// COMオブジェクトの構造体
struct Dx11Context
{
    ComPtr<ID3D11Device>           device;              // GPUリソース（バッファ、テクスチャ、シェーダーなど）を作る
    ComPtr<ID3D11DeviceContext>    deviceContext;       // 作ったリソースを使って描画命令を発行する
    ComPtr<IDXGISwapChain>         swapChain;           // 画面に出すための表裏（バックバッファ）の入れ替え役
    ComPtr<ID3D11RenderTargetView> renderTargetView;    // 書き込み先の窓口（バックバッファに直接は書かず View を作ってOMに渡す）
};

// 頂点情報（CPU→GPUに渡す形）
struct Vertex
{
    float x, y, z;      // POSITION
    float r, g, b, a;   // COLOR
};

// シェーダー構造体
struct Shaders
{
    ComPtr<ID3D11VertexShader> vertexShader;
    ComPtr<ID3D11PixelShader> pixelShader;
};


// ウィンドウハンドル
HWND g_hWnd;

// 例外処理
static void ThrowIfFailed(HRESULT hr, const char* msg)
{
    if (FAILED(hr)) {
        throw std::runtime_error(msg);
    }
}

// ウィンドウプロシージャ
static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_CREATE: {
        const auto createStruct = reinterpret_cast<LPCREATESTRUCTW>(lParam);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(createStruct->lpCreateParams));
        return 0;
    }
    case WM_CLOSE:
        if (MessageBoxW(hwnd, L"保存していないデータは破棄されます。", L"ゲームを終了しますか？", MB_YESNO) == IDYES) {
            DestroyWindow(hwnd);
        }
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
}


// ウィンドウ初期化
static void InitWindow(HINSTANCE hInstance, int nCmdShow, const ProjectSettings& settings)
{
    // ウィンドウクラス登録（見た目や挙動のウィンドウを作ると OS に申請）
    WNDCLASSEXW windowClass = {};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;    // 描画の再要求方針（縦・横サイズ変更されたら描き直す）
    windowClass.lpfnWndProc = WndProc;
    windowClass.cbClsExtra = 0;         // クラスに追加する余分なメモリ領域サイズ
    windowClass.cbWndExtra = 0;         // ウィンドウインスタンスに追加する余分な領域
    windowClass.hInstance = hInstance;  // 実行モジュール（EXE/DLL）のインスタンスハンドル
    windowClass.hIcon = NULL;
    windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    windowClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);     // 背景を塗るブラシ
    windowClass.lpszMenuName = NULL;                            // メニューリソース名
    windowClass.lpszClassName = settings.title.c_str();         // ウィンドウクラス名（タイトルと同じだが概念的には別物）

    // ウィンドウクラスを OS に登録
    RegisterClassExW(&windowClass);

    // ウィンドウ作成（クライアント領域を Width * Height にしたいから枠分を足し引き）
    RECT rect{ 0, 0, settings.width, settings.height };
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

    g_hWnd = CreateWindowExW(0, windowClass.lpszClassName, L"DirectX11-study",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
        rect.right - rect.left, rect.bottom - rect.top,
        nullptr, nullptr, hInstance, nullptr);

    if (!g_hWnd) return;

    ShowWindow(g_hWnd, nCmdShow);
    UpdateWindow(g_hWnd);
}


// 初期化処理
static void InitD3D11(HWND hwnd, const ProjectSettings& settings, Dx11Context& dx)
{
    // 表示の仕様書のようなもの（バックバッファの形式や何枚持つか？）
    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
    swapChainDesc.BufferDesc.Width = settings.width;
    swapChainDesc.BufferDesc.Height = settings.height;
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;   // 一般的なRGBA
    swapChainDesc.BufferDesc.RefreshRate = { 60, 1 };               // リフレッシュレート

    swapChainDesc.SampleDesc.Count = 1;     // MSAAなし（マルチサンプル・アンチエイリアス）
    swapChainDesc.SampleDesc.Quality = 0;   // MSAAの品質（使う場合をデバイス対応の品質レベルを調べる必要あり）

    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;    // バッファ（バックバッファ）の用途
    swapChainDesc.BufferCount = 2;          // 2枚のバッファ
    swapChainDesc.OutputWindow = hwnd;
    swapChainDesc.Windowed = TRUE;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;    // Present後の裏バッファの中身は保証しない
    swapChainDesc.Flags = 0;

    // デバッグレイヤー（間違った使い方をした際に出力してくれる）
    UINT createFlags = 0;
#if defined(_DEBUG)
    createFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    // 現在のGPU（ドライバ）でどの機能レベルが使えるか
    D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0
    };
    D3D_FEATURE_LEVEL createdFeatureLevel = D3D_FEATURE_LEVEL_11_0;

    // Device/DeviceContext/SwapChain を生成
    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        createFlags,
        featureLevels,
        _countof(featureLevels),
        D3D11_SDK_VERSION,
        &swapChainDesc,
        dx.swapChain.ReleaseAndGetAddressOf(),
        dx.device.ReleaseAndGetAddressOf(),
        &createdFeatureLevel,
        dx.deviceContext.ReleaseAndGetAddressOf()
    );
    ThrowIfFailed(hr, "D3D11CreateDeviceAndSwapChain Failed");

    // SwapChain が内部に持っているバックバッファを取り出す
    ComPtr<ID3D11Texture2D> backBuffer;
    hr = dx.swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
    ThrowIfFailed(hr, "Get BackBuffer Failed");

    // RenderTargetView 作成（バックバッファに対して書き込める窓口）
    hr = dx.device->CreateRenderTargetView(backBuffer.Get(), nullptr, dx.renderTargetView.ReleaseAndGetAddressOf());
    ThrowIfFailed(hr, "Create RenderTargetView Failed");

    // OM に出力先（RTV）を設定
    dx.deviceContext->OMSetRenderTargets(1, dx.renderTargetView.GetAddressOf(), nullptr);

    // ビューポート設定（クリップ空間の結果を画面上のどの領域に写すか）
    D3D11_VIEWPORT viewPort = {};
    viewPort.Width = (float)settings.width;
    viewPort.Height = (float)settings.height;
    viewPort.MinDepth = 0.0f;
    viewPort.MaxDepth = 1.0f;
    dx.deviceContext->RSSetViewports(1, &viewPort);
}


// VertexBuffer 作成
ComPtr<ID3D11Buffer> g_vertexBuffer;
static void CreateVertexBuffer(ID3D11Device* device)
{
    // 頂点情報指定
    Vertex vertices[] =
    {
        {  0.0f,  0.5f, 0.0f,  1,0,0,1 }, // 上：赤
        {  0.5f, -0.5f, 0.0f,  0,1,0,1 }, // 右下：緑
        { -0.5f, -0.5f, 0.0f,  0,0,1,1 }, // 左下：青
    };

    // 各設定（どんな用途・性質）
    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.ByteWidth = sizeof(vertices);            // バッファーサイズ（バイト数）
    bufferDesc.Usage = D3D11_USAGE_DEFAULT;             // バッファの使われ方（今回は「 GPU が主に使う」）
    bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;    // このバッファを何としてパイプラインにバインドするか
    bufferDesc.CPUAccessFlags = 0;                      // CPU がこのバッファにアクセスできるか（ 0 はできない）
    bufferDesc.MiscFlags = 0;                           // 特殊な用途の追加フラグ（なし）
    bufferDesc.StructureByteStride = 0;                 // 特殊フラグの要素サイズ（使わないのでもちろんなし）

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = vertices;

    ThrowIfFailed(device->CreateBuffer(&bufferDesc, &initData, &g_vertexBuffer), "Create Vertex Buffer Failed");
}

// VertexShader/PixelShader 作成
static void CreateShaders(Shaders& shaders, Dx11Context& dx)
{
    // VertexShader
    HRESULT hr = dx.device->CreateVertexShader(
        g_BasicVertexShader, std::size(g_BasicVertexShader), NULL, shaders.vertexShader.GetAddressOf());
    ThrowIfFailed(hr, "Create VertexShader Failed");

    // PixelShader
    hr = dx.device->CreatePixelShader(
        g_BasicPixelShader, std::size(g_BasicPixelShader), NULL, shaders.pixelShader.GetAddressOf());
    ThrowIfFailed(hr, "Create PixelShader Failed");
}

// InputLayout 作成
ComPtr<ID3D11InputLayout> g_inputLayout;
static void CreateInputLayout(ID3D11Device* device)
{
    // HLSL ファイルと同じ形式（セマンティクス）
    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    HRESULT hr = device->CreateInputLayout(layout, _countof(layout),
        g_BasicVertexShader, std::size(g_BasicVertexShader), &g_inputLayout);   // 生成されたヘッダーファイルを利用してバイトコードなどを渡す
    ThrowIfFailed(hr, "Create InputLayout Failed");
}


// 描画処理（更新）
static void Render(Dx11Context& dx, Shaders& shaders)
{
    // 1) 画面クリア
    const float clearColor[4] = { 0 / 255.0f, 99 / 255.0f, 181 / 255.0f, 1.0f };
    dx.deviceContext->ClearRenderTargetView(dx.renderTargetView.Get(), clearColor);

    // 2) 出力先（RTV）をセット
    dx.deviceContext->OMSetRenderTargets(1,dx.renderTargetView.GetAddressOf(), nullptr);

    // 3) IA（Input Assembler）設定：Layout / VB / Topology
    dx.deviceContext->IASetInputLayout(g_inputLayout.Get());

    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    ID3D11Buffer* vb = g_vertexBuffer.Get();
    dx.deviceContext->IASetVertexBuffers(0, 1, &vb, &stride, &offset);

    dx.deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);    // 三角形

    // 4) シェーダー設定
    dx.deviceContext->VSSetShader(shaders.vertexShader.Get(), nullptr, 0);
    dx.deviceContext->PSSetShader(shaders.pixelShader.Get(), nullptr, 0);

    // 5) 描き込み
    dx.deviceContext->Draw(3, 0);

    // 6) 表示
    dx.swapChain->Present(1, 0);
}

// エントリーポイント
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow)
{
    static const ProjectSettings settings = {
                .title = L"DirectX11-study",
                .width = 1280,
                .height = 720
    };
    Dx11Context dx = {};
    Shaders shaders = {};


    try {
        // ウィンドウ初期化
        InitWindow(hInstance, nCmdShow, settings);
        // D3D11初期化
        InitD3D11(g_hWnd, settings, dx);
        // シェーダー作成
        CreateShaders(shaders, dx);
        // インプットレイアウト作成
        CreateInputLayout(dx.device.Get());
        // 頂点バッファー作成
        CreateVertexBuffer(dx.device.Get());


        // メインループ（イベントがあるときは処理、それ以外は描画）
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
                Render(dx, shaders);
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

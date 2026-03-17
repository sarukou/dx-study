#include "Renderer.h"

#include "Utility.h"

using namespace Microsoft::WRL;


void Renderer::Initialize(HWND hwnd, const ProjectSettings& settings)
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
        m_swapChain.ReleaseAndGetAddressOf(),
        m_device.ReleaseAndGetAddressOf(),
        &createdFeatureLevel,
        m_deviceContext.ReleaseAndGetAddressOf()
    );
    ThrowIfFailed(hr, "D3D11CreateDeviceAndSwapChain Failed");

    // SwapChain が内部に持っているバックバッファを取り出す
    ComPtr<ID3D11Texture2D> backBuffer;
    hr = m_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
    ThrowIfFailed(hr, "Get BackBuffer Failed");

    // RenderTargetView 作成（バックバッファに対して書き込める窓口）
    hr = m_device->CreateRenderTargetView(backBuffer.Get(), nullptr, m_renderTargetView.ReleaseAndGetAddressOf());
    ThrowIfFailed(hr, "Create RenderTargetView Failed");

    // OM に出力先（RTV）を設定
    m_deviceContext->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf(), nullptr);

    // ビューポート設定（クリップ空間の結果を画面上のどの領域に写すか）
    D3D11_VIEWPORT viewPort = {};
    viewPort.Width = (float)settings.width;
    viewPort.Height = (float)settings.height;
    viewPort.MinDepth = 0.0f;
    viewPort.MaxDepth = 1.0f;
    m_deviceContext->RSSetViewports(1, &viewPort);

    CreateConstantBuffer();
}

void Renderer::CreateConstantBuffer()
{
    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.ByteWidth = sizeof(ConstantPerFrame);
    bufferDesc.Usage = D3D11_USAGE_DYNAMIC;             // 毎フレーム書き換える
    bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;  // 定数バッファ
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE; // CPU から書き込む
    bufferDesc.MiscFlags = 0;
    bufferDesc.StructureByteStride = 0;

    ThrowIfFailed(m_device->CreateBuffer(&bufferDesc, nullptr, m_constantBuffer.GetAddressOf()), "Create Constant Buffer Failed");
}



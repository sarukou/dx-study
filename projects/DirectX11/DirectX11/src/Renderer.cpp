#include "Renderer.h"

#include "Utility.h"

using namespace Microsoft::WRL;
using namespace DirectX;

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
    m_mainViewport = {};
    m_mainViewport.TopLeftX = 0.0f;
    m_mainViewport.TopLeftY = 0.0f;
    m_mainViewport.Width = static_cast<float>(settings.width);
    m_mainViewport.Height = static_cast<float>(settings.height);
    m_mainViewport.MinDepth = 0.0f;
    m_mainViewport.MaxDepth = 1.0f;
    m_deviceContext->RSSetViewports(1, &m_mainViewport);

    CreateConstantBuffer();
    CreateShadowMapResources();
    CreateLightMatrices();
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

void Renderer::CreateShadowMapResources()
{
    // ShadowMap本体のTexture2Dを作成 //
    D3D11_TEXTURE2D_DESC textureDesc = {};
    textureDesc.Width = ShadowMapSize;
    textureDesc.Height = ShadowMapSize;
    textureDesc.MipLevels = 1;
    textureDesc.ArraySize = 1;
    textureDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;    // DSVとSRVの両方から使えるようにTYPELESS
    textureDesc.SampleDesc.Count = 1;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Usage = D3D11_USAGE_DEFAULT;
    textureDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;  // ShadowPassではDSとして書き込み、MainPassではSRとして読み込む
    textureDesc.CPUAccessFlags = 0;
    textureDesc.MiscFlags = 0;

    ThrowIfFailed(m_device->CreateTexture2D(&textureDesc, nullptr, m_shadowMapTexture.ReleaseAndGetAddressOf()), "Create ShadowMap Texture Failed");

    // ShadowMap用DepthStencilViewを作成 //
    D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Texture2D.MipSlice = 0;

    ThrowIfFailed(m_device->CreateDepthStencilView(m_shadowMapTexture.Get(), &dsvDesc, m_shadowMapDSV.ReleaseAndGetAddressOf()), "Create ShadowMap DSV Failed");

    // ShadowMap用ShaderResourceViewを作成 //
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = 1;

    ThrowIfFailed(m_device->CreateShaderResourceView(m_shadowMapTexture.Get(), &srvDesc, m_shadowMapSRV.ReleaseAndGetAddressOf()), "Create ShadowMap SRV Failed");

    // ShadowMap用SamplerStateを作成 //
    D3D11_SAMPLER_DESC samplerDesc = {};
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT; // 範囲外を読んだ場合は BorderColor を返す
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
    samplerDesc.BorderColor[0] = 1.0f;
    samplerDesc.BorderColor[1] = 1.0f;
    samplerDesc.BorderColor[2] = 1.0f;
    samplerDesc.BorderColor[3] = 1.0f;
    samplerDesc.MipLODBias = 0.0f;
    samplerDesc.MaxAnisotropy = 1;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    samplerDesc.MinLOD = 0.0f;
    samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;


    ThrowIfFailed(m_device->CreateSamplerState(&samplerDesc, m_shadowSampler.ReleaseAndGetAddressOf()), "Create ShadowMap Sampler Failed");

    // ShadowMap用Viewportを作成
    m_shadowViewport.TopLeftX = 0.0f;
    m_shadowViewport.TopLeftY = 0.0f;
    m_shadowViewport.Width = static_cast<float>(ShadowMapSize);
    m_shadowViewport.Height = static_cast<float>(ShadowMapSize);
    m_shadowViewport.MinDepth = 0.0f;
    m_shadowViewport.MaxDepth = 1.0f;
}

void Renderer::CreateLightMatrices()
{
    // DirectionalLight用の仮想ライトカメラを作る //
    XMVECTOR lightDirection = XMVector3Normalize(XMVectorSet(0.5f, -1.0f, 1.0f, 0.0f));

    XMVECTOR sceneCenter = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
    XMVECTOR lightPosition = sceneCenter - lightDirection * 10.0f;
    XMVECTOR lightUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    XMMATRIX lightView = XMMatrixLookAtLH(lightPosition, sceneCenter, lightUp);

    // DirectionalLightのため正射影を使う
    // 20 * 20 の範囲をライトから見る
    // nearZ = 0.1f farZ = 50.0f
    XMMATRIX lightProjection = XMMatrixOrthographicLH(20.0f, 20.0f, 0.1f, 50.0f);

    XMMATRIX lightViewProjection = lightView * lightProjection;

    XMStoreFloat4x4(&m_lightViewMatrix, lightView);
    XMStoreFloat4x4(&m_lightProjectionMatrix, lightProjection);
    XMStoreFloat4x4(&m_lightViewProjectionMatrix, lightViewProjection);
}

void Renderer::BeginShadowPass()
{
    // ShadowMapをSRVとしてバインドしていた場合に備えて外す
    ID3D11ShaderResourceView* nullSRV[1] = { nullptr };
    m_deviceContext->PSSetShaderResources(2, 1, nullSRV);

    // ShaderMap用DSVをクリア
    m_deviceContext->ClearDepthStencilView(m_shadowMapDSV.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);

    // RenderTargetは使わず、DepthStencilViewだけをセット（ShadowPassでは色を書かないためRTVは不要）
    m_deviceContext->OMSetRenderTargets(0, nullptr, m_shadowMapDSV.Get());

    // ShadowMapの解像度に合わせたViewportに切り替える
    m_deviceContext->RSSetViewports(1, &m_shadowViewport);
}

void Renderer::BeginMainPass(const float clearColor[4])
{
    // 画面用RTVをクリア
    m_deviceContext->ClearRenderTargetView(m_renderTargetView.Get(), clearColor);

    // 通常描画用のRTVをセット
    ID3D11RenderTargetView* renderTargetView = m_renderTargetView.Get();
    m_deviceContext->OMSetRenderTargets(1, &renderTargetView, nullptr);

    // 画面用Viewportに戻す
    m_deviceContext->RSSetViewports(1, &m_mainViewport);
}
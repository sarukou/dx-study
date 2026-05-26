#pragma once

#include "Types.h"

#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <wrl/client.h>

class Renderer
{
public:
    // 初期化処理
    void Initialize(HWND hwnd, const ProjectSettings& settings);

    // COMオブジェクト取得
    ID3D11Device* GetDevice() const { return m_device.Get(); }
    ID3D11DeviceContext* GetDeviceContext() const { return m_deviceContext.Get(); }
    IDXGISwapChain* GetSwapChain() const { return m_swapChain.Get(); }
    ID3D11RenderTargetView* GetRenderTargetView() const { return m_renderTargetView.Get(); }
    ID3D11Buffer* GetConstantBuffer() const { return m_constantBuffer.Get(); }

    // ShadowMap取得
    ID3D11DepthStencilView* GetShadowMapDSV() const { return m_shadowMapDSV.Get(); }
    ID3D11ShaderResourceView* GetShadowMapSRV() const { return m_shadowMapSRV.Get(); }
    ID3D11SamplerState* GetShadowSampler() const { return m_shadowSampler.Get(); }
    const D3D11_VIEWPORT& GetShadowViewport() const { return m_shadowViewport; }

    // ライト行列取得
    const DirectX::XMFLOAT4X4& GetLightViewMatrix() const { return m_lightViewMatrix; }
    const DirectX::XMFLOAT4X4& GetLightProjectionMatrix() const { return m_lightProjectionMatrix; }
    const DirectX::XMFLOAT4X4& GetLightViewProjectionMatrix() const { return m_lightViewProjectionMatrix; }

    // ShadowPass開始
    void BeginShadowPass();
    // MainPass開始
    void BeginMainPass(const float clearColor[4]);

private:
    // 定数バッファ作成
    void CreateConstantBuffer();

    // ShadowMap用 Texture / DSV /SRV / Sampler 作成
    void CreateShadowMapResources();

    // ShadowMap用 LightView / LightProjection / LightViewProjection 作成
    void CreateLightMatrices();

private:
    // COMオブジェクト
    Microsoft::WRL::ComPtr<ID3D11Device>           m_device;                // GPUリソース（バッファ、テクスチャ、シェーダーなど）を作る
    Microsoft::WRL::ComPtr<ID3D11DeviceContext>    m_deviceContext;         // 作ったリソースを使って描画命令を発行する
    Microsoft::WRL::ComPtr<IDXGISwapChain>         m_swapChain;             // 画面に出すための表裏（バックバッファ）の入れ替え役
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_renderTargetView;      // 書き込み先の窓口（バックバッファに直接は書かず View を作ってOMに渡す）
    Microsoft::WRL::ComPtr<ID3D11Buffer>           m_constantBuffer;        // 定数バッファ

    // ShadowMapResource
    static constexpr UINT ShadowMapSize = 2048;
    Microsoft::WRL::ComPtr<ID3D11Texture2D>          m_shadowMapTexture;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView>   m_shadowMapDSV;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_shadowMapSRV;
    Microsoft::WRL::ComPtr<ID3D11SamplerState>       m_shadowSampler;

    // ShadowMap用ライト行列
    DirectX::XMFLOAT4X4 m_lightViewMatrix{};
    DirectX::XMFLOAT4X4 m_lightProjectionMatrix{};
    DirectX::XMFLOAT4X4 m_lightViewProjectionMatrix{};

    // ViewPort
    D3D11_VIEWPORT m_mainViewport{};
    D3D11_VIEWPORT m_shadowViewport{};
};
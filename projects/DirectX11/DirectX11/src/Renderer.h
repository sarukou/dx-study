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

    ID3D11Device* GetDevice() const { return m_device.Get(); }
    ID3D11DeviceContext* GetDeviceContext() const { return m_deviceContext.Get(); }
    IDXGISwapChain* GetSwapChain() const { return m_swapChain.Get(); }
    ID3D11RenderTargetView* GetRenderTargetView() const { return m_renderTargetView.Get(); }
    ID3D11Buffer* GetConstantBuffer() const { return m_constantBuffer.Get(); }

private:
    // 定数バッファ作成
    void CreateConstantBuffer();

private:
    // COMオブジェクト
    Microsoft::WRL::ComPtr<ID3D11Device>           m_device;              // GPUリソース（バッファ、テクスチャ、シェーダーなど）を作る
    Microsoft::WRL::ComPtr<ID3D11DeviceContext>    m_deviceContext;       // 作ったリソースを使って描画命令を発行する
    Microsoft::WRL::ComPtr<IDXGISwapChain>         m_swapChain;           // 画面に出すための表裏（バックバッファ）の入れ替え役
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_renderTargetView;    // 書き込み先の窓口（バックバッファに直接は書かず View を作ってOMに渡す）
    Microsoft::WRL::ComPtr<ID3D11Buffer>           m_constantBuffer;
};
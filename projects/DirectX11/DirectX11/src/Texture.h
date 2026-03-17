#pragma once

#include <d3d11.h>
#include <wincodec.h>
#include <wrl/client.h>

class Renderer;

class Texture
{
public:
    void Initialize(Renderer& renderer, const wchar_t* filePath);
    // ピクセルシェーダーに SRV と Sampler をバインド
    void Bind(Renderer& renderer, UINT slot = 0) const;

private:
    // テクスチャ作成
    void CreateTextureFromFile(Renderer& renderer, const wchar_t* filePath);
    // サンプラーステート作成
    void CreateSamplerState(Renderer& renderer);

private:
    // COMオブジェクト
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_textureSRV;
    Microsoft::WRL::ComPtr<ID3D11SamplerState> m_samplerState;
};
#pragma once

#include <d3d11.h>
#include <wrl/client.h>

class Renderer;

class Shader
{
public:
    void Initialize(Renderer& renderer);

    // インプットレイアウト、シェーダーをバインド //
    void Bind(Renderer& renderer) const;            // MainPass用
    void BindShadowPass(Renderer& renderer) const;  // ShadowPass用

private:
    // VertexShader/PixelShader 作成 //
    void CreateShaders(Renderer& renderer);         // MainPass用
    void CreateShadowPassShader(Renderer& renderer);// ShadowPass用

    // InputLayout 作成
    void CreateInputLayout(Renderer& renderer);

private:
    // COMオブジェクト //
    // MainPass用
    Microsoft::WRL::ComPtr<ID3D11VertexShader> m_vertexShader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader>  m_pixelShader;
    // ShadowPass用
    Microsoft::WRL::ComPtr<ID3D11VertexShader> m_shadowMapVertexShader;
    // 共通
    Microsoft::WRL::ComPtr<ID3D11InputLayout>  m_inputLayout;
};
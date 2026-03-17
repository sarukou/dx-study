#pragma once

#include <d3d11.h>
#include <wrl/client.h>

class Renderer;

class Shader
{
public:
    void Initialize(Renderer& renderer);
    // インプットレイアウト、シェーダーをバインド
    void Bind(Renderer& renderer) const;

private:
    // VertexShader/PixelShader 作成
    void CreateShaders(Renderer& renderer);
    // InputLayout 作成
    void CreateInputLayout(Renderer& renderer);

private:
    // COMオブジェクト
    Microsoft::WRL::ComPtr<ID3D11VertexShader> m_vertexShader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader>  m_pixelShader;
    Microsoft::WRL::ComPtr<ID3D11InputLayout>  m_inputLayout;
};
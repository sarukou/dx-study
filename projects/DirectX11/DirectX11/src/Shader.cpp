#include "Shader.h"

#include "Renderer.h"
#include "Utility.h"

#include "BasicVertexShader.h" // シェーダーをコンパイルしたヘッダーファイル
#include "BasicPixelShader.h"

using namespace Microsoft::WRL;

void Shader::Initialize(Renderer& renderer)
{
    CreateShaders(renderer);
    CreateInputLayout(renderer);
}

void Shader::CreateShaders(Renderer& renderer)
{
    // VertexShader
    HRESULT hr = renderer.GetDevice()->CreateVertexShader(
        g_BasicVertexShader, std::size(g_BasicVertexShader), NULL, m_vertexShader.GetAddressOf());
    ThrowIfFailed(hr, "Create VertexShader Failed");

    // PixelShader
    hr = renderer.GetDevice()->CreatePixelShader(
        g_BasicPixelShader, std::size(g_BasicPixelShader), NULL, m_pixelShader.GetAddressOf());
    ThrowIfFailed(hr, "Create PixelShader Failed");
}

void Shader::CreateInputLayout(Renderer& renderer)
{
    // HLSL ファイルと同じ形式（セマンティクス）
    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,                            D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TANGENT",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    HRESULT hr = renderer.GetDevice()->CreateInputLayout(layout, _countof(layout),
        g_BasicVertexShader, std::size(g_BasicVertexShader), m_inputLayout.GetAddressOf());   // 生成されたヘッダーファイルを利用してバイトコードなどを渡す
    ThrowIfFailed(hr, "Create InputLayout Failed");
}

void Shader::Bind(Renderer& renderer) const
{
    renderer.GetDeviceContext()->IASetInputLayout(m_inputLayout.Get());
    renderer.GetDeviceContext()->VSSetShader(m_vertexShader.Get(), nullptr, 0);
    renderer.GetDeviceContext()->PSSetShader(m_pixelShader.Get(), nullptr, 0);
}
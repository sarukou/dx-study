#pragma once

#include <d3d11.h>
#include <wrl/client.h>

#include "Types.h"

class Renderer;

class Mesh
{
public:
    void Initialize(Renderer& renderer, const MeshData& meshData);
    // IA にバッファーをバインド
    void Bind(Renderer& renderer) const;

    UINT GetIndexCount() const { return m_indexCount; }

private:
    // VertexBuffer 作成
    void CreateVertexBuffer(Renderer& renderer, const MeshData& meshData);
    // IndexBuffer 作成
    void CreateIndexBuffer(Renderer& renderer, const MeshData& meshData);

private:
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_vertexBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_indexBuffer;
    UINT m_indexCount = 0;
};
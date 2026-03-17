#include "Mesh.h"

#include "Renderer.h"
#include "Utility.h"

using namespace Microsoft::WRL;


void Mesh::Initialize(Renderer& renderer, const MeshData& meshData)
{
    CreateVertexBuffer(renderer, meshData);
    CreateIndexBuffer(renderer, meshData);
    m_indexCount = static_cast<UINT>(meshData.indices.size());
}

void Mesh::CreateVertexBuffer(Renderer& renderer, const MeshData& meshData)
{
    // 各設定（どんな用途・性質）
    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.ByteWidth = static_cast<UINT>(sizeof(Vertex) * meshData.vertices.size());            // バッファーサイズ（バイト数）
    bufferDesc.Usage = D3D11_USAGE_DEFAULT;             // バッファの使われ方（今回は「 GPU が主に使う」）
    bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;    // このバッファを何としてパイプラインにバインドするか（頂点バッファ）
    bufferDesc.CPUAccessFlags = 0;                      // CPU がこのバッファにアクセスできるか（ 0 はできない）
    bufferDesc.MiscFlags = 0;                           // 特殊な用途の追加フラグ（なし）
    bufferDesc.StructureByteStride = 0;                 // 特殊フラグの要素サイズ（使わないのでもちろんなし）

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = meshData.vertices.data();

    ThrowIfFailed(renderer.GetDevice()->CreateBuffer(&bufferDesc, &initData, m_vertexBuffer.GetAddressOf()), "Create Vertex Buffer Failed");
}

void Mesh::CreateIndexBuffer(Renderer& renderer, const MeshData& meshData)
{
    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.ByteWidth = static_cast<UINT>(sizeof(uint32_t) * meshData.indices.size());
    bufferDesc.Usage = D3D11_USAGE_DEFAULT;
    bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    bufferDesc.CPUAccessFlags = 0;
    bufferDesc.MiscFlags = 0;
    bufferDesc.StructureByteStride = 0;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = meshData.indices.data();

    ThrowIfFailed(renderer.GetDevice()->CreateBuffer(&bufferDesc, &initData, m_indexBuffer.GetAddressOf()), "Create Index Buffer Failed");
}

void Mesh::Bind(Renderer& renderer) const
{
    UINT stride = sizeof(Vertex);
    UINT offset = 0;

    ID3D11Buffer* vertexBuffer = m_vertexBuffer.Get();
    renderer.GetDeviceContext()->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
    renderer.GetDeviceContext()->IASetIndexBuffer(m_indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
}
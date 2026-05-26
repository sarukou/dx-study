#pragma once

#include <DirectXMath.h>
#include <string>
#include <vector>
#include <cstdint>

// プロジェクト設定
struct ProjectSettings
{
    const std::wstring title = L"DirectX11-study";
    const int width = 1280;
    const int height = 720;
};

// 頂点情報（CPU→GPUに渡す形）
struct Vertex
{
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT3 normal;
    DirectX::XMFLOAT2 uv;
    DirectX::XMFLOAT3 tangent;
};

// メッシュデータ
struct MeshData
{
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
};

// 定数バッファ用構造体
struct ConstantPerFrame
{
    // WVP 行列
    DirectX::XMFLOAT4X4 worldMatrix;
    DirectX::XMFLOAT4X4 viewMatrix;
    DirectX::XMFLOAT4X4 projectionMatrix;
    DirectX::XMFLOAT4X4 worldViewProjectionMatrix;

    // ShadowMap用：ワールド座標をライト空間へ変換する行列
    DirectX::XMFLOAT4X4 lightViewProjectionMatrix;

    // カメラ
    DirectX::XMFLOAT3 cameraPosition; float padding0;

    // ライト
    DirectX::XMFLOAT3 directional; float padding1;
    DirectX::XMFLOAT3 lightColor;  float padding2;
    DirectX::XMFLOAT3 ambient;     float padding3;

    // NormalMap
    int useNormalMap; DirectX::XMFLOAT3 padding4;

    // PBR用マテリアル
    DirectX::XMFLOAT3 baseColor;
    float metallic;
    float roughness; DirectX::XMFLOAT3 padding5;
};
// デバッグ用
static_assert((sizeof(ConstantPerFrame) % 16) == 0, "Constant buffer size must be 16-byte aligned.");

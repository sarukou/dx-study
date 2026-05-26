cbuffer ConstantBuffer : register(b0)
{
    matrix World;                   // ワールド変換行列
    matrix View;                    // ビュー変換行列
    matrix Projection;              // 透視射影変換行列
    matrix WorldViewProjection;     // WVP行列
    
    matrix LightViewProjection;     // シャドウマップ用行列
    
    float3 CameraPosition; float padding0;
    
    float3 Directional; float padding1;     // ディレクショナルライト
    float3 LightColor;  float padding2;     // ライトの色
    float3 Ambient;     float padding3;     // 環境光
    
    int UseNormalMap; float3 paddong4;    // NormalMap
    
    float3 BaseColor;                   // PBR用
    float Metallic;
    float Roughness; float3 padding5;
}

// 頂点シェーダーの入力データ
struct VSInput
{
    float3 position : POSITION;
    float3 normal   : NORMAL;
    float2 uv       : TEXCOORD;
    float3 tangent  : TANGENT;
};

// 頂点データの出力データ
struct VSOutput
{
    float4 position : SV_POSITION;      // 通常カメラから見たクリップ空間座標
    float3 worldPos : TEXCOORD1;
    float3 normal   : NORMAL;
    float2 uv       : TEXCOORD0;
    float3 tangent  : TANGENT;
    float4 lightClipPos : TEXCOORD2;    // ShadowMap用：ワールド座標をライト視点のクリップ空間に変換した値

};


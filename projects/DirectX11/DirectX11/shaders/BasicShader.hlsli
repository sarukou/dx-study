cbuffer ConstantBuffer : register(b0)
{
    matrix World;                   // ワールド変換行列
    matrix View;                    // ビュー変換行列
    matrix Projection;              // 透視射影変換行列
    matrix WorldViewProjection;     // WVP行列
    
    float3 Directional; float padding0;     // ディレクショナルライト
    float3 LightColor;  float padding1;     // ライトの色
    float3 Ambient;     float padding2;     // 環境光
    
    float UseNormalMap; float3 paddong3;    // NormalMap
    
    float3 BaseColor;                   // PBR用
    float Metallic;
    float Roughness; float3 padding4;
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
    float4 position : SV_POSITION;  // SV_POSITION はラスタライザに渡す必須の位置
    float3 normal   : NORMAL;
    float2 uv       : TEXCOORD;
    float3 tangent  : TANGENT;
};


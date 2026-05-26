#include "BasicShader.hlsli"


VSOutput VSMain(VSInput input)
{
    VSOutput output;
    
    // ローカル座標
    float4 localPosition = float4(input.position, 1.0f);
    // ワールド座標
    float4 worldPosition = mul(localPosition, World);
    
    // 通常カメラ用のクリップ空間座標
    output.position = mul(localPosition, WorldViewProjection);
    
    // PixelShaderでViewベクトル計算などに使うワールド座標
    output.worldPos = worldPosition.xyz;
    
    // ShadowMap用：ライト視点のクリップ空間座標
    output.lightClipPos = mul(worldPosition, LightViewProjection);
    
    // ワールド空間法線
    output.normal = normalize(mul(input.normal, (float3x3)World));
    
    output.uv = input.uv;
    
    output.tangent = normalize(mul(input.tangent, (float3x3)World));
    
	return output;
}
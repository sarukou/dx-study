#include "BasicShader.hlsli"

Texture2D g_albedoTexture : register(t0);
Texture2D g_normalTexture : register(t1);
SamplerState g_sampler0 : register(s0);
SamplerState g_sampler1 : register(s1);

float4 PSMain(VSOutput vsOutput) : SV_TARGET
{
    // テクスチャカラー（元の色）
    float4 textureColor = g_albedoTexture.Sample(g_sampler0, vsOutput.uv);
    
    // 法線
    float3 normal = normalize(vsOutput.normal);
    float3 finalNormal = normal;
    
    // 法線マップを使うか
    if (UseNormalMap != 0)
    {
        float3 normalSample = g_normalTexture.Sample(g_sampler1, vsOutput.uv).rgb;
        normalSample = normalSample * 2.0f - 1.0f;
        
        float3 tangent = normalize(vsOutput.tangent);
        float3 bitangent = normalize(cross(normal, tangent));
        
        float3x3 TBN = float3x3(tangent, bitangent, normal);
        finalNormal = normalize(mul(normalSample, TBN));
    }
    
    // ライト系
    float3 light  = normalize(-Directional);     // 面→光の合わせる
    float ndotl = saturate(dot(finalNormal, light));    // 最終的な法線
    // 拡散反射
    float3 diffuse = LightColor * ndotl;
    float3 lighting     = Ambient + diffuse;    // 環境光も足す
    
    // テクスチャカラーとライトを掛け合わせる
    float3 finalRGB = textureColor.rgb * lighting;
    return float4(finalRGB, textureColor.a);
}
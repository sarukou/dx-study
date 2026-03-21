#include "BasicShader.hlsli"

Texture2D g_albedoTexture : register(t0);
Texture2D g_normalTexture : register(t1);
SamplerState g_sampler0 : register(s0);
SamplerState g_sampler1 : register(s1);

float4 PSMain(VSOutput vsOutput) : SV_TARGET
{
    float3 normal = normalize(vsOutput.normal);
    float3 light  = normalize(-Directional);     // ñ Å®åıÇÃçáÇÌÇπÇÈ
    float ndotl   = saturate(dot(normal, light));

    float3 diffuse = LightColor * ndotl;
    float3 lighting     = Ambient + diffuse;
    
    float4 textureColor = g_albedoTexture.Sample(g_sampler0, vsOutput.uv);
    float4 normalColor = g_normalTexture.Sample(g_sampler1, vsOutput.uv);
    
    float3 finalRGB = textureColor.rgb * lighting;
    
    float3 tangent = normalize(vsOutput.tangent);
    
    
    return float4(normalColor.rgb, 1.0f);
    
    //return float4(finalRGB, textureColor.a);
}
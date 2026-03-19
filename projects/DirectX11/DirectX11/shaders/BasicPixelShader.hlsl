#include "BasicShader.hlsli"

Texture2D gTexture : register(t0);
SamplerState gSampler : register(s0);

float4 PSMain(VSOutput vsOutput) : SV_TARGET
{
    float3 normal = normalize(vsOutput.normal);
    float3 light  = normalize(-Directional);     // ñ Å®åıÇÃçáÇÌÇπÇÈ
    float ndotl   = saturate(dot(normal, light));

    float3 diffuse = LightColor * ndotl;
    float3 lighting     = Ambient + diffuse;
    
    float4 textureColor = gTexture.Sample(gSampler, vsOutput.uv);
    
    float3 finalRGB = textureColor.rgb * lighting;
    
    float3 tangent = normalize(vsOutput.tangent);
    
    return float4(finalRGB, textureColor.a);
}
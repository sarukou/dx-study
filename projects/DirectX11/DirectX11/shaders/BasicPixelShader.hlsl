#include "BasicShader.hlsli"

Texture2D gTexture : register(t0);
SamplerState gSampler : register(s0);

float4 PSMain(VSOutput vsOutput) : SV_TARGET
{
    float3 normal = normalize(vsOutput.normal);
    float3 light  = normalize(-Directional);     // ñ Å®åıÇÃçáÇÌÇπÇÈ
    float ndotl   = saturate(dot(normal, light));

    float3 diffuse = LightColor * ndotl;
    float3 rgb     = (Ambient + diffuse);
    
    return gTexture.Sample(gSampler, vsOutput.uv);
    //return float4(rgb, 1.0f);
}
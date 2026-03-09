#include "BasicShader.hlsli"


float4 PSMain(VSOutput vsOutput) : SV_TARGET
{
    float3 normal = normalize(vsOutput.normal);
    float3 light  = normalize(-Directional);     // ñ Å®åıÇÃçáÇÌÇπÇÈ
    float ndotl   = saturate(dot(normal, light));

    float3 diffuse = LightColor * ndotl;
    float3 rgb     = (Ambient + diffuse);
    
    return float4(vsOutput.uv, 0.0f, 1.0f);
    //return float4(rgb, 1.0f);
}
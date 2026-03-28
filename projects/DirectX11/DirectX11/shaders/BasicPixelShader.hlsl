#include "BasicShader.hlsli"

Texture2D g_albedoTexture : register(t0);
Texture2D g_normalTexture : register(t1);
SamplerState g_sampler0 : register(s0);
SamplerState g_sampler1 : register(s1);

float3 FresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0f - F0) * pow(1.0f - cosTheta, 5.0f);
}

float4 PSMain(VSOutput vsOutput) : SV_TARGET
{
    // テクスチャカラー（元の色）
    float4 textureColor = g_albedoTexture.Sample(g_sampler0, vsOutput.uv);
    float3 albedo = textureColor.rgb * BaseColor;
    
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
    float3 Normal = normalize(finalNormal);
    float3 Light  = normalize(-Directional);     // 面→光の合わせる
    float3 View   = normalize(CameraPosition - vsOutput.worldPos);
    float3 HalfVector = normalize(Light + View);
    float NdotL = saturate(dot(Normal, Light));
    float NdotH = saturate(dot(Normal, HalfVector));
    float VdotH = saturate(dot(View, HalfVector));
    
    // 拡散反射
    float3 diffuse = albedo * LightColor * NdotL;
    
    // Fresnel
    float3 F0 = lerp(float3(0.1f, 0.1f, 0.1f), albedo, Metallic);
    float3 Fresnel = FresnelSchlick(VdotH, F0);
    
    // 簡易Specular
    float3 specPower = 32.0f;
    float spec = pow(NdotH, specPower);
    float3 specular = LightColor * spec * Fresnel *  NdotL;
    
    // 環境光
    float3 ambient = albedo * Ambient;
    
    // 最終的な色
    float3 finalRGB = ambient + diffuse + specular;
    return float4(finalRGB, textureColor.a);
}
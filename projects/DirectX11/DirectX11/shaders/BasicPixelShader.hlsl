#include "BasicShader.hlsli"

Texture2D g_albedoTexture : register(t0);
Texture2D g_normalTexture : register(t1);
SamplerState g_sampler0 : register(s0);
SamplerState g_sampler1 : register(s1);


static const float PI = 3.14159265;


float3 FresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0f - F0) * pow(1.0f - cosTheta, 5.0f);
}

float DistributionGGX(float NdotH, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;

    float NdotH2 = NdotH * NdotH;

    float denom = NdotH2 * (a2 - 1.0f) + 1.0f;
    return a2 / max(PI * denom * denom, 0.0001f);
}

float GeometrySchlickGGX(float NdotX, float roughness)
{
    float r = roughness + 1.0f;
    float k = (r * r) / 8.0f;

    return NdotX / max(NdotX * (1.0f - k) + k, 0.0001f);
}

float GeometrySmith(float NdotV, float NdotL, float roughness)
{
    float ggxV = GeometrySchlickGGX(NdotV, roughness);
    float ggxL = GeometrySchlickGGX(NdotL, roughness);

    return ggxV * ggxL;
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
    
    float3 Normal = normalize(finalNormal);
    float3 Light  = normalize(-Directional);     // 面→光の合わせる
    float3 View   = normalize(CameraPosition - vsOutput.worldPos);
    float3 HalfVector = normalize(Light + View);
    
    float NdotL = saturate(dot(Normal, Light));
    float NdotV = saturate(dot(Normal, View));
    float NdotH = saturate(dot(Normal, HalfVector));
    float VdotH = saturate(dot(View, HalfVector));
    
    // Diffuse
    float3 diffuse = albedo / PI;
    
    // Fresnel
    float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), albedo, Metallic);
    float3 Fresnel = FresnelSchlick(VdotH, F0);
    
    // Roughness
    float roughness = clamp(Roughness, 0.05f, 1.0f);
    
    // GGX + Geometry
    float Distribution = DistributionGGX(NdotH, roughness);
    float Geometry = GeometrySmith(NdotV, NdotL, roughness);
    
    // Cook-Torrance Specular
    float3 numerator = Distribution * Fresnel * Geometry;
    float denominator = max(4.0f * NdotV * NdotL, 0.0001f);
    float3 specular = numerator / denominator;
    
    // Ambient
    float3 ambient = albedo * Ambient;
    
    // 最終的な色
    float3 finalRGB = ambient + (diffuse + specular) * LightColor * NdotL * 3.25f;
    return float4(finalRGB, textureColor.a);
}
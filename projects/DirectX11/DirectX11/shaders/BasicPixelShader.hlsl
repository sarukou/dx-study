#include "BasicShader.hlsli"

Texture2D g_albedoTexture : register(t0);
Texture2D g_normalTexture : register(t1);
Texture2D g_shadowMapTexture : register(t2);

SamplerState g_albedoSampler : register(s0);
SamplerState g_normalSampler : register(s1);
SamplerState g_shadowMapSampler : register(s2);


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

float CalculateShadow(float4 lightClipPos)
{   
    // ライト視点のクリップ空間座標をNDCに変換
    float3 projCoords = lightClipPos.xyz / lightClipPos.w;  // DirectXのNDC： x: -1 ~ 1 / y: -1 ~ 1 / z: 0 ~ 1
    
    float2 shadowUV;    // ShadowMapのUV： u: 0 ~ 1 / v: 0 ~ 1
    shadowUV.x = projCoords.x * 0.5f + 0.5f;
    shadowUV.y = -projCoords.y * 0.5f + 0.5f;
    
    // ShadowMapの範囲外なら影なし
    if (shadowUV.x < 0.0f || shadowUV.x > 1.0f ||
        shadowUV.y < 0.0f || shadowUV.y > 1.0f ||
        projCoords.z < 0.0f || projCoords.z > 1.0f)
    {
        return 1.0f;
    }
    
    // ShadowMapに保存されている「ライトから見た最前面の深度」
    float closestDepth = g_shadowMapTexture.Sample(g_shadowMapSampler, shadowUV).r;
    
    // 今描画しているピクセルの「ライトから見た深度」
    float currentDepth = projCoords.z;
    
    if (currentDepth > closestDepth)
    {
        return 0.35f; // 影にする
    }
    
    return 1.0f; //光が当たっている
}

float4 PSMain(VSOutput vsOutput) : SV_TARGET
{
    // テクスチャカラー（元の色）
    float4 textureColor = g_albedoTexture.Sample(g_albedoSampler, vsOutput.uv);
    float3 albedo = textureColor.rgb * BaseColor;
    
    // 法線
    float3 normal = normalize(vsOutput.normal);
    float3 finalNormal = normal;
    
    // 法線マップを使うか
    if (UseNormalMap != 0)
    {
        float3 normalSample = g_normalTexture.Sample(g_normalSampler, vsOutput.uv).rgb;
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
    
    // Roughness
    float roughness = clamp(Roughness, 0.05f, 1.0f);
    // Metallic
    float metallic = saturate(Metallic);

    // Fresnel
    float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), albedo, Metallic);
    float3 Fresnel = FresnelSchlick(VdotH, F0);
    
    // GGX + Geometry
    float Distribution = DistributionGGX(NdotH, roughness);
    float Geometry = GeometrySmith(NdotV, NdotL, roughness);
    
    // Cook-Torrance Specular
    float3 numerator = Distribution * Fresnel * Geometry;
    float denominator = max(4.0f * NdotV * NdotL, 0.0001f);
    float3 specular = numerator / denominator;
    
    // Diffuse（Metallic で Diffuse, Specular 配分を変更）
    float3 kS = Fresnel;
    float3 kD = 1.0f - kS;
    kD *= (1.0f - metallic);
    float3 diffuse = kD * albedo / PI;
    
    // Ambient
    float3 ambient = albedo * Ambient;
    
    // ShadowMapによる影係数
    float shadow = CalculateShadow(vsOutput.lightClipPos);
    
    // DirectLight成分
    float directLighting = (diffuse + specular) * LightColor * NdotL;
    
    // 最終的な色
    // 影の中でも環境光は届いている扱いにするため ambientには掛けない
    float3 finalRGB = ambient + shadow * directLighting;
    return float4(finalRGB, textureColor.a);
}
Texture2D shadowMapTexture : register(t2);
SamplerState shadowMapSampler : register(s2);

struct VSOutput
{
    float4 position : SV_POSITION;
    float2 uv       : TEXCOORD0;
};

float4 main(VSOutput vsOutput) : SV_TARGET
{
    float depth = shadowMapTexture.Sample(shadowMapSampler, vsOutput.uv).r;

    // 見やすくするために強調
    depth = saturate((depth - 0.7f) * 4.0f);
    
    // 深度値をグレースケール表示する。
    // 近いほど黒、遠いほど白。
    return float4(depth, depth, depth, 1.0f);
}
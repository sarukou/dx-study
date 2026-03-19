#include "BasicShader.hlsli"


VSOutput VSMain(VSInput input)
{
    VSOutput output;
    output.position = mul(float4(input.position, 1.0f), WorldViewProjection);
    
    // ƒ[ƒ‹ƒh‹óŠÔ–@ü
    output.normal = normalize(mul(input.normal, (float3x3) World));
    
    output.uv = input.uv;
    
    output.tangent = normalize(mul(input.tangent, (float3x3) World));
	return output;
}
#include "BasicShader.hlsli"

// ¡‰ñ‚ÍÀ•W‚ğ‚»‚Ì‚Ü‚Ü•Ô‚·
VSOutput VSMain(VSInput input)
{
    VSOutput output;
    output.position = mul(float4(input.position, 1.0f), WorldViewProjection);
    output.color = input.color;
	return output;
}
#include "BasicShader.hlsli"

// ¡‰ñ‚ÍÀ•W‚ğ‚»‚Ì‚Ü‚Ü•Ô‚·
VSOutput VSMain(VSInput input)
{
    VSOutput output;
    output.position = float4(input.position, 1.0);
    output.color = input.color;
	return output;
}
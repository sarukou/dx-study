#include "BasicShader.hlsli"


VSOutput main(VSInput input)
{
    VSOutput output;
    output.position = mul(float4(input.position, 1.0f), WorldViewProjection);
    return output;
}
#include "BasicShader.hlsli"

// ¡‰ñ‚ÍF‚ğ‚»‚Ì‚Ü‚Ü•Ô‚·
float4 PSMain(VSOutput vsOutput) : SV_TARGET
{
    return vsOutput.color;
}
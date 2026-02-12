#include "BasicShader.hlsli"

// ¡‰ñ‚ÍF‚ğ‚»‚Ì‚Ü‚Ü•Ô‚·
float4 PSMain(VSOutput vsOut) : SV_TARGET
{
    return vsOut.color;
}
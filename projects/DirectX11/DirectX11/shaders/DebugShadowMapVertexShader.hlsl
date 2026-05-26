struct VSOutput
{
    float4 position : SV_POSITION;
    float2 uv       : TEXCOORD0;
};

VSOutput main(uint vertexId : SV_VertexID)
{
    VSOutput output;
    
    // 画面左上に表示する小さい矩形
    // NDC座標：x : -1 ~ 1 y : -1 ~ 1
    // 左上 40% * 40% に表示
    float2 positions[6] =
    {
        float2(-1.0f, 1.0f),
        float2(-0.2f, 1.0f),
        float2(-1.0f, 0.2f),
        
        float2(-1.0f, 0.2f),
        float2(-0.2f, 1.0f),
        float2(-0.2f, 0.2f),
    };

    float2 uvs[6] =
    {
        float2(0.0f, 0.0f),
        float2(1.0f, 0.0f),
        float2(0.0f, 1.0f),
        
        float2(0.0f, 1.0f),
        float2(1.0f, 0.0f),
        float2(1.0f, 1.0f),
    };

    output.position = float4(positions[vertexId], 0.0f, 1.0f);
    output.uv = uvs[vertexId];
    return output;
}
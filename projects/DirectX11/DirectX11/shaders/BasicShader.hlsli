// 頂点シェーダーの入力データ
struct VSInput
{
    float3 position : POSITION;
    float4 color : COLOR;
};

// 頂点データの出力データ
struct VSOutput
{
    float4 position : SV_POSITION;  // SV_POSITION はラスタライザに渡す必須の位置
    float4 color : COLOR;
};


cbuffer constants : register(b0)
{
    float4x4 orthoMatrix;
};

struct VS_Input {
    float2 pos : POS;
    float3 pos1 : POSA_INSTANCE;
    float3 pos2 : POSB_INSTANCE;
    float4 color1 : COLOR_INSTANCE;
};

struct VS_Output {
    float4 pos : SV_POSITION;
    float4 color : COLOR;
};

VS_Output vs_main(VS_Input input)
{
    VS_Output output;

    float2 pos_ = input.pos;

    float3 pos = pos_.x * input.pos1 + pos_.y * input.pos2;

    output.pos = mul(orthoMatrix, float4(pos, 1.0f));

    output.color = input.color1;
    return output;
}

float4 ps_main(VS_Output input) : SV_Target
{
    return input.color;
    
}

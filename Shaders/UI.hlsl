// UI.hlsl
cbuffer UIConstants : register(b0)
{
    float4x4 OrthoProj; // Orthographic Projection
};

struct VSInput
{
    float2 Position : POSITION; // 화면 픽셀 좌표 (0,0 ~ width,height)
    float2 UV : TEXCOORD0;
    float4 Color : COLOR0;
};

struct PSInput
{
    float4 Position : SV_POSITION;
    float2 UV : TEXCOORD0;
    float4 Color : COLOR0;
};

Texture2D gTexture : register(t0);
SamplerState gSampler : register(s0);

PSInput VS(VSInput input)
{
    PSInput output;
    
    // 화면 좌표 → NDC (-1 ~ 1) 변환
    float4 pos = float4(input.Position, 0.0f, 1.0f);
    output.Position = mul(pos, OrthoProj);
    output.UV = input.UV;
    output.Color = input.Color;
    
    return output;
}

float4 PS(PSInput input) : SV_TARGET
{
    float4 texColor = gTexture.Sample(gSampler, input.UV);
    return texColor * input.Color; // 알파 블렌딩용
    //return input.Color;
}
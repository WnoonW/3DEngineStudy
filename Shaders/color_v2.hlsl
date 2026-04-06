//***************************************************************************************
// color.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//
// Transforms and colors geometry.
//***************************************************************************************

cbuffer cbPerObject : register(b0)
{
    float4x4 gWorldViewProj;
    int Selected;
};


Texture2D gTexture : register(t0);
SamplerState gSampler : register(s0);

struct VertexIn
{
    float3 PosL : POSITION;
    float2 TEXCOORD : TEXCOORD;
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float2 TEXCOORD : TEXCOORD;
};

VertexOut VS(VertexIn vin)
{
    VertexOut vout;
	
	// Transform to homogeneous clip space.
    vout.PosH = mul(float4(vin.PosL, 1.0f), gWorldViewProj);
	
	// Just pass vertex color into the pixel shader.
    vout.TEXCOORD = vin.TEXCOORD;
    
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    float4 tex = gTexture.Sample(gSampler, pin.TEXCOORD);
    
    // 텍스처가 제대로 안 읽히면 밝은 색으로 표시
    if (all(tex.rgb == 0.0))
        return float4(1.0, 0.0, 1.0, 1.0); // 마젠타 = 텍스처 실패 신호
    
  
    //if (Selected)
    //{
    //   return tex * float4(1.5, 1.5, 1.5, 1.0);
    //}
    
    
    //return tex * float4(1.2, 1.0, 0.9, 1.0);
    
    
    if (Selected)
    {
       return float4(1.5, 1.5, 1.5, 1.0);
    }
    return float4(1.2, 1.0, 0.9, 1.0);
}



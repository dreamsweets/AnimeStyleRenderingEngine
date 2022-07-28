cbuffer PerObject : register(b0)
{
    float4x4 world;
};

cbuffer PerFrame : register(b1)
{
    float3 cameraPosition;
    float padding1;
    float4x4 view;
    float4x4 inverseView;
    float4x4 projection;
};

struct VSInput
{
    float3 pos : POSITION;
    float4 color : COLOR;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
    float3 bitangent : BITANGENT;
    float3 tangent : TANGENT;
    float4 Weight : BLENDWEIGHT;
    float4 BoneIndices : BLENDINDICES;
};

struct PSInput
{
    float4 pos : SV_POSITION;
    float3 uv : TEXCOORD;
};

struct PSOutput
{
    float4 color : SV_Target;
};

PSInput VSMain(VSInput input)
{
    PSInput vs_out = (PSInput)0;
    
    float4x4 wvp = mul(mul(world, view), projection);
    
    vs_out.pos = mul(float4(input.pos, 1.f), wvp).xyww;
    vs_out.uv = input.pos;
    
    return vs_out;
}

PSOutput PSMain(PSInput input) : SV_TARGET
{
    PSOutput output = (PSOutput)0;
    
    float4 topColor = float4(6.f / 255.f, 70.f / 255.f, 194.f / 255.f, 0.f);
    float4 midColor = float4(168.f / 255.f, 148.f / 255.f, 117.f / 255.f, 0.f);
    float4 botColor = float4(102.f / 255.f, 218.f / 255.f, 253.f / 255.f, 0.f);
    if (input.uv.y > 0)
    {
        output.color = lerp(topColor, midColor, 1 - input.uv.y);
    }
    else
    {
        output.color = lerp(midColor, botColor, -input.uv.y);
    }
    
    return output;
}
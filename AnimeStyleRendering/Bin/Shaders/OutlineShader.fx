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

cbuffer OutlineInfo : register(b2)
{
    float amount;
    float3 outlineColor;
}

struct VSInput
{
    float3 pos : POSITION;
    float4 color : COLOR;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
    float3 bitangent : BITANGENT;
    float3 tangent : TANGENT;
};

struct PSInput
{
    float4 pos : SV_POSITION;
    float4 color : COLOR;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
    float3 bitangent : BITANGENT;
    float3 tangent : TANGENT;
};

struct PSOutput
{
    float4 color : SV_Target;
};

PSInput VSMain(VSInput vs_in)
{
    PSInput vs_out;
    vs_out.color = vs_in.color;
    vs_out.uv = vs_in.uv;
	
    float4x4 scaleOffsetMat;
    scaleOffsetMat[0][0] = 1.f + amount;
    scaleOffsetMat[0][1] = 0.f;
    scaleOffsetMat[0][2] = 0.f;
    scaleOffsetMat[0][3] = 0.f;
    
    scaleOffsetMat[1][0] = 0.f;
    scaleOffsetMat[1][1] = 1.f + amount;
    scaleOffsetMat[1][2] = 0.f;
    scaleOffsetMat[1][3] = 0.f;
    
    scaleOffsetMat[2][0] = 0.f;
    scaleOffsetMat[2][1] = 0.f;
    scaleOffsetMat[2][2] = 1.f + amount;
    scaleOffsetMat[2][3] = 0.f;
    
    scaleOffsetMat[3][0] = 0.0f;
    scaleOffsetMat[3][1] = 0.0f;
    scaleOffsetMat[3][2] = 0.0f;
    scaleOffsetMat[3][3] = 1.0f;
    
    float4x4 scaledWorld = mul(world, scaleOffsetMat);
    float4x4 worldView = mul(scaledWorld, view);
    float4x4 wvp = mul(mul(scaledWorld, view), projection);
	

    vs_out.pos = mul(float4(vs_in.pos, 1.0f), wvp);
    vs_out.color = float4(outlineColor, 1.0f);
    vs_out.normal = normalize(mul(float4(vs_in.normal, 0.0f), world).xyz);
    vs_out.tangent = normalize(mul(float4(vs_in.tangent, 1.0f), worldView).xyz);
    vs_out.bitangent = normalize(mul(float4(vs_in.bitangent, 1.0f), worldView).xyz);
	
    return vs_out;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    return input.color;
}
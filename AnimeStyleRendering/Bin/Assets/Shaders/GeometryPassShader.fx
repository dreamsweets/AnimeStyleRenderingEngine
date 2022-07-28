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
    float4 worldpos : POSITION;
    float4 pos : SV_POSITION;
    float4 color : COLOR;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
    float3 bitangent : BITANGENT;
    float3 tangent : TANGENT;
};

struct PSOutput
{
    float4 WorldPos : SV_TARGET0;
    float4 Diffuse : SV_TARGET1;
    float4 Normal : SV_TARGET2;
    float4 Bitangent : SV_TARGET3;
    float4 Tangent : SV_TARGET4;
};

PSInput VSMain(VSInput vs_in)
{
    PSInput vs_out;
    vs_out.color = vs_in.color;
    vs_out.uv = vs_in.uv;
	
    float4x4 worldView = mul(world, view);
    float4x4 wvp = mul(mul(world, view), projection);
	
    vs_out.worldpos = mul(float4(vs_in.pos, 1.f), world);
    vs_out.pos = mul(float4(vs_in.pos, 1.0f), wvp);
    vs_out.normal = normalize(mul(float4(vs_in.normal, 0.0f), world).xyz);
    vs_out.tangent = normalize(mul(float4(vs_in.tangent, 1.0f), worldView).xyz);
    vs_out.bitangent = normalize(mul(float4(vs_in.bitangent, 1.0f), worldView).xyz);
	
    return vs_out;
}

SamplerState Sampler;
Texture2D Albedo : register(t0);

PSOutput PSMain(PSInput input)
{
    PSOutput output;
    output.WorldPos = input.worldpos;
    output.Diffuse = Albedo.Sample(Sampler, input.uv);
    output.Normal = float4(input.normal, 1.f);
    output.Bitangent = float4(input.bitangent, 1.f);
    output.Tangent = float4(input.tangent, 1.f);
    output.Diffuse.a = 1.f;
    return output;
}
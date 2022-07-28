StructuredBuffer<matrix> g_SkinningBoneMatrixArray;

struct SkinningInfo
{
    float3 Pos;
    float3 Normal;
    float3 Tangent;
    float3 Bitangent;
};

matrix GetBoneMatrix(int Index)
{
    return g_SkinningBoneMatrixArray[Index];
}

SkinningInfo Skinning(float3 Pos, float3 Normal, float3 Tangent, float3 Bitangent,
	float4 Weight, float4 Index)
{
    SkinningInfo Info = (SkinningInfo) 0;

    for (int i = 0; i < 4; ++i)
    {
        if (Weight[i] == 0.f)
            continue;

        matrix matBone = GetBoneMatrix((int) Index[i]);

        Info.Pos += (mul(float4(Pos, 1.f), matBone)).xyz * Weight[i];
        Info.Normal += (mul(float4(Normal, 0.f), matBone)).xyz * Weight[i];
        Info.Tangent += (mul(float4(Tangent, 0.f), matBone)).xyz * Weight[i];
        Info.Bitangent += (mul(float4(Bitangent, 0.f), matBone)).xyz * Weight[i];
    }

    Info.Normal = normalize(Info.Normal);
    Info.Tangent = normalize(Info.Tangent);
    Info.Bitangent = normalize(Info.Bitangent);

    return Info;
}

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
    
    
    SkinningInfo Info = Skinning(vs_in.pos, vs_in.normal, vs_in.tangent, vs_in.bitangent, vs_in.Weight, vs_in.BoneIndices);
    
    vs_out.worldpos = mul(float4(Info.Pos, 1.f), world);
    vs_out.pos = mul(float4(Info.Pos, 1.0f), wvp);
    vs_out.normal = normalize(mul(float4(Info.Normal, 0.0f), world).xyz);
    vs_out.tangent = normalize(mul(float4(Info.Tangent, 1.0f), worldView).xyz);
    vs_out.bitangent = normalize(mul(float4(Info.Bitangent, 1.0f), worldView).xyz);
	
    return vs_out;
}

SamplerState Sampler;
Texture2D Albedo;

PSOutput PSMain(PSInput input)
{
    PSOutput output;
    output.WorldPos = input.worldpos;
    output.Diffuse = Albedo.Sample(Sampler, input.uv);
    output.Normal = float4(input.normal, 1.f);
    output.Bitangent = float4(input.bitangent, 1.f);
    output.Tangent = float4(input.tangent, 1.f);
    
    return output;
}
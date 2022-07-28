SamplerState Sampler;

static const float2 g_NullPos[4] =
{
    float2(-1.f, 1.f),
	float2(1.f, 1.f),
	float2(-1.f, -1.f),
	float2(1.f, -1.f)
};

static const float2 g_NullUV[4] =
{
    float2(0.f, 0.f),
	float2(1.f, 0.f),
	float2(0.f, 1.f),
	float2(1.f, 1.f)
};

float SqrDistance(float3 pos1, float3 pos2)
{
    float3 distVec = (pos2 - pos1);
    return pow(distVec.x, 2) + pow(distVec.y, 2) + pow(distVec.z, 2);
}

float3 GetShadeColor(float3 Color)
{
    float3 outColor = (float3) 0;
    outColor.r = -0.03704 + 1.01913 * Color.r - 1.58259 * pow(Color.r, 2) + 1.39386 * pow(Color.r, 3);
    outColor.g = -0.2375 + 2.3285 * Color.g - 3.9943 * pow(Color.g, 2) + 2.6192 * pow(Color.g, 3);
    outColor.b = 0.4244 + -4.3663 * Color.b + 16.6827 * pow(Color.b, 2) - 22.3293 * pow(Color.b, 3) + 10.3923 * pow(Color.b, 4);
    
    return outColor;
}

struct VS_OUTPUT_NULLBUFFER
{
    float4 Pos : SV_POSITION;
    float2 UV : TEXCOORD;
    float4 ProjPos : POSITION;
};

VS_OUTPUT_NULLBUFFER VSMain(uint VertexID : SV_VertexID)
{
    VS_OUTPUT_NULLBUFFER output = (VS_OUTPUT_NULLBUFFER) 0;

    output.ProjPos = float4(g_NullPos[VertexID], 0.f, 1.f);
    output.Pos = output.ProjPos;
    output.UV = g_NullUV[VertexID];

    return output;
}

Texture2D LightPassResult : register(t1);

float4 PSMain(VS_OUTPUT_NULLBUFFER input) : SV_TARGET
{
    float4 output = float4(0, 0, 0, 1);
    float4 LightPassColor = LightPassResult.Sample(Sampler, input.UV);
    if (LightPassColor.a == 0.f) clip(-1);
    
    LightPassColor.a = 1.f;
    output = LightPassColor;
    return output;
}
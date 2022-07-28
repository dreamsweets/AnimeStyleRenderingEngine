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

Texture2D PostProcessing;

float4 PSMain(VS_OUTPUT_NULLBUFFER input) : SV_TARGET
{
    float4 output = float4(0, 0, 0, 1);
    float4 Color = PostProcessing.Sample(Sampler, input.UV);
    
    output = Color;
    
    return output;
}
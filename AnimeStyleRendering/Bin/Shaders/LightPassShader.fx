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

struct LightData
{
    uint lightType;
    float3 position;

    float range;
    float3 direction;

    float spotAngle;
    float3 color;
};

cbuffer LightPassData : register(b0)
{
    uint lightCount;
    float3 padding;
    LightData lights[32];
};

Texture2D WorldPos : register(t1);
Texture2D Diffuse : register(t2);
Texture2D Normal : register(t3);
Texture2D Bitangent : register(t4);
Texture2D Tangent : register(t5);
Texture2D Depth : register(t6);
Texture2D RaytracingResult : register(t7);

float4 PSMain(VS_OUTPUT_NULLBUFFER input) : SV_TARGET
{
    float4 WorldPosition = WorldPos.Sample(Sampler, input.UV);
    float4 DiffuseColor = Diffuse.Sample(Sampler, input.UV);
    float4 normal = Normal.Sample(Sampler, input.UV);
    
    if (DiffuseColor.a == 0.f) clip(-1);
    
    float4 output = float4(0, 0, 0, 1);
    
    float shadow = RaytracingResult.Sample(Sampler, input.UV).r;
    
    shadow = (shadow * 0.5) + 0.5; // [0,1] -> [0.5, 1]
    
    float3 ShadeColor = GetShadeColor(DiffuseColor.rgb);
    
    for (uint i = 0; i < lightCount; ++i)
    {
        LightData light = lights[i];
        
        if(light.lightType == 1u) // Directional Light == 1
        {
            float3 dir = -light.direction;
            float intensity = normalize(dot(normal.xyz, dir));
            
            intensity = clamp(intensity, 0, 1);
            
            if (0 < intensity && intensity < 0.05)
                output += float4(GetShadeColor(ShadeColor), 0.f);
            ////else if (intensity < 0.1)
            ////    output += float4(GetShadeColor(GetShadeColor(DiffuseColor.rgb * light.color)), 0.f);
            //else
            //    output += float4(DiffuseColor.rgb * light.color , 0.f);
            if(shadow < 0.9f)
                output += float4(ShadeColor * light.color, 0.f);
            else
                output += float4(DiffuseColor.rgb * light.color, 0.f);
            saturate(output);
        }
        
        if (light.lightType == 2u)
        {
            float dist = SqrDistance(light.position, WorldPosition.xyz);
            float range = pow(light.range, 2);
            if (dist <= range)
            {
                float pointLightIntensity = 1 - (dist / range);
                output += float4((light.color * pointLightIntensity  * shadow), 0.f);
                saturate(output);
            }
        }
    }
    
    //output *= shadow;
    
    return output;
}
RaytracingAccelerationStructure RTScene : register(t0);
RWTexture2D<float4> OuputTexture : register(u1);
Texture2D<float> PrevResult : register(t2);

struct CameraData
{
    matrix view;
    matrix proj;
    matrix InverseView;
    matrix InverseProj;
    
    float3 position;
    float nearZ;
    float farZ;
    float3 CameraFront;
};

struct LightData
{
    uint lightType;
    float3 position;

    float range;
    float3 direction;

    float spotAngle;
    float3 color;
};

struct SceneData
{
    uint lightCount;
    float3 padding;
    CameraData Camera;
    LightData Lights[32];
};

ConstantBuffer<SceneData> PerFrame : register(b2);

struct CameraHit
{
    float shadowFactor;
    float3 color;
};

struct ShadowHit
{
    bool hit;
};
float3 CameraPosition()
{
    return PerFrame.Camera.position.xyz;
}
float3 CameraRight()
{
    return PerFrame.Camera.view[0].xyz;
}
float3 CameraUp()
{
    return PerFrame.Camera.view[1].xyz;
}
float3 CameraForward()
{
    return -PerFrame.Camera.view[2].xyz;
}
float CameraFocalLength()
{
    return abs(PerFrame.Camera.proj[1][1]);
}

float3 HitPosition()
{
    return WorldRayOrigin() + WorldRayDirection() * (RayTCurrent() - 0.002f);
}

float SampleDifferentialF(int2 idx, out float center, out float diff)
{
    int2 dim;
    PrevResult.GetDimensions(dim.x, dim.y);

    center = PrevResult[idx].x;
    diff = 0;

    // 4 samples for now. an option for more samples may be needed. it can make an unignorable difference (both quality and speed).
    diff += abs(PrevResult[clamp(idx + int2(-1, 0), int2(0, 0), dim - 1)].x - center);
    diff += abs(PrevResult[clamp(idx + int2(1, 0), int2(0, 0), dim - 1)].x - center);
    diff += abs(PrevResult[clamp(idx + int2(0, -1), int2(0, 0), dim - 1)].x - center);
    diff += abs(PrevResult[clamp(idx + int2(0, 1), int2(0, 0), dim - 1)].x - center);
    return diff;
}

uint SampleDifferentialI(int2 idx, out uint center, out uint diff)
{
    int2 dim;
    PrevResult.GetDimensions(dim.x, dim.y);

    center = asuint(PrevResult[idx].x);
    diff = 0;

    // 4 samples for now. an option for more samples may be needed. it can make an unignorable difference (both quality and speed).
    diff += abs(asuint(PrevResult[clamp(idx + int2(-1, 0), int2(0, 0), dim - 1)].x) - center);
    diff += abs(asuint(PrevResult[clamp(idx + int2(1, 0), int2(0, 0), dim - 1)].x) - center);
    diff += abs(asuint(PrevResult[clamp(idx + int2(0, -1), int2(0, 0), dim - 1)].x) - center);
    diff += abs(asuint(PrevResult[clamp(idx + int2(0, 1), int2(0, 0), dim - 1)].x) - center);
    return diff;
}

float angle_between(float3 a, float3 b)
{
    return acos(clamp(dot(a, b), 0, 1));
}

void GenerateCameraRay(uint2 index, out float3 origin, out float3 direction)
{
    float2 Resolutions = float2(DispatchRaysDimensions().xy);
    float2 xy = (float2)index + 0.5f; // center in the middle of the pixel
    float2 screenPos = (xy / Resolutions * 2.0) - 1.0; // x : 0 ~ 1920, y : 0~ 1200 -> x : -960 ~ 959, y : -600 ~ 599 
    
    // Invert Y for DirectX-style coordinates
    screenPos.y = -screenPos.y;
    
    // Unproject into a ray
    float4 unprojected = mul(mul(float4(screenPos, 0, 1), PerFrame.Camera.InverseProj), PerFrame.Camera.InverseView);
    float3 world = unprojected.xyz / unprojected.w;
    origin = PerFrame.Camera.position;
    direction = normalize(world - origin);
}

RayDesc GetCameraRay(float2 offset = 0.0f)
{
    uint2 ScreenIdx = DispatchRaysIndex().xy;
    float2 Resolutions = float2(DispatchRaysDimensions().xy);
    
    float2 xy = (float2) ScreenIdx + 0.5f + offset; // center in the middle of the pixel
    float2 screenPos = (xy / Resolutions * 2.0) - 1.0; // x : 0 ~ 1920, y : 0~ 1200 -> x : -960 ~ 959, y : -600 ~ 599 
    
    // Invert Y for DirectX-style coordinates
    screenPos.y = -screenPos.y;
    
    // Unproject into a ray
    float4 unprojected = mul(mul(float4(screenPos, 0, 1), PerFrame.Camera.InverseProj), PerFrame.Camera.InverseView);
    float3 world = unprojected.xyz / unprojected.w;
    float3 origin = PerFrame.Camera.position;
    float3 direction = normalize(world - origin);
    
    RayDesc Ray;
    Ray.Origin = origin;
    Ray.Direction = direction;
    Ray.TMin = 0;
    Ray.TMax = 100000;
    
    return Ray;
}

CameraHit ShootCameraRay(float2 offset = 0.0f)
{
    RayDesc ray = GetCameraRay(offset);
    
    CameraHit payload;
    payload.shadowFactor = 0.0f;
    payload.color = float3(1.0f, 1.0f, 1.0f);
    
    uint ray_flags = /*RAY_FLAG_CULL_BACK_FACING_TRIANGLES*/0;
    
    TraceRay(RTScene, ray_flags, 0xFF, 0 /* ray index*/, 0, 0, ray, payload);
    
    return payload;
}

bool ShootShadowRay(uint flags, in RayDesc ray, inout ShadowHit payload)
{
    TraceRay(RTScene, flags, 0xFF, 1, 0, 1, ray, payload);
    return payload.hit;
}

// Raygen Shader

//[shader("raygeneration")]
//void RayGeneration_02_AdaptiveSampling()
//{
//    uint2 screen_idx = DispatchRaysIndex().xy;
//    uint2 cur_dim = DispatchRaysDimensions().xy;
//    uint2 pre_dim;
//    PrevResult.GetDimensions(pre_dim.x, pre_dim.y);
//    int2 pre_idx = (int2) ((float2) screen_idx * ((float2) pre_dim / (float2) cur_dim));
    
//    float center, diff;
//    SampleDifferentialF(pre_idx, center, diff);
//    OuputTexture[screen_idx] = diff == 0 ? center : ShootCameraRay().color.r;
//}

[shader("raygeneration")]
void RayGeneration_03_Antialiasing()
{
    uint2 idx = DispatchRaysIndex().xy;

    float center, diff;
    SampleDifferentialF(idx, center, diff);

    if (diff == 0)
    {
        OuputTexture[idx] = center;
    }
    else
    {
            // todo: make offset values shader parameter
        const int N = 4;
        const float d = 0.333f;
        float2 offsets[N] =
        {
            float2(d, d), float2(-d, d), float2(-d, -d), float2(d, -d)
        };

        float total = asfloat(center);
        for (int i = 0; i < N; ++i)
            total += ShootCameraRay(offsets[i]).shadowFactor;
        OuputTexture[idx] = total / (float) (N + 1);
    }
}
//Miss Shaders 

[shader("miss")] // Miss 0
void CameraRayMiss(inout CameraHit payload)
{
    payload.color = float3(0.0, 0.0, 0.0);
}

[shader("miss")] // Miss 1
void ShadowRayMiss(inout ShadowHit payload)
{
    payload.hit = false;
}

//Hit Shaders

[shader("closesthit")] // HitGroup 0 : chs
void CameraRayClosestHit(inout CameraHit payload, in BuiltInTriangleIntersectionAttributes attribs)
{
    ShadowHit shadowPayload;
    shadowPayload.hit = false;
    
    float ls = 1.0f / PerFrame.lightCount;
    
    uint ray_flags_common = 0;
    
    int li;
    for (li = 0; li < PerFrame.lightCount; ++li)
    {
        LightData light = PerFrame.Lights[li];
      
        uint ray_flags = ray_flags_common;
    
        bool hit = false;
        if (light.lightType == 1) // Directional
        {
            // directional light
            RayDesc ray;
            ray.Origin = HitPosition();
            ray.Direction = -light.direction.xyz;
            ray.TMin = 0.0f;
            ray.TMax = PerFrame.Camera.farZ;
            hit = ShootShadowRay(ray_flags, ray, shadowPayload);
        }
        else if (light.lightType == 2) // Point
        {
            // point light
            float3 pos = HitPosition();
            float3 dir = normalize(light.position - pos);
            float distance = length(light.position - pos);
    
            if (distance <= light.range)
            {
                RayDesc ray;
                ray.Origin = pos;
                ray.Direction = dir;
                ray.TMin = 0.0f;
                ray.TMax = distance;
                hit = ShootShadowRay(ray_flags, ray, shadowPayload);
            }
            else
                continue;
            
        }
        else if (light.lightType == 3) // Spot
        {
            // spot light
            float3 pos = HitPosition();
            float3 dir = normalize(light.position - pos);
            float distance = length(light.position - pos);
            if (distance <= light.range && angle_between(-dir, light.direction) * 2.0f <= light.spotAngle)
            {
                RayDesc ray;
                ray.Origin = pos;
                ray.Direction = dir;
                ray.TMin = 0.0f;
                ray.TMax = distance;
                hit = ShootShadowRay(ray_flags, ray, shadowPayload);
            }
            else
                continue;
        }
    
        if (!hit)
        {
            payload.shadowFactor += ls;
        }
    }
    
    payload.color = payload.color * payload.shadowFactor;
}

[shader("closesthit")] // HitGroup 1 : chs
void ShadowRayClosestHit(inout ShadowHit payload, in BuiltInTriangleIntersectionAttributes attribs)
{
    payload.hit = true;
}
#define kMaxLights 32
enum LIGHT_TYPE
{
    LT_DIRECTIONAL = 1,
    LT_SPOT = 2,
    LT_POINT = 3,
};
enum RENDER_FLAG
{
    RF_CULL_BACK_FACES = 0x00000001,
    RF_FLIP_CASTER_FACES = 0x00000002,
    RF_IGNORE_SELF_SHADOW = 0x00000004,
    RF_KEEP_SELF_DROP_SHADOW = 0x00000008,
};
enum INSTANCE_FLAG
{
    IF_RECEIVE_SHADOWS = 0x01,
    IF_SHADOWS_ONLY = 0x02,
    IF_CAST_SHADOWS = 0x04,
};

struct CameraData
{
    matrix view;
    matrix proj;
    
    float3 position;
    float nearZ;
    float farZ;
    float3 padd;
};
struct LightData
{
    uint light_type;
    float3 position;
    float range;
    float3 direction;
    float spot_angle; // radian
    float3 color;
};

struct SceneData
{
    uint light_count;
    float3 padd;
    CameraData Camera;
    LightData Lights[kMaxLights];
};

RaytracingAccelerationStructure gRtScene : register(t0);
RWTexture2D<float4> gOutput : register(u0);
ConstantBuffer<SceneData> PerFrame : register(b0);

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

float CameraNearPlane()
{
    return PerFrame.Camera.nearZ;
}

float CameraFarPlane()
{
    return PerFrame.Camera.farZ;
}

int LightCount()
{
    return PerFrame.light_count;
}

LightData GetLight(int i)
{
    return PerFrame.Lights[i];
}

float angle_between(float3 a, float3 b)
{
    return acos(clamp(dot(a, b), 0, 1));
}

float3 HitPosition()
{
    return WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
}

float3 linearToSrgb(float3 c)
{
    // Based on http://chilliant.blogspot.com/2012/08/srgb-approximations-for-hlsl.html
    float3 sq1 = sqrt(c);
    float3 sq2 = sqrt(sq1);
    float3 sq3 = sqrt(sq2);
    float3 srgb = 0.662002687 * sq1 + 0.684122060 * sq2 - 0.323583601 * sq3 - 0.0225411470 * c;
    return srgb;
}

struct RayPayload
{
    float shadowFactor;
    float3 color;
};

struct ShadowPayload
{
    bool hit;
};

bool ShootShadowRay(uint flags, in RayDesc ray, inout ShadowPayload payload)
{
    TraceRay(gRtScene, flags, 0xFF, 1, 0, 1, ray, payload);
    return payload.hit;
}

[shader("raygeneration")]
void rayGen()
{
    //모델링 추가한 후에 활성화 시키기. 현재 카메라 데이터를 넣으니 삼각형으론 충돌판정을 못하고 있음.
    //uint2 screen_idx = DispatchRaysIndex().xy;
    //uint2 screen_dim = DispatchRaysDimensions().xy;

    //float aspect_ratio = (float) screen_dim.x / (float) screen_dim.y;
    //float2 screen_pos = ((float2(screen_idx) + 0.5f) / float2(screen_dim)) * 2.0f - 1.0f;
    //screen_pos.x *= aspect_ratio;
    
    //RayDesc ray;
    //ray.Origin = PerFrame.Camera.position;
    //ray.Direction = normalize(
    //    CameraRight() * screen_pos.x +
    //    CameraUp() * screen_pos.y +
    //    CameraForward() * CameraFocalLength());
    //ray.TMin = CameraNearPlane();
    //ray.TMax = CameraFarPlane();
    
    //RayPayload payload;
    //payload.shadowFactor = 0.f;
    //payload.color = float4(1.f, 1.f, 1.f, 1.f);
    
    //uint ray_flags = 0;
    //TraceRay(gRtScene, RAY_FLAG_NONE, 0xFF, 0, 0, 0, ray, payload);
    
    //gOutput[screen_idx] = float4(payload.color, 1);
    
    uint3 launchIndex = DispatchRaysIndex();
    uint3 launchDim = DispatchRaysDimensions();

    float2 crd = float2(launchIndex.xy);
    float2 dims = float2(launchDim.xy);

    float2 d = ((crd / dims) * 2.f - 1.f);
    float aspectRatio = dims.x / dims.y;

    RayDesc ray;
    ray.Origin = PerFrame.Camera.position;
    ray.Direction = normalize(float3(d.x * aspectRatio, -d.y, 1));

    ray.TMin = 0;
    ray.TMax = 100000;

    RayPayload payload;
    payload.shadowFactor = 0.f;
    payload.color = float4(1.f, 1.f, 1.f, 1.f);
    
    TraceRay(gRtScene, RAY_FLAG_NONE, 0xFF, 0, 2, 0, ray, payload);
    
    float3 col = linearToSrgb(payload.color);
    gOutput[launchIndex.xy] = float4(payload.color, 1);
}

[shader("miss")]
void miss(inout RayPayload payload)
{
    payload.color = float3(0.2, 0.0, 0.9);
}

[shader("closesthit")]
void chs(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attribs)
{
    //payload.color = payload.color * 1.0f;
    
    //float hitT = RayTCurrent();
    //float3 rayDirW = WorldRayDirection();
    //float3 rayOriginW = WorldRayOrigin();

    //float3 posW = rayOriginW + hitT * rayDirW;

    //RayDesc ray;
    //ray.Origin = posW;
    //ray.Direction = normalize(float3(0.5, 0.5, -0.5));
    //ray.TMin = 0.01;
    //ray.TMax = 100000;
    //ShadowPayload shadowPayload;
    
    //TraceRay(gRtScene, RAY_FLAG_NONE, 0xFF, 1, 0, 1, ray, shadowPayload);

    //float factor = shadowPayload.hit ? 0.1 : 1.0;
    //payload.color = payload.color * factor;
    
    ShadowPayload shadowPayload;
    shadowPayload.hit = false;
    
    float ls = 1.0f / LightCount();
    
    uint ray_flags_common = 0;
    
    int li;
    for (li = 0; li < LightCount(); ++li)
    {
        LightData light = GetLight(li);
      
        uint ray_flags = ray_flags_common;
    
        bool hit = true;
        if (light.light_type == LT_DIRECTIONAL)
        {
            // directional light
            RayDesc ray;
            ray.Origin = HitPosition();
            ray.Direction = -light.direction.xyz;
            ray.TMin = 0.0f;
            ray.TMax = CameraFarPlane();
            hit = ShootShadowRay(ray_flags, ray, shadowPayload);
        }
        else if (light.light_type == LT_SPOT)
        {
            // spot light
            float3 pos = HitPosition();
            float3 dir = normalize(light.position - pos);
            float distance = length(light.position - pos);
            if (distance <= light.range && angle_between(-dir, light.direction) * 2.0f <= light.spot_angle)
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
        else if (light.light_type == LT_POINT)
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
    
        if (!hit)
        {
            payload.shadowFactor += ls;
        }
    }
    
    payload.color = payload.color * payload.shadowFactor;
}

[shader("closesthit")]
void chs2(inout ShadowPayload payload, in BuiltInTriangleIntersectionAttributes attribs)
{
    payload.hit = true;
}

[shader("miss")]
void miss2(inout ShadowPayload payload)
{
    payload.hit = false;
}
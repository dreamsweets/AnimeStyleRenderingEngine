RaytracingAccelerationStructure RTScene : register(t0);
RWTexture2D<float4> OuputTexture : register(u0);

struct CameraData
{
    matrix view;
    matrix proj;
    
    float3 position;
    float nearZ;
    float farZ;
    float3 padd;
};
ConstantBuffer<CameraData> Camera : register(b0);

float3 CameraPosition()
{
    return Camera.position.xyz;
}

float3 CameraRight()
{
    return -Camera.view[0].xyz;
}

float3 CameraUp()
{
    return -Camera.view[1].xyz;
}

float3 CameraForward()
{
    return Camera.view[2].xyz;
}

float CameraFocalLength()
{
    return abs(Camera.proj[1][1]);
}

float CameraNearPlane()
{
    return Camera.nearZ;
}

float CameraFarPlane()
{
    return Camera.farZ;
}

struct RayPayload
{
    float3 color;
};

[shader("raygeneration")]
void RayGeneration()
{
    uint2 screen_idx = DispatchRaysIndex().xy;
    uint2 screen_dim = DispatchRaysDimensions().xy;
    
    float aspect_ratio = (float) screen_dim.x / (float) screen_dim.y;
    float2 screen_pos = ((float2(screen_idx) + 0.5f) / float2(screen_dim)) * 2.0f - 1.0f;
    screen_pos.x *= aspect_ratio;
    
    RayDesc ray;
    ray.Origin = Camera.position;
    ray.Direction = normalize(
        CameraRight() * screen_pos.x +
        CameraUp() * screen_pos.y +
        CameraForward() * CameraFocalLength());
    ray.TMin = CameraNearPlane();
    ray.TMax = CameraFarPlane();
    
    RayPayload payload;
    
    TraceRay(RTScene, 0 /*rayFlags*/, 0xFF, 0 /* ray index*/, 0, 0, ray, payload);
    float3 col = payload.color;
    OuputTexture[screen_idx] = float4(col, 1);
}

[shader("miss")]
void MissShader(inout RayPayload payload)
{
    payload.color = float3(0.0, 0.0, 0.0);

}

[shader("closesthit")]
void ClosestHitShader(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attribs)
{
    payload.color = float3(1.f, 1.f, 1.f);
}
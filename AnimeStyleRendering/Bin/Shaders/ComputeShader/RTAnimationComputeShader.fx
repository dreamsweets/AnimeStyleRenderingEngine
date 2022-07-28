struct SkinnedVertex
{
    float3 pos;
    float4 color;
    float2 uv;
    float3 normal;
    float3 bitangent;
    float3 tangent;
    float4 Weight;
    float4 BoneIndices;
};

StructuredBuffer<SkinnedVertex> g_VertexBuffer;
StructuredBuffer<matrix> g_SkinningBoneMatrixArray;
RWStructuredBuffer<float3> g_AnimationedVertexBuffer;

matrix GetBoneMatrix(int Index)
{
    return g_SkinningBoneMatrixArray[Index];
}

[numthreads(1024, 1, 1)]
void CSMain(int3 ThreadID : SV_DispatchThreadID)
{
    SkinnedVertex vertex = g_VertexBuffer[ThreadID.x];
    float3 pos = (float3)0;
    for (int i = 0; i < 4; ++i)
    {
        if (vertex.Weight[i] <= 0.f)
            continue;
        
        matrix matBone = GetBoneMatrix((int) vertex.BoneIndices[i]);
        pos += (mul(float4(vertex.pos, 1.f), matBone) * vertex.Weight[i]).xyz;
    }
    g_AnimationedVertexBuffer[ThreadID.x] = pos;
}
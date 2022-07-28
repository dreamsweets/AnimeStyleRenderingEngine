#include "RayTracingMeshComponent.h"
#include "DX12/VertexBuffer.h"
#include "DX12/IndexBuffer.h"
#include "Components/ComponentManager.h"
#include "Render/RenderManager.h"
#include "Resources/Animation/Animation.h"

RegisterComponentData(RayTracingMeshComponent)

namespace NvidiaHelpers = nv_helpers_dx12;

void RayTracingMeshComponent::OnIMGUIRender()
{
}

void RayTracingMeshComponent::AddMesh(Mesh* mesh, bool isAnimMesh)
{
    m_isAnimationMesh = isAnimMesh;
	auto iter = std::find(m_vecMesh.begin(), m_vecMesh.end(), mesh);
	if (iter != m_vecMesh.end()) return;
	
	m_vecMesh.push_back(mesh);
    
    if (!isAnimMesh)
    {
        m_BLASGenerator.AddVertexBuffer(
            mesh->GetVertexBuffer()->GetResource().Get(), 0, mesh->GetVertexCount(), sizeof(Vertex_SkeletalMesh),
            mesh->GetIndexBuffer()->GetResource().Get(), 0, mesh->GetIndexCount(), nullptr, 0);
    }
    else
    {
        m_VertexComputeShader.LoadByPath("Shaders/ComputeShader/RTAnimationComputeShader.fx");
        auto vertexBuffer = mesh->GetVertexBuffer()->GetResource();
        u32 vertexCount = mesh->GetVertexBuffer()->GetNumVertices();
        m_VertexComputeShader.SetRWBuffer("g_AnimationedVertexBuffer", sizeof(math::vec3), vertexCount);
        m_VertexBuffer = m_VertexComputeShader.GetResultBuffer();
        m_VertexComputeShader.SetAsStructuredBuffer("g_VertexBuffer", vertexBuffer, sizeof(Vertex_SkeletalMesh), vertexCount);
    }
    m_BLASDirty = true;
}

void RayTracingMeshComponent::AddAnimation(Animation* animation)
{
    m_Animation = animation;
}

Mesh* RayTracingMeshComponent::GetMesh(u32 index)
{
	if (index >= m_vecMesh.size()) return nullptr;

	return m_vecMesh[index];
}

void RayTracingMeshComponent::RemoveMesh(Mesh* mesh)
{
	auto iter = std::find(m_vecMesh.begin(), m_vecMesh.end(), mesh);
	if (iter == m_vecMesh.end()) return;

	m_vecMesh.erase(iter);
    m_BLASDirty = true;
}

void RayTracingMeshComponent::RemoveMesh(u32 index)
{
	if (index >= m_vecMesh.size()) return;

	m_vecMesh.erase(m_vecMesh.begin() + index);
    m_BLASDirty = true;
}

void RayTracingMeshComponent::Init()
{
    PrimitiveComponent::Init();
}

void RayTracingMeshComponent::Update()
{
    PrimitiveComponent::Update();

	if (m_BLASDirty)
	{
        BuildBottomLevelAS(false);
        m_BLASDirty = false;
	}
    m_InstanseWorld = m_Entity->GetTransform()->GetFixedWorldMatrix();
    
    if (m_isAnimationMesh)
    {
        UpdateVertexBuffer();
        BuildBottomLevelAS(true);
    }
}

void RayTracingMeshComponent::Render()
{
}

void RayTracingMeshComponent::BuildBottomLevelAS(bool isUpdate)
{
    /*if (isUpdate)
    {
        Gfx::ExecuteRTCommandList();
        Gfx::ResetRTCommandList();
        m_BLASGenerator.Generate(Gfx::GetRTCommandList(), m_BottomLevelAS.pScratch.Get(), m_BottomLevelAS.pResult.Get(), true, m_BottomLevelAS.pResult.Get());
        return;
    }
    */
    auto mesh = m_vecMesh.front();
    m_BLASGenerator.GetVertexBuffers().clear();

    if (!m_isAnimationMesh)
    {
        m_BLASGenerator.AddVertexBuffer(
            mesh->GetVertexBuffer()->GetResource().Get(), 0, mesh->GetVertexCount(), sizeof(Vertex_SkeletalMesh),
            mesh->GetIndexBuffer()->GetResource().Get(), 0, mesh->GetIndexCount(), nullptr, 0);
    }
    else
    {
        m_BLASGenerator.AddVertexBuffer(
            m_VertexBuffer.Get(), 0, mesh->GetVertexCount(), sizeof(math::vec3),
            mesh->GetIndexBuffer()->GetResource().Get(), 0, mesh->GetIndexCount(), nullptr, 0);
    }

    Gfx::ExecuteRTCommandList();

    UINT64 ScratchSizeInBytes = 0;
    UINT64 ResultSizeInBytes = 0;
    
    m_BLASGenerator.ComputeASBufferSizes(Gfx::GetDevice(), m_isAnimationMesh, &ScratchSizeInBytes, &ResultSizeInBytes);
    m_BottomLevelAS.pResult = Gfx::CreateBuffer(
        ResultSizeInBytes,
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
        kDefaultHeapProps
    );
    m_BottomLevelAS.pScratch = Gfx::CreateBuffer(
        ScratchSizeInBytes,
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON,
        kDefaultHeapProps
    );
    Gfx::ResetRTCommandList();

    m_BLASGenerator.Generate(Gfx::GetRTCommandList(), m_BottomLevelAS.pScratch.Get(), m_BottomLevelAS.pResult.Get(), false, nullptr);
}

void RayTracingMeshComponent::UpdateVertexBuffer()
{
    if (!m_Animation) return;
    auto boneCount = m_Animation->GetSkeleton()->GetBoneCount();
    auto BoneBuffer = m_Animation->GetBoneMatrixResource();
    m_VertexComputeShader.SetAsStructuredBuffer("g_SkinningBoneMatrixArray", BoneBuffer, sizeof(math::mat4), boneCount);
    m_VertexComputeShader.Dispatch(m_Animation->GetSkeleton()->GetBoneCount(), 1, 1);
}

#pragma once
#include "PrimitiveComponent.h"
#include "Resources/Mesh/Model.h"
#include "DX12/RayTracingMaterial.h"
#include "DX12/ComputeShader.h"

class Animation;

class RayTracingMeshComponent : public PrimitiveComponent {
	friend class RenderManager;

public:
	RayTracingMeshComponent(Entity* entity) : PrimitiveComponent{ entity } {}

	virtual void OnIMGUIRender() override;

public:
	void AddMesh(Mesh* mesh, bool isAnimMesh = false); 
	void AddAnimation(Animation* animation);
	Mesh* GetMesh(u32 index);
	void RemoveMesh(Mesh* mesh);
	void RemoveMesh(u32 index);
	math::mat4x4& InstanseWorldMatrix() { return m_InstanseWorld; }
	virtual void Init();
	virtual void Update();
	virtual void Render();
	AccelerationStructureBuffers& GetBottomLevelAS() { return m_BottomLevelAS; }

private:
	void BuildBottomLevelAS(bool isUpdate);
	void UpdateVertexBuffer();

private:
	std::vector<Mesh*> m_vecMesh;
	Animation* m_Animation{nullptr};
	ComputeShader m_VertexComputeShader;
	ComPtr<ID3D12Resource> m_VertexBuffer;
	AccelerationStructureBuffers m_BottomLevelAS;
	NvidiaHelpers::BottomLevelASGenerator m_BLASGenerator;
	math::mat4x4 m_InstanseWorld = math::identity<math::mat4x4>();
	bool m_BLASDirty = false;
	bool m_isAnimationMesh = false;
};
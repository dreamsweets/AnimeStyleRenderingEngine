#pragma once
#include "DX12/ConstantBuffer.h"

class PrimitiveComponent;
class MeshRendererComponent;
class RayTracingMeshComponent;
class LightComponent;
class SkyComponent;

class Mesh;
class GraphicsShader;
class Material;
class CameraManager;
class RayTracingMaterial;

class RenderManager {
public:
	void Init();

	void Register(PrimitiveComponent* c);
	void Unregister(PrimitiveComponent* c);

	void Register(LightComponent* c);

	void Update();
	void PostUpdate();
	void Render();
	void PostRender();

private:
	void PrepareLightPass();
	void PrepareGeometryPass();
	void PrepareRaytracingPass();
	void PreparePostProcessingPass();

	void ClearViews();
	void CreateDSV();
	void CreateRTVs();

	void SkyPass();
	void GeometryPass();
	void LightingPass();
	void CombineLightingResult();
	void PostProcessingPass();
	void RaytracingPass();

	//ImGui
private:
	void InitImGui();

	//Normal Rendering
private:
	PerFrameData m_PerFrameData;
	LightPassData m_LightPassData;
	RaytracingPassData m_RaytracingPassData;

	mutable bool m_TopLevelASDirty = true;

	std::vector<LightComponent*> m_vecLightComponent;
	
	std::vector<MeshRendererComponent*> m_vecMeshRendererComponent;
	std::vector<u32> m_vecMeshIndex;
	std::deque<u32> m_deqFreeMeshIndex;

	std::vector<RayTracingMeshComponent*> m_vecRaytracingMeshComponent;
	std::vector<u32> m_vecRaytracingMeshIndex;
	std::deque<u32> m_deqFreeRaytracingMeshIndex;

	CameraManager* m_CameraManager;
	ComPtr<ID3D12Resource> m_NormalPassOutputTexture;

	//Depth Buffer
	ComPtr<ID3D12DescriptorHeap> m_dsvHeap;
	ComPtr<ID3D12Resource> m_DepthTexture;
	D3D12_CPU_DESCRIPTOR_HANDLE m_dsvHandle;

	ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
	struct {
		using RTVTexture = std::pair<ComPtr<ID3D12Resource>, D3D12_CPU_DESCRIPTOR_HANDLE>;
		RTVTexture WorldPos;
		RTVTexture Diffuse;
		RTVTexture Normal;
		RTVTexture Bitangent;
		RTVTexture Tangent;
		RTVTexture LightResult;
		RTVTexture PostProcess;
	} m_RtvTexture;

	SkyComponent* m_Sky;
private:
	GraphicsShader* m_LightingShader{ nullptr };
	GraphicsShader* m_LightCombineShader{ nullptr };
	GraphicsShader* m_PostProcessingShader{nullptr};
	Material* m_LightingMaterial{ nullptr };
	Material* m_LightCombineMaterial{ nullptr };
	Material* m_PostProcessingMaterial{nullptr};
	
	RayTracingMaterial* m_RaytracingMaterial{ nullptr };

	DECLARE_SINGLE(RenderManager)

};
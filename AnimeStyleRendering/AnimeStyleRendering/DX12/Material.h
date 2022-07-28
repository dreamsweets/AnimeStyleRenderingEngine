#pragma once
#include "GraphicsShader.h"

class Texture;

class RootSignature {
public:
	RootSignature();

	ID3D12RootSignature* GetRootSignature() { return m_RootSignature.Get(); }

protected:
	ComPtr<ID3D12RootSignature> m_RootSignature;
};

struct MaterialResourceData {
	std::string Name{"Not Initialized"};
	u32 HeapIndex = (u32)-1;
	D3D_SHADER_INPUT_TYPE Type;
	ComPtr<ID3D12Resource> Resource{nullptr};
	ConstantBuffer ConstantBuffer;
	ImGuiTexture ImGuiTexture;
};

class Material : public RootSignature, public IMGUIElement {
	using BindPoint = u32;
	using HeapOffset = u32;
public:
	Material();
	void SetShader(GraphicsShader* shader);
	//가지고 있는 RootSignature와 PipelineState를 CommandList에 세팅한다.
	virtual void SetMaterial();

	template<typename T>
	void SetCBufferValue(std::string key, T data)
	{
		for (auto it = m_mapMaterialResourceIndex.lower_bound(key); it != m_mapMaterialResourceIndex.upper_bound(key); it++)
		{
			auto& resourceData = m_vecMaterialResourceData[it->second];
			if (resourceData.Type != D3D_SHADER_INPUT_TYPE::D3D_SIT_CBUFFER) continue;
			resourceData.ConstantBuffer.SetData(data);
		}
	}
	template<typename T>
	void SetStructuredBuffer(std::string key, ComPtr<ID3D12Resource> resource, u32 DataCount)
	{
		SetStructuredBuffer(key, resource, sizeof(T), DataCount);
	}

	void SetTexture(std::string key, Texture* texture);
	void SetTexture(std::string key, ComPtr<ID3D12Resource> resource);
	void SetStructuredBuffer(std::string key, ComPtr<ID3D12Resource> resource, u32 StructStride, u32 DataCount);
	void EnableDepth(bool value) { m_EnableDepth = value; CreatePSO(); }

	void SetCullMode(D3D12_CULL_MODE CullMode) { m_CullMode = CullMode; CreatePSO(); }
	void SetName(std::string name) { m_Name = name; }
	std::string GetName() { return m_Name; }

public:
	virtual void OnIMGUIRender() override;

protected:
	void CreatePSO();
	virtual void SetDefaultPSOVariables();
	virtual void CreateRootSignature(GraphicsShader* shader);
	void CreateShaderResourceView(u32 HeapIndex, ComPtr<ID3D12Resource> Resource, D3D12_SHADER_RESOURCE_VIEW_DESC desc);
	void CreateConstantBufferView(u32 HeapIndex);
	void CreateSamplerView(u32 HeapIndex, D3D12_SAMPLER_DESC& desc);
	void CreateConstantBuffers();
protected:
	std::string m_Name{ "Unnamed Material" };
	GraphicsShader* m_Shader{nullptr};
	bool m_EnableDepth{ true };
	D3D12_CULL_MODE m_CullMode{D3D12_CULL_MODE_BACK};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC m_PSODesc{};
	ComPtr<ID3D12PipelineState> m_PipelineState;
	
	DescriptorHeapWrapper m_DescHeap;
	std::vector<MaterialResourceData> m_vecMaterialResourceData;
	std::multimap<std::string, u32> m_mapMaterialResourceIndex;
	u32 m_HeapSize{ 0u };
	u32 m_HeapOffsetPS{ 0u };
	bool m_HasParameterVS{ false }, m_HasParameterPS{ false };
};

class GeometryMaterial : public Material {

protected:
	virtual void SetDefaultPSOVariables() override;

public:
	
};
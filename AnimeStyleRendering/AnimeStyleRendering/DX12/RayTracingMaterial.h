#pragma once
#include "DX12Common.h"
#include "Material.h"
#include <ThirdParty/DXCAPI/dxcapi.use.h>
#include <nv_helpers_dx12/TopLevelASGenerator.h>
#include <nv_helpers_dx12/BottomLevelASGenerator.h>
#include <nv_helpers_dx12/ShaderBindingTableGenerator.h>
#include <nv_helpers_dx12/RootSignatureGenerator.h>
#include <nv_helpers_dx12/ShaderBindingTableGenerator.h>
#include <nv_helpers_dx12/RaytracingPipelineGenerator.h>

namespace NvidiaHelpers = nv_helpers_dx12;

class RayTracingMeshComponent;
struct IDxcBlob;
class Mesh;

enum class ShaderType {
	RayGeneration = 7,
	AnyHit = 9,
	ClosestHit = 10,
	Miss = 11,
};

struct RTResourceMetaData {
	D3D_SHADER_INPUT_TYPE Type;
	u32 BindPoint;
	u32 DataSize = 0; // ��������ΰ�츸 ��ȿ
};

struct RootSignatureDesc
{
	D3D12_ROOT_SIGNATURE_DESC desc = {};
	std::vector<D3D12_DESCRIPTOR_RANGE> range;
	std::vector<D3D12_ROOT_PARAMETER> rootParams;
};

struct AccelerationStructureBuffers
{
	ComPtr<ID3D12Resource> pScratch;
	ComPtr<ID3D12Resource> pResult;
	ComPtr<ID3D12Resource> pInstanceDesc;    // Used only for top-level AS
};

struct RTShaderMetaData {
	const WCHAR* Name{nullptr};
	ShaderType Type;
	
	std::string payloadName; //HitRoup �ĺ��ڷ� ���

	std::vector<RTResourceMetaData> vecResourceData; //Shader���� ����ϴ�(���� ���ε��Ǵ�) ���ҽ��� ���� ������
	std::unordered_map<std::string, u32> mapResourceIndexing; //���ҽ��� string���� �˻��ؼ� �����ϱ� ���� id map
};

struct RayGen{
	RTShaderMetaData* RayGenerationShader{ nullptr };
	RootSignatureDesc LocalRootSignatureDesc;

	ComPtr<ID3D12RootSignature> LocalRootSignature;
	D3D12_GPU_DESCRIPTOR_HANDLE HeapStartAddress;
	std::vector<u32> vecHeapOffsetIndex;
	
	std::unordered_map<u32, ConstantBuffer> mapConstantBuffers; //����ϴ� ������� ����
	u32 EntrySize = 0; // ShaderTableEntrySize

	ConstantBuffer* GetCbuffer(std::string key)
	{
		auto result = RayGenerationShader->mapResourceIndexing.find(key);
		if (result == RayGenerationShader->mapResourceIndexing.end()) return nullptr;
		return &mapConstantBuffers[result->second];
	}

	u32 GetHeapIndex(std::string key)
	{
		auto result = RayGenerationShader->mapResourceIndexing.find(key);
		if (result == RayGenerationShader->mapResourceIndexing.end()) return (u32)-1;
		return result->second;
	}

	ComPtr<ID3D12Resource>	OutputTexure;
};

struct HitGroup{
	const WCHAR* Name{ nullptr };
	
	RootSignatureDesc LocalRootSignatureDesc;
	ComPtr<ID3D12RootSignature> LocalRootSignature;
	D3D12_GPU_DESCRIPTOR_HANDLE HeapStartAddress;
	std::vector<u32> vecHeapOffsetIndex;

	std::unordered_map<u32, ConstantBuffer> mapConstantBuffers; //����ϴ� ������� ����

	RTShaderMetaData* AnyHitShader{nullptr};
	RTShaderMetaData* ChsShader{nullptr};
	RTShaderMetaData* IntersectShader{ nullptr };
	RTShaderMetaData* RepresentationShader{ nullptr };  //������ HitGroup������ ������ �����ϴ� nullptr�� �ƴ� ��ǥ ���̴��� �ϳ� ���� �� �ְ� ��.
	u32 EntrySize = 0; // ShaderTableEntrySize

	ConstantBuffer* GetCbuffer(std::string key)
	{
		auto result = RepresentationShader->mapResourceIndexing.find(key);
		if (result == RepresentationShader->mapResourceIndexing.end()) return nullptr;
		return &mapConstantBuffers[result->second];
	}
};

struct Miss {
	RTShaderMetaData* MissShader{ nullptr };

	RootSignatureDesc LocalRootSignatureDesc;
	ComPtr<ID3D12RootSignature> LocalRootSignature;
	D3D12_GPU_DESCRIPTOR_HANDLE HeapStartAddress;
	std::vector<u32> vecHeapOffsetIndex;

	std::unordered_map<u32, ConstantBuffer> mapConstantBuffers; //����ϴ� ������� ���� (�����ϴ� vecResourceData index - �������)
	
	u32 EntrySize = 0; // ShaderTableEntrySize

	ConstantBuffer* GetCbuffer(std::string key)
	{
		auto result = MissShader->mapResourceIndexing.find(key);
		if (result == MissShader->mapResourceIndexing.end()) return nullptr;
		return &mapConstantBuffers[result->second];
	}
};

class RayTracingMaterial {
public:
	void Load(std::string ShaderPath);
	void Render();

	void BuildTopLevelAS(std::vector<RayTracingMeshComponent*>& vecInstances, bool UpdateOnly = false);
	void RemovePrevFrameBuffers();
public:
	RayGen* GetRayGen(u32 index);
	u32 GetRaygenSize() { return (u32)m_vecRaygen.size(); }
	HitGroup* GetHitGroup(std::string hitgroupName);
	Miss* GetMiss(u32 index);
	ID3D12Resource* GetOutputTexure() { return m_OutputTexure.Get(); }
	DescriptorHeapWrapper& GetHeap() { return m_DescriptorHeap; }
private:
	void CreateAccelerationStructures();
	void CreateGlobalRootSignature();
	
	void CreatePipelineState(); // Shader�� ���� PipelineState ����
	
	void CreateShaderTable();	// �� Shader���� ����� ���ҽ����� ���ε� ��. ����� ���̴� ���÷������� ������ ���̴��� ���ؼ��� ó���ϴµ�, ���߿� �� ������Ʈ������ �ٸ� ������۸� �־�� �ϴ� ���� ���� ó���ϵ��� ����.

private:
	void ReflectionLibrary(IDxcBlob* ShaderCode);
	void Init(RayGen& raygen);
	void Init(HitGroup& hitgroup);
	void Init(Miss& miss);
	u32 CalculateHeapSize();
	void GenerateLocalRootSignature(RootSignatureDesc& desc, RTShaderMetaData& shader);
private:
	static constexpr const u32 g_ShaderIdentifierSize	{ D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES };
	static constexpr const u32 g_ShaderDescriptorOffset	{ 8u };

	NvidiaHelpers::TopLevelASGenerator m_TopLevelASGenerator;
	AccelerationStructureBuffers m_TopLevelAS;
	AccelerationStructureBuffers m_PrevTopLevelAS;
	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> m_vecTLASViewCPUHandle;
	std::vector<std::string> m_vecPayloadName;
	std::vector<std::wstring>		m_vecName;
	std::vector<RTShaderMetaData>	m_vecShader; //��ü ���̴� ��
	ComPtr<IDxcBlob>				m_ShaderBlob{nullptr};
	
	RootSignatureDesc	m_GlobalRootSignatureDesc{};
	ComPtr<ID3D12RootSignature> m_GlobalRootSignature;
	
	std::vector<RayGen>		m_vecRaygen;
	std::unordered_map<std::string, HitGroup> m_mapHitGroup;
	std::vector<Miss>		m_vecMiss;
	
	u32 m_MaxPayloadSize	{ sizeof(float) * 4 }; //Sizeof Vector4(Color)
	u32 m_MaxAttributeSize	{ sizeof(float) * 2 }; //Default Attribute Size (Vector2(UV))
	
	//Pipeline State
	ComPtr<ID3D12StateObject>			m_PipelineStateObject;
	ComPtr<ID3D12StateObjectProperties> m_StateObjectProperties;
	
	//Resource
	ComPtr<ID3D12Resource>	m_OutputTexure;
	
	//DescriptorHeap
	DescriptorHeapWrapper m_DescriptorHeap;
	u32 m_HeapCurrentIndex = 0;
	u32 m_HeapSize = 0;

	NvidiaHelpers::ShaderBindingTableGenerator m_TableGenerator;
	ComPtr<ID3D12Resource>	m_ShaderTableStorage;
};
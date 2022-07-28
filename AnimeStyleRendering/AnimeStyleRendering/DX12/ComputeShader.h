#pragma once
#include "Material.h"
#include "Gfx.h"

struct ComputeResourceMetaData {
	using BindPoint = u32;
	using HeapIndex = u32;

	BindPoint bindPoint;
	D3D_SHADER_INPUT_TYPE type;
	HeapIndex heapIndex{(u32)-1};
	u32 ByteStride{0};
	u32 Size{0};
	ComPtr<ID3D12Resource> resource;
	ConstantBuffer constantBuffer;
};


class ComputeShader : public RootSignature {
	using Name = std::string;
	using BindPoint = u32;
	using DataSize = u32;
	
private:
	Name m_Name;
	ComPtr<ID3DBlob> m_CS;
	std::unordered_map<BindPoint, ReflectionShaderResource> m_mapCSConstantBuffer;
	std::vector<ComputeResourceMetaData> m_vecResourceData;
	std::unordered_map<Name, u32> m_mapResourceIndex;
	std::vector<ComPtr<ID3D12Resource>> m_HeapResources;

	ComPtr<ID3D12Resource> m_ResultBuffer;
	ComputeResourceMetaData* m_ResultMetaData{nullptr};
	ComPtr<ID3D12Resource> m_ReadBackBuffer;
	DescriptorHeapWrapper m_DescHeap;
	D3D12_COMPUTE_PIPELINE_STATE_DESC m_PipelineStateDesc;
	ComPtr<ID3D12PipelineState> m_PipelineState;
	bool m_UseRTCommandList = false;

public:
	template<typename T>
	static ComPtr<ID3D12Resource> CreateRWStructuredBuffer(const std::vector<T>& datas);
	static ComPtr<ID3D12Resource> CreateRWStructuredBuffer(u32 stride, u32 count);
	template<typename T>
	static ComPtr<ID3D12Resource> CreateStructuredBuffer(const std::vector<T>& datas);

	template<typename T>
	void SetAsRWBuffer(std::string name, const std::vector<T>& datas);
	template<typename T>
	void SetAsStructuredBuffer(std::string name, const std::vector<T>& datas);
	template<typename T>
	void CBufferSetValue(std::string key, T data);
	//GPU데이터를 CPU로 받아옵니다.
	void GetResourceFromResult(void* result);
	//주어진 stride, count로 내부에서 RWBuffer를 만들고 세팅합니다.
	void SetRWBuffer(std::string name, u32 stride, u32 count);
	//주어진 버퍼를 Structured Buffer로 세팅합니다.
	void SetAsStructuredBuffer(std::string name, ComPtr<ID3D12Resource>& resource, u32 stride, u32 count);
	void SetAsRWBuffer(std::string name, ComPtr<ID3D12Resource>& resource, u32 stride, u32 count);

	ComPtr<ID3D12Resource> GetResultBuffer() { return m_ResultBuffer; }
	u32 GetDataStride() { return m_ResultMetaData ? m_ResultMetaData->ByteStride : 0; }
	u32 GetDataCount() { return m_ResultMetaData ? m_ResultMetaData->Size / m_ResultMetaData->ByteStride : 0; }

public:
	void LoadByPath(std::string fullPath);
	~ComputeShader() = default;
	void Dispatch(int threadX, int threadY = 1, int threadZ = 1 );
	void SetUseRTCommandList() {
		m_UseRTCommandList = true;
	}

private:
	void CreateReadBackBuffer(ComputeResourceMetaData& metaData);
	void SetData(std::string name, void* pData, u32 size);
	void ReflectionCS(ID3DBlob* shader);
	void CreateDescriptorHeap();
	void CreateRootSignature();
	void CreatePipelineState();

	void SetConstantBuffers();
};

template<typename T>
inline ComPtr<ID3D12Resource> ComputeShader::CreateRWStructuredBuffer(const std::vector<T>& datas)
{
	ComPtr<ID3D12Resource> Buffer = Gfx::CreateBuffer(sizeof(T) * datas.size(), D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	return Buffer;
}

template<typename T>
inline ComPtr<ID3D12Resource> ComputeShader::CreateStructuredBuffer(const std::vector<T>& datas)
{
	ComPtr<ID3D12Resource> Buffer = Gfx::CreateBuffer(sizeof(T) * datas.size(), D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COPY_DEST);
	return Buffer;
}

template<typename T>
inline void ComputeShader::SetAsRWBuffer(std::string name, const std::vector<T>& datas)
{
	auto res = m_mapResourceIndex.find(name);
	if (res == m_mapResourceIndex.end()) return;

	auto& metaData = m_vecResourceData[res->second];
	if (metaData.type != D3D_SHADER_INPUT_TYPE::D3D_SIT_UAV_RWSTRUCTURED) return;

	auto& buffer = m_HeapResources[metaData.heapIndex];

	auto CreateNewBuffer = [&]() {		
		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};

		uavDesc.Format = DXGI_FORMAT_UNKNOWN;
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
		uavDesc.Buffer.FirstElement = 0;
		uavDesc.Buffer.NumElements = datas.size();
		uavDesc.Buffer.StructureByteStride = sizeof(T);
		uavDesc.Buffer.CounterOffsetInBytes = 0;
		uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

		metaData.ByteStride = uavDesc.Buffer.StructureByteStride;
		metaData.Size = uavDesc.Buffer.NumElements * metaData.ByteStride;

		D3D12_CPU_DESCRIPTOR_HANDLE uavHandle = m_DescHeap.hCPU(metaData.heapIndex);

		buffer = CreateRWStructuredBuffer(datas);
		buffer->SetName(WideString{ name + " RWBuffer" });
		Gfx::GetDevice()->CreateUnorderedAccessView(buffer.Get(), nullptr, &uavDesc, uavHandle);

		m_ResultBuffer = buffer;
		CreateReadBackBuffer(metaData);
		m_ResultMetaData = &(metaData);
	};

	if (!buffer)
	{
		CreateNewBuffer();
		return;
	}
	
	if (metaData.ByteStride != sizeof(T) || metaData.Size != sizeof(T) * datas.size())
	{
		//데이터 구조가 변화함 -> 다시 생성해야함.
		Gfx::ExecuteComputeCommandList();
		buffer.Reset();
		Gfx::ResetComputeCommandList();
		CreateNewBuffer();
		return;
	}
}

template<typename T>
inline void ComputeShader::SetAsStructuredBuffer(std::string name, const std::vector<T>& datas)
{
	if (datas.empty()) return;

	auto res = m_mapResourceIndex.find(name);
	if (res == m_mapResourceIndex.end()) return;
	
	auto& metaData = m_vecResourceData[res->second];

	if (metaData.type != D3D_SHADER_INPUT_TYPE::D3D11_SIT_STRUCTURED) return;

	auto& buffer = m_HeapResources[metaData.heapIndex];

	auto CreateNewBuffer = [&]() {
		ComPtr<ID3D12Resource> structuredBuffer = CreateStructuredBuffer(datas);
		structuredBuffer->SetName(WideString{ name + "Structured Buffer" });

		ComPtr<ID3D12Resource> uploadBuffer = Gfx::CreateBuffer(sizeof(T) * datas.size(), D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, kUploadHeapProps);
		uploadBuffer->SetName(WideString{ name + "UpladBuffer" });
		
		T* pData = nullptr;
		uploadBuffer->Map(0, nullptr, (void**)&pData);
		memcpy((void*)pData, (void*)datas.data(), sizeof(T) * datas.size());
		uploadBuffer->Unmap(0, nullptr);
		
		//D3D12_SUBRESOURCE_DATA subResourceData{};
		//subResourceData.pData = datas.data();
		//subResourceData.RowPitch = sizeof(T) * datas.size();
		//subResourceData.SlicePitch = subResourceData.RowPitch;
		//UpdateSubresources(Gfx::GetComputeCommandList(), structuredBuffer.Get(), uploadBuffer.Get(), 0, 0, 1, &subResourceData);
		Gfx::GetComputeCommandList()->CopyBufferRegion(structuredBuffer.Get(), 0, uploadBuffer.Get(), 0, sizeof(T) * datas.size());

		//Gfx::ExecuteComputeCommandList();

		//uploadBuffer.Reset();

		//Gfx::ResetComputeCommandList();

		Gfx::ResourceBarrier(Gfx::GetComputeCommandList(), structuredBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		srvDesc.Buffer.FirstElement = 0;
		srvDesc.Buffer.NumElements = datas.size();
		srvDesc.Buffer.StructureByteStride = sizeof(T);
		srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

		D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = m_DescHeap.hCPU(metaData.heapIndex);
		buffer = structuredBuffer;
		metaData.resource = uploadBuffer; // 업뎃이 된건지 모르겠어서 일단 Upload 버퍼를 지우지말고 남겨놓자.
		Gfx::GetDevice()->CreateShaderResourceView(buffer.Get(), &srvDesc, srvHandle);

		metaData.ByteStride = sizeof(T);
		metaData.Size = sizeof(T) * datas.size();
	};

	if (!buffer)
	{
		CreateNewBuffer();
		return;
	}

	//업로드 버퍼를 이용하여 구조화버퍼 업데이트
	auto& uploadBuffer = metaData.resource;
	T* pData = nullptr;
	uploadBuffer->Map(0, nullptr, (void**)&pData);
	memcpy((void*)pData, (void*)datas.data(), sizeof(T) * datas.size());
	uploadBuffer->Unmap(0, nullptr);
	Gfx::ResourceBarrier(Gfx::GetComputeCommandList(), buffer.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST);
	Gfx::GetComputeCommandList()->CopyBufferRegion(buffer.Get(), 0, uploadBuffer.Get(), 0, sizeof(T) * datas.size());
}

template<typename T>
inline void ComputeShader::CBufferSetValue(std::string key, T data)
{
	if (m_mapResourceIndex.find(key) == m_mapResourceIndex.end()) return;
	m_vecResourceData[m_mapResourceIndex[key]].constantBuffer.SetData(data);
}

#include "ComputeShader.h"
#include "Gfx.h"

void ComputeShader::SetData(std::string name, void* pData, u32 size)
{
	auto res = m_mapResourceIndex.find(name);
	if (res == m_mapResourceIndex.end()) return;
	
	auto& buffer = m_HeapResources[m_vecResourceData[res->second].heapIndex];

	D3D12_RANGE range{};
	range.Begin = 0;
	range.End = size;
	void* dataHandle = nullptr;
	buffer->Map(1, &range, &dataHandle);
	if (!dataHandle) return;

	memcpy(dataHandle, pData, size);
	buffer->Unmap(1, &range);
}

ComPtr<ID3D12Resource> ComputeShader::CreateRWStructuredBuffer(u32 stride, u32 count)
{
	ComPtr<ID3D12Resource> Buffer = Gfx::CreateBuffer(stride * count, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	return Buffer;
}

void ComputeShader::GetResourceFromResult(void* result)
{
	int* pData = nullptr;
	auto& MetaData = *m_ResultMetaData;

	const D3D12_RANGE range = { 0, MetaData.Size };
	D3DCall(m_ReadBackBuffer->Map(0, &range, (void**)&pData));
	memcpy(result, pData, MetaData.Size);
	m_ReadBackBuffer->Unmap(0, nullptr);
}

void ComputeShader::SetRWBuffer(std::string name, u32 stride, u32 count)
{
	auto res = m_mapResourceIndex.find(name);
	if (res == m_mapResourceIndex.end()) return;

	auto& metaData = m_vecResourceData[res->second];
	if (metaData.type != D3D_SHADER_INPUT_TYPE::D3D_SIT_UAV_RWSTRUCTURED) return;

	auto& buffer = m_HeapResources[metaData.heapIndex];

	buffer = CreateRWStructuredBuffer(stride, count);

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};

	uavDesc.Format = DXGI_FORMAT_UNKNOWN;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	uavDesc.Buffer.FirstElement = 0;
	uavDesc.Buffer.NumElements = count;
	uavDesc.Buffer.StructureByteStride = stride;
	uavDesc.Buffer.CounterOffsetInBytes = 0;
	uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

	metaData.ByteStride = uavDesc.Buffer.StructureByteStride;
	metaData.Size = uavDesc.Buffer.NumElements * metaData.ByteStride;

	D3D12_CPU_DESCRIPTOR_HANDLE uavHandle = m_DescHeap.hCPU(metaData.heapIndex);

	buffer->SetName(WideString{ name + " RWBuffer" });
	Gfx::GetDevice()->CreateUnorderedAccessView(buffer.Get(), nullptr, &uavDesc, uavHandle);

	m_ResultBuffer = buffer;
	CreateReadBackBuffer(metaData);
	m_ResultMetaData = &(metaData);
}

void ComputeShader::SetAsStructuredBuffer(std::string name, ComPtr<ID3D12Resource>& resource, u32 stride, u32 count)
{
	auto res = m_mapResourceIndex.find(name);
	if (res == m_mapResourceIndex.end()) return;

	auto& metaData = m_vecResourceData[res->second];

	if (metaData.type != D3D_SHADER_INPUT_TYPE::D3D11_SIT_STRUCTURED) return;

	auto& buffer = m_HeapResources[metaData.heapIndex];

	buffer = resource;
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Buffer.FirstElement = 0;
	srvDesc.Buffer.NumElements = count;
	srvDesc.Buffer.StructureByteStride = stride;
	srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

	D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = m_DescHeap.hCPU(metaData.heapIndex);
	Gfx::GetDevice()->CreateShaderResourceView(buffer.Get(), &srvDesc, srvHandle);

	metaData.ByteStride = stride;
	metaData.Size = stride * count;
}

void ComputeShader::SetAsRWBuffer(std::string name, ComPtr<ID3D12Resource>& resource, u32 stride, u32 count)
{
	auto res = m_mapResourceIndex.find(name);
	if (res == m_mapResourceIndex.end()) return;

	auto& metaData = m_vecResourceData[res->second];
	if (metaData.type != D3D_SHADER_INPUT_TYPE::D3D_SIT_UAV_RWSTRUCTURED) return;

	auto& buffer = m_HeapResources[metaData.heapIndex];

	buffer = resource;

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};

	uavDesc.Format = DXGI_FORMAT_UNKNOWN;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	uavDesc.Buffer.FirstElement = 0;
	uavDesc.Buffer.NumElements = count;
	uavDesc.Buffer.StructureByteStride = stride;
	uavDesc.Buffer.CounterOffsetInBytes = 0;
	uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

	metaData.ByteStride = uavDesc.Buffer.StructureByteStride;
	metaData.Size = uavDesc.Buffer.NumElements * metaData.ByteStride;

	D3D12_CPU_DESCRIPTOR_HANDLE uavHandle = m_DescHeap.hCPU(metaData.heapIndex);

	buffer->SetName(WideString{ name + " RWBuffer" });
	Gfx::GetDevice()->CreateUnorderedAccessView(buffer.Get(), nullptr, &uavDesc, uavHandle);

	m_ResultBuffer = buffer;
	CreateReadBackBuffer(metaData);
	m_ResultMetaData = &(metaData);
}

void ComputeShader::LoadByPath(std::string fullPath)
{
	assert(Path::IsExist(fullPath));

	m_Name = Path::GetFileName(fullPath);

	std::wstring widePath{ fullPath.begin(), fullPath.end() };

	//셰이더 컴파일
	{
	#if defined(_DEBUG)
		UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
	#else
		UINT compileFlags = 0;
	#endif

		ComPtr<ID3DBlob> error;
		if (FAILED(D3DCompileFromFile(widePath.c_str(), nullptr, nullptr, "CSMain", "cs_5_0", compileFlags, 0, &m_CS, &error))) {

			std::string errorString = (const char*)error->GetBufferPointer();
			MSGBox("CS 컴파일 에러 : " + errorString);
		}

		ReflectionCS(m_CS.Get());
		CreateDescriptorHeap();
		CreateRootSignature();
		CreatePipelineState();
		SetConstantBuffers();
	}
}

void ComputeShader::Dispatch(int threadX, int threadY, int threadZ)
{
	
	auto commandList = Gfx::GetComputeCommandList();
	
	commandList->SetPipelineState(m_PipelineState.Get());
	commandList->SetComputeRootSignature(m_RootSignature.Get());
	ID3D12DescriptorHeap* ppHeaps[] = { m_DescHeap };
	commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
	commandList->SetComputeRootDescriptorTable(0, m_DescHeap.hGPUHeapStart);
	
	commandList->Dispatch(threadX, threadY, threadZ);

	Gfx::ResourceBarrier(commandList, m_ResultBuffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
	commandList->CopyResource(m_ReadBackBuffer.Get(), m_ResultBuffer.Get());
	Gfx::ResourceBarrier(commandList, m_ResultBuffer.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	Gfx::ExecuteComputeCommandList();
	Gfx::ResetComputeCommandList();
}

void ComputeShader::CreateReadBackBuffer(ComputeResourceMetaData& metaData)
{
	D3D12_HEAP_PROPERTIES heapProperties = { D3D12_HEAP_TYPE_READBACK, D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
	D3D12_MEMORY_POOL_UNKNOWN, 1, 1 };

	m_ReadBackBuffer = Gfx::CreateBuffer(metaData.Size, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COPY_DEST, heapProperties);
	m_ReadBackBuffer->SetName(L"ReadBack Buffer");
}

void ComputeShader::ReflectionCS(ID3DBlob* shader)
{
	ComPtr<ID3D12ShaderReflection> reflection;
	D3DCall(D3DReflect(shader->GetBufferPointer(), shader->GetBufferSize(), IID_PPV_ARGS(&reflection)));
	D3D12_SHADER_DESC shader_desc{};
	reflection->GetDesc(&shader_desc);

	{
		u32 heapIndex = 0;
		for (int i = 0; i < (int)shader_desc.BoundResources; ++i)
		{
			D3D12_SHADER_INPUT_BIND_DESC inputBindDesc{};
			reflection->GetResourceBindingDesc(i, &inputBindDesc);

			auto index = (u32)m_vecResourceData.size();
			m_vecResourceData.push_back({});
			m_mapResourceIndex.insert({ inputBindDesc.Name, index });
			ComputeResourceMetaData& metaData = m_vecResourceData.back();

			metaData.bindPoint = inputBindDesc.BindPoint;
			metaData.type = inputBindDesc.Type;
			metaData.heapIndex = heapIndex++;

			if (inputBindDesc.Type == D3D_SHADER_INPUT_TYPE::D3D_SIT_CBUFFER)
			{
				//상수버퍼 처리
				ID3D12ShaderReflectionConstantBuffer* reflCbuf =
					reflection->GetConstantBufferByName(inputBindDesc.Name);
				D3D12_SHADER_BUFFER_DESC cbufDesc{};
				reflCbuf->GetDesc(&cbufDesc);

				metaData.constantBuffer.Init(Gfx::GetDevice(), cbufDesc.Size);

				auto& refCbuffer = m_mapCSConstantBuffer[inputBindDesc.BindPoint];
				refCbuffer.name = inputBindDesc.Name;
				refCbuffer.size = cbufDesc.Size;

				for (int j = 0; j < (int)cbufDesc.Variables; ++j)
				{
					auto variable = reflCbuf->GetVariableByIndex(j);
					D3D12_SHADER_VARIABLE_DESC varDesc{};
					variable->GetDesc(&varDesc);
					ReflectionVariable var;
					var.parent = &refCbuffer;
					var.name = varDesc.Name;
					var.offset = varDesc.StartOffset;
					var.size = varDesc.Size;

					D3D12_SHADER_TYPE_DESC typeDesc;
					variable->GetType()->GetDesc(&typeDesc);
					var.type = typeDesc.Name;

					auto index = refCbuffer.vecVariable.size();
					refCbuffer.vecVariable.push_back(var);
					refCbuffer.mapVariableNames[var.name] = (u32)index;
				}
			}
		}
	}
}

void ComputeShader::CreateDescriptorHeap()
{
	auto Size = m_mapResourceIndex.size();
	m_DescHeap.Create(Gfx::GetDevice(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, (UINT)Size, true);
	m_HeapResources.resize(Size);
}

void ComputeShader::CreateRootSignature()
{
	std::vector<CD3DX12_ROOT_PARAMETER> vecRootParameter;
	std::vector<D3D12_DESCRIPTOR_RANGE> vecRange;

	for (auto& Data : m_vecResourceData)
	{
		if (Data.type == D3D_SHADER_INPUT_TYPE::D3D_SIT_STRUCTURED)
		{
			CD3DX12_DESCRIPTOR_RANGE range{};
			range.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, Data.bindPoint);
			vecRange.push_back(range);
		}
		else if (Data.type == D3D_SHADER_INPUT_TYPE::D3D_SIT_UAV_RWSTRUCTURED)
		{
			CD3DX12_DESCRIPTOR_RANGE range{};
			range.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, Data.bindPoint);
			vecRange.push_back(range);
		}
		else if (Data.type == D3D_SHADER_INPUT_TYPE::D3D_SIT_CBUFFER)
		{
			CD3DX12_DESCRIPTOR_RANGE range{};
			range.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, Data.bindPoint);
			vecRange.push_back(range);
		}
	
	}

	if (!vecRange.empty())
	{
		CD3DX12_ROOT_PARAMETER param;
		param.InitAsDescriptorTable((UINT)vecRange.size(), vecRange.data(), D3D12_SHADER_VISIBILITY_ALL);
		vecRootParameter.push_back(param);
	}
	CD3DX12_ROOT_SIGNATURE_DESC RootSignatureDesc;
	RootSignatureDesc.Init((UINT)vecRootParameter.size(), vecRootParameter.data(), 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_NONE);
	
	ComPtr<ID3DBlob> signature;
	ComPtr<ID3DBlob> error;
	if (FAILED(D3D12SerializeRootSignature(&RootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error)))
	{
		std::string errorString = (const char*)error->GetBufferPointer();
		MSGBox("Signature 생성 실패 : \n" + errorString);
		throw;
	}

	D3DCall(Gfx::GetDevice()->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_RootSignature)));
}

void ComputeShader::CreatePipelineState()
{
	m_PipelineStateDesc = {0};
	m_PipelineStateDesc.pRootSignature = m_RootSignature.Get();
	m_PipelineStateDesc.CS = { m_CS->GetBufferPointer(), m_CS->GetBufferSize() };
	m_PipelineStateDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	
	D3DCall(Gfx::GetDevice()->CreateComputePipelineState(&m_PipelineStateDesc, IID_PPV_ARGS(&m_PipelineState)));
}

void ComputeShader::SetConstantBuffers()
{
	for (auto& metadata : m_vecResourceData)
	{
		if (metadata.type == D3D_SHADER_INPUT_TYPE::D3D_SIT_CBUFFER)
		{
			D3D12_CPU_DESCRIPTOR_HANDLE handle = m_DescHeap.hCPU(metadata.heapIndex);
			
			D3D12_CONSTANT_BUFFER_VIEW_DESC cbDesc;
			cbDesc.BufferLocation = metadata.constantBuffer.GetGPUVirtualAddress();
			cbDesc.SizeInBytes = metadata.constantBuffer.GetBufferSize();
			Gfx::GetDevice()->CreateConstantBufferView(&cbDesc, handle);
			m_HeapResources[metadata.heapIndex] = metadata.constantBuffer.GetResourceComPtr();
		}
	}
}

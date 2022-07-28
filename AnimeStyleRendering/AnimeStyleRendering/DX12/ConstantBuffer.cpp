#include "ConstantBuffer.h"

void ConstantBuffer::Init(ComPtr<ID3D12Device5> device, u32 BufferSize)
{
	auto heapProperty = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	m_BufferSize = ROUND_UP(BufferSize, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
	auto reourceDesc = CD3DX12_RESOURCE_DESC::Buffer(m_BufferSize);

	D3DCall(device->CreateCommittedResource(&heapProperty, D3D12_HEAP_FLAG_NONE, &reourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		IID_PPV_ARGS(&m_Resource)));
	m_Resource->SetName(L"ConstantBuffer Resource");
	CD3DX12_RANGE ReadRange(0, 0);
	D3DCall(m_Resource->Map(0, &ReadRange, reinterpret_cast<void**>(&m_GPUAddress)));

	m_Data.resize(BufferSize);
	m_Initialized = true;
}

void ConstantBuffer::BindBuffer(ComPtr<ID3D12GraphicsCommandList4> CommandList, UINT RootParameterIndex)
{
	CommandList->SetGraphicsRootConstantBufferView(RootParameterIndex, m_Resource->GetGPUVirtualAddress());
}

void ConstantBuffer::Destroy()
{
	if (m_Initialized == true)
	{
		m_Resource->Unmap(0, nullptr);
	}
}

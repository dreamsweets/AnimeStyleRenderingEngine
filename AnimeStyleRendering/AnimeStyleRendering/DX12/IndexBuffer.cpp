#include "IndexBuffer.h"

IndexBuffer::IndexBuffer(IndexBufferDataType& indices)
{
	const UINT IndexBufferSize = sizeof(u32) * indices.size();
	m_NumIndices = (u32)indices.size();

	auto Device = Gfx::GetDevice();
	{
		auto heapProperty = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(IndexBufferSize);
		D3DCall(Device->CreateCommittedResource(
			&heapProperty,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_IndexBuffer)));
	}

	{
		UINT8* pVertexDataBegin;
		CD3DX12_RANGE readRange(0, 0);		// We do not intend to read from this resource on the CPU.
		D3DCall(m_IndexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
		memcpy(pVertexDataBegin, indices.data(), IndexBufferSize);
		m_IndexBuffer->Unmap(0, nullptr);
	}

	{
		m_IndexBufferView.BufferLocation = m_IndexBuffer->GetGPUVirtualAddress();
		m_IndexBufferView.Format = DXGI_FORMAT_R32_UINT;
		m_IndexBufferView.SizeInBytes = IndexBufferSize;
	}
}

void IndexBuffer::SetIndexBuffer(ComPtr<ID3D12GraphicsCommandList4> commandList)
{
	commandList->IASetIndexBuffer(&m_IndexBufferView);
}
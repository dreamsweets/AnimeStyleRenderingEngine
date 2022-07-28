#pragma once
#include"Gfx.h"

class VertexBuffer {
public:
	template<typename T>
	VertexBuffer(std::vector<T>& vertices);
	void SetVertexBuffer(ComPtr<ID3D12GraphicsCommandList4> commandList);
	u32 GetNumVertices() { return m_NumVertices; }
	ComPtr<ID3D12Resource>& GetResource() { return m_VertexBuffer; }
	D3D12_VERTEX_BUFFER_VIEW& GetView() { return m_VertexBufferView; }

private:
	unsigned int m_NumVertices;
	ComPtr<ID3D12Resource> m_VertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView;
};

template<typename T>
inline VertexBuffer::VertexBuffer(std::vector<T>& vertices)
{
	const UINT VertexBufferSize = sizeof(T) * (UINT)vertices.size();
	m_NumVertices = (unsigned int)vertices.size();

	//VertexBuffer 생성
	{
		auto heapProperty = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(VertexBufferSize);
		D3DCall(Gfx::GetDevice()->CreateCommittedResource(
			&heapProperty,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_VertexBuffer)));
	}

	//VertexBuffer에 데이터 카피
	{
		UINT8* pVertexDataBegin;
		CD3DX12_RANGE readRange(0, 0);		// We do not intend to read from this resource on the CPU.
		D3DCall(m_VertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
		memcpy(pVertexDataBegin, vertices.data(), VertexBufferSize);
		m_VertexBuffer->Unmap(0, nullptr);
	}

	//VertexBufferView 초기화
	{
		m_VertexBufferView.BufferLocation = m_VertexBuffer->GetGPUVirtualAddress();
		m_VertexBufferView.StrideInBytes = sizeof(T);
		m_VertexBufferView.SizeInBytes = VertexBufferSize;
	}
}
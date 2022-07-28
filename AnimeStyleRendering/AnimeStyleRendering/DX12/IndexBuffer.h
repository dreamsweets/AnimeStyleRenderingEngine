#pragma once
#include "Gfx.h"

class IndexBuffer {
public:
	using IndexBufferDataType = std::vector<unsigned int>;
	IndexBuffer(IndexBufferDataType& indices);

	void SetIndexBuffer(ComPtr<ID3D12GraphicsCommandList4> commandList);
	u32 GetNumIndices() { return m_NumIndices; }
	ComPtr<ID3D12Resource>& GetResource() { return m_IndexBuffer; }
	D3D12_INDEX_BUFFER_VIEW& GetView() { return m_IndexBufferView; }
private:
	unsigned int m_NumIndices{ 0u };
	ComPtr<ID3D12Resource> m_IndexBuffer;
	D3D12_INDEX_BUFFER_VIEW m_IndexBufferView{};
};
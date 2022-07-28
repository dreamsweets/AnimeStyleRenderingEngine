#include "VertexBuffer.h"

void VertexBuffer::SetVertexBuffer(ComPtr<ID3D12GraphicsCommandList4> commandList)
{
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->IASetVertexBuffers(0, 1, &m_VertexBufferView);
	commandList->DrawInstanced(m_NumVertices, 1, 0, 0);
}
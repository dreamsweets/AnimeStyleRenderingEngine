#pragma once
#include "DX12Common.h"

class ConstantBuffer {
	u32 m_BufferSize{ 0 };
	std::vector<byte> m_Data;
	ComPtr<ID3D12Resource> m_Resource{ nullptr };
	UINT8* m_GPUAddress{ nullptr };
	bool m_Initialized = false;
public:
	ConstantBuffer() = default;
	void Destroy();
	~ConstantBuffer() = default;
	
	void Init(ComPtr<ID3D12Device5> device, u32 BufferSize);

	void BindBuffer(ComPtr<ID3D12GraphicsCommandList4> CommandList, UINT RootParameterIndex);

	ID3D12Resource* GetResource() const
	{
		return m_Resource.Get();
	}

	ComPtr<ID3D12Resource> GetResourceComPtr() { return m_Resource; }

	inline D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const
	{
		return m_Resource->GetGPUVirtualAddress();
	}

	template<typename T>
	void SetData(T data)
	{
		memcpy(m_Data.data(), &data, m_Data.size());
		T& debug = *((T*)m_GPUAddress);
		memcpy(m_GPUAddress, m_Data.data(), m_Data.size());
	}

	u32 GetBufferSize() { return m_BufferSize; }
};
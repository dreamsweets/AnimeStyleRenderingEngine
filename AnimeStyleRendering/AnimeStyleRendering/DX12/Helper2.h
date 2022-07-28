#pragma once
#include "DX12Common.h"
#include "ConstantBuffer.h"

class Helper2 {
public:
	static constexpr unsigned int g_NumFrames = 3u;
	static constexpr const char* g_windowName = "Helper2Window";
	static Helper2& Inst();

public:
	bool Init();
	void OnFrame();
	void Destroy();
	u32 BeginFrame();
	void EndFrame(uint32_t rtvIndex);

	void ExecuteCommandList();
	void WaitForGPU();
	void ResetCommandList();

public:
	ComPtr<ID3D12Resource> CreateBuffer(uint64_t size, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES initState, D3D12_HEAP_PROPERTIES HeapProperty = kDefaultHeapProps);
	void ResourceBarrier(ComPtr<ID3D12GraphicsCommandList4> pCmdList, ComPtr<ID3D12Resource> pResource, D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter);
	ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(uint32_t count, D3D12_DESCRIPTOR_HEAP_TYPE type, bool shaderVisible);

//Getter Setter
public:
	HWND GetHWND() { return m_hWnd; }
	ComPtr<ID3D12Device5> GetDevice() { return mpDevice; }
	ComPtr<ID3D12GraphicsCommandList4> GetCommandList() { return mpCmdList; }
	u32 GetWidth() { return m_Width; }
	u32 GetHeight() { return m_Height; }
	ComPtr<ID3D12Resource> GetBackBufferResource(uint32_t RtvIndex) { return mFrameObjects[RtvIndex].pSwapChainBuffer; }
	ComPtr<ID3D12CommandAllocator> GetCurrentCommandAllocator();

private:
	bool InitDX();

private:
	ComPtr<ID3D12Device5> mpDevice;
	ComPtr<ID3D12CommandQueue> mpCmdQueue;
	ComPtr<IDXGISwapChain3> mpSwapChain;
	ComPtr<ID3D12GraphicsCommandList4> mpCmdList;
	ComPtr<ID3D12Fence> mpFence;
	HANDLE mFenceEvent;
	uint64_t mFenceValue = 0;
	struct
	{
		ComPtr<ID3D12CommandAllocator> pCmdAllocator;
		ComPtr<ID3D12Resource> pSwapChainBuffer;
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle;
	} mFrameObjects[g_NumFrames];

	struct HeapData
	{
		ComPtr<ID3D12DescriptorHeap> pHeap;
		uint32_t usedEntries = 0;
	};
	HeapData mRtvHeap;
	static const uint32_t kRtvHeapSize = 3;

	CD3DX12_VIEWPORT m_viewport;
	CD3DX12_RECT m_scissorRect;

	HWND m_hWnd;
	u32 m_Width, m_Height;

	static LRESULT WINAPI HelperWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
};

#define Gfx2 Helper2::Inst()

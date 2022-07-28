#pragma once
#include "DX12Common.h"
#include "ConstantBuffer.h"

namespace Gfx {
static constexpr unsigned int g_NumFrames = 3u;
static constexpr const char* g_windowName = "Anime Rendering";

bool Init();
void Destroy();
void BeginFrame();
void EndFrame();

void ClearRenderTarget(ID3D12Resource* resource, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle, D3D12_RESOURCE_STATES stateBefore, math::vec4 clearColor);

ComPtr<ID3D12Resource> CreateBuffer(uint64_t size, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES initState, D3D12_HEAP_PROPERTIES HeapProperty = kDefaultHeapProps);
void ResourceBarrier(ID3D12GraphicsCommandList* pCmdList, ID3D12Resource* pResource, D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter);
ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(uint32_t count, D3D12_DESCRIPTOR_HEAP_TYPE type, bool shaderVisible);

HWND GetHWND();
ID3D12Device5* GetDevice();
ID3D12GraphicsCommandList4* GetCommandList();
ID3D12GraphicsCommandList4* GetComputeCommandList();
ID3D12GraphicsCommandList4* GetRTCommandList();
u32 GetWidth();
u32 GetHeight();
ID3D12Resource* GetBackBuffer();
void WaitForGPU();

void ExecuteComputeCommandList();
void ResetComputeCommandList();

void ExecuteRTCommandList();
void ResetRTCommandList();

D3D12_CPU_DESCRIPTOR_HANDLE& GetBackbufferRtvHandle();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
} // namespace Gfx

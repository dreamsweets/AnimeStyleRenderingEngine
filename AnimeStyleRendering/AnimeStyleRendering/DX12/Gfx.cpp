#include "Gfx.h"
#include "imgui/imgui_impl_win32.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
namespace Gfx {
namespace {
ComPtr<ID3D12Device5> m_Device;
ComPtr<ID3D12CommandQueue> m_GraphicsCmdQueue;
ComPtr<ID3D12CommandQueue> m_ComputeCmdQueue;
ComPtr<IDXGISwapChain3> m_SwapChain;
ComPtr<ID3D12GraphicsCommandList4> m_CmdList;
ComPtr<ID3D12GraphicsCommandList4> m_RTCmdList;
ComPtr<ID3D12GraphicsCommandList4> m_ComputeCmdList;

ComPtr<ID3D12Fence> m_Fence;
UINT64 m_FenceValues[g_NumFrames] {};
HANDLE m_FenceEvent{};
HANDLE m_ComputeFenceEvent{};
int m_FrameIndex{ 0 };

struct {
    ComPtr<ID3D12Resource> m_DepthStencilTexture;
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle;
} m_DepthStencilObjects;

struct
{
    ComPtr<ID3D12CommandAllocator> pCmdAllocator;
    ComPtr<ID3D12CommandAllocator> pComputeCmdAllocator;
    ComPtr<ID3D12CommandAllocator> pRTCmdAllocator;
    ComPtr<ID3D12Resource> pSwapChainBuffer;
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle;
} m_FrameObjects[g_NumFrames];

struct HeapData
{
    ComPtr<ID3D12DescriptorHeap> pHeap;
    uint32_t usedEntries = 0;
};
HeapData m_RtvHeap;
static const uint32_t g_RtvHeapSize = 3;

CD3DX12_VIEWPORT m_Viewport;
CD3DX12_RECT m_ScissorRect;

HWND m_hWnd;
u32 m_Width, m_Height;
} //anonymous namespace

inline namespace For_Initialize {
ComPtr<IDXGISwapChain3> createDxgiSwapChain(IDXGIFactory4* pFactory, HWND hwnd, uint32_t width, uint32_t height, DXGI_FORMAT format, ID3D12CommandQueue* pCommandQueue)
{
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.BufferCount = g_NumFrames;
    swapChainDesc.Width = width;
    swapChainDesc.Height = height;
    swapChainDesc.Format = format;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.SampleDesc.Count = 1;

    ComPtr<IDXGISwapChain1> pSwapChain;

    D3DCall(pFactory->CreateSwapChainForHwnd(pCommandQueue, hwnd, &swapChainDesc, nullptr, nullptr, &pSwapChain));

    ComPtr<IDXGISwapChain3> pSwapChain3;
    D3DCall(pSwapChain->QueryInterface(IID_PPV_ARGS(&pSwapChain3)));
    
    return pSwapChain3;
}

ComPtr<ID3D12Device5> createDevice(IDXGIFactory4* pDxgiFactory)
{
    // Find the HW adapter
    ComPtr<IDXGIAdapter1> pAdapter;

    for (uint32_t i = 0; DXGI_ERROR_NOT_FOUND != pDxgiFactory->EnumAdapters1(i, &pAdapter); i++)
    {
        DXGI_ADAPTER_DESC1 desc;
        pAdapter->GetDesc1(&desc);

        // Skip SW adapters
        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue;
    #ifdef _DEBUG
        ComPtr<ID3D12Debug> pDx12Debug;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&pDx12Debug))))
        {
            pDx12Debug->EnableDebugLayer();
        }
    #endif
        // Create the device
        ComPtr<ID3D12Device5> pDevice;
        D3DCall(D3D12CreateDevice(pAdapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&pDevice)));

        D3D12_FEATURE_DATA_D3D12_OPTIONS5 features5{};
        HRESULT hr = pDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &features5, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS5));
        if (SUCCEEDED(hr) && features5.RaytracingTier != D3D12_RAYTRACING_TIER_NOT_SUPPORTED)
        {
            return pDevice;
        }
    }
    MSGBox("Raytracing is not supported on this device. Make sure your GPU supports DXR (such as Nvidia's Volta or Turing RTX) and you're on the latest drivers. The DXR fallback layer is not supported.");
    
    exit(1);

    return nullptr;
}

ComPtr<ID3D12CommandQueue> createGraphicsCommandQueue(ID3D12Device* pDevice)
{
    ComPtr<ID3D12CommandQueue> pQueue;
    D3D12_COMMAND_QUEUE_DESC cqDesc = {};
    cqDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    cqDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    D3DCall(pDevice->CreateCommandQueue(&cqDesc, IID_PPV_ARGS(&pQueue)));
    return pQueue;
}

ComPtr<ID3D12CommandQueue> createComputeCommandQueue(ID3D12Device* pDevice)
{
    ComPtr<ID3D12CommandQueue> pQueue;
    D3D12_COMMAND_QUEUE_DESC cqDesc = {};
    cqDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    cqDesc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
    D3DCall(pDevice->CreateCommandQueue(&cqDesc, IID_PPV_ARGS(&pQueue)));
    return pQueue;
}

ComPtr<ID3D12DescriptorHeap> createDescriptorHeap(ID3D12Device* pDevice, uint32_t count, D3D12_DESCRIPTOR_HEAP_TYPE type, bool shaderVisible)
{
    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.NumDescriptors = count;
    desc.Type = type;
    desc.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    ComPtr<ID3D12DescriptorHeap> pHeap;
    D3DCall(pDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&pHeap)));
    return pHeap;
}

D3D12_CPU_DESCRIPTOR_HANDLE createRTV(ID3D12Device* pDevice, ID3D12Resource* pResource, ID3D12DescriptorHeap* pHeap, uint32_t& usedHeapEntries, DXGI_FORMAT format)
{
    D3D12_RENDER_TARGET_VIEW_DESC desc = {};
    desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    desc.Format = format;
    desc.Texture2D.MipSlice = 0;
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = pHeap->GetCPUDescriptorHandleForHeapStart();
    rtvHandle.ptr += usedHeapEntries * pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    usedHeapEntries++;
    pDevice->CreateRenderTargetView(pResource, &desc, rtvHandle);
    return rtvHandle;
}

void resourceBarrier(ID3D12GraphicsCommandList* pCmdList, ID3D12Resource* pResource, D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter)
{
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = pResource;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = stateBefore;
    barrier.Transition.StateAfter = stateAfter;
    pCmdList->ResourceBarrier(1, &barrier);
}

//uint64_t submitCommandList(ID3D12GraphicsCommandList* pCmdList, ID3D12CommandQueue* pCmdQueue, ID3D12Fence* pFence, uint64_t fenceValue)
//{
//    D3DCall(pCmdList->Close());
//    ID3D12CommandList* pGraphicsList = pCmdList;
//    pCmdQueue->ExecuteCommandLists(1, &pGraphicsList);
//    fenceValue++;
//    pCmdQueue->Signal(pFence, fenceValue);
//    return fenceValue;
//}

void createFenceEvent() {
    D3DCall(m_Device->CreateFence(m_FenceValues[m_FrameIndex], D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_Fence)));
    m_FenceValues[m_FrameIndex]++;

    m_FenceEvent = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);
    if (m_FenceEvent == nullptr) throw std::exception("Fence Event was nullptr");

    m_ComputeFenceEvent = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);
    if (m_ComputeFenceEvent == nullptr) throw std::exception("Fence Event was nullptr");
}



bool InitDX(){
    UINT DxgiFactoryFlags = 0u;
#ifdef _DEBUG
    ComPtr<ID3D12Debug> pDebug;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&pDebug))))
    {
        pDebug->EnableDebugLayer();
        DxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
    }

    Microsoft::WRL::ComPtr<IDXGIInfoQueue> dxgiInfoQueue;
    if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(dxgiInfoQueue.GetAddressOf()))))
    {
        dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
        dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);

        DXGI_INFO_QUEUE_MESSAGE_ID hide[] =
        {
            80 /* IDXGISwapChain::GetContainingOutput: The swapchain's adapter does not control the output on which the swapchain's window resides. */,
        };
        DXGI_INFO_QUEUE_FILTER filter = {};
        filter.DenyList.NumIDs = _countof(hide);
        filter.DenyList.pIDList = hide;
        dxgiInfoQueue->AddStorageFilterEntries(DXGI_DEBUG_DXGI, &filter);
    }

#endif
    ComPtr<IDXGIFactory4> pDxgiFactory;
    D3DCall(CreateDXGIFactory2(DxgiFactoryFlags, IID_PPV_ARGS(&pDxgiFactory)));

    m_Device = createDevice(pDxgiFactory.Get());
    m_Device->SetName(L"D3D12Device5");

    m_GraphicsCmdQueue = createGraphicsCommandQueue(m_Device.Get());
    m_GraphicsCmdQueue->SetName(L"GraphicsCommandQueue");

    m_ComputeCmdQueue = createComputeCommandQueue(m_Device.Get());
    m_ComputeCmdQueue->SetName(L"ComputeCommandQueue");

    m_SwapChain = createDxgiSwapChain(pDxgiFactory.Get(), m_hWnd, m_Width, m_Height, DXGI_FORMAT_R8G8B8A8_UNORM, m_GraphicsCmdQueue.Get());

    m_RtvHeap.pHeap = createDescriptorHeap(m_Device.Get(), g_RtvHeapSize, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, false);
    m_RtvHeap.pHeap->SetName(L"RenderTargetView Heap");

    for (uint32_t i = 0; i < arraysize(m_FrameObjects); i++)
    {
        D3DCall(m_Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_FrameObjects[i].pCmdAllocator)));

        //Create Ray tracing CommandAllocator
        D3DCall(m_Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_FrameObjects[i].pRTCmdAllocator)));

        //Create Compute CommandAllocator
        D3DCall(m_Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_FrameObjects[i].pComputeCmdAllocator)));

        D3DCall(m_SwapChain->GetBuffer(i, IID_PPV_ARGS(&m_FrameObjects[i].pSwapChainBuffer)));
        m_FrameObjects[i].rtvHandle = createRTV(m_Device.Get(), m_FrameObjects[i].pSwapChainBuffer.Get(), m_RtvHeap.pHeap.Get(), m_RtvHeap.usedEntries, DXGI_FORMAT_R8G8B8A8_UNORM);

        char s[256];
        sprintf(s, "SwapChainBuffer %d", i);
        m_FrameObjects[i].pSwapChainBuffer->SetName(WideString{ s });
    }
    D3DCall(m_Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_FrameObjects[0].pCmdAllocator.Get(), nullptr, IID_PPV_ARGS(&m_CmdList)));
    m_CmdList->SetName(L"Default CommandList");
    // Create Ray tracing CommandList
    D3DCall(m_Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_FrameObjects[0].pRTCmdAllocator.Get(), nullptr, IID_PPV_ARGS(&m_RTCmdList)));
    m_RTCmdList->SetName(L"RayTracing CommandList");
    //Create Compute CommandList
    D3DCall(m_Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_FrameObjects[0].pComputeCmdAllocator.Get(), nullptr, IID_PPV_ARGS(&m_ComputeCmdList)));
    m_ComputeCmdList->SetName(L"Compute CommandList");
    createFenceEvent();
    return true;
}
} // namespace For_Initialize

void MoveToNextFrame()
{
    // Schedule a Signal command in the queues.
    const UINT64 currentFenceValue = m_FenceValues[m_FrameIndex];
    D3DCall(m_GraphicsCmdQueue->Signal(m_Fence.Get(), currentFenceValue));
    //ThrowIfFailed(hr, "Failed to signal fence on Command Queue");

    D3DCall(m_ComputeCmdQueue->Signal(m_Fence.Get(), currentFenceValue));
    //ThrowIfFailed(hr, "Failed to signal fence on Command Queue");

    // Advance the frame index.
    m_FrameIndex = (m_FrameIndex + 1) % g_NumFrames;

    // Check to see if the next frame is ready to start.
    if (m_Fence->GetCompletedValue() < m_FenceValues[m_FrameIndex])
    {
        // Wait for the submitted work on the graphics queue to finish.
        D3DCall(m_Fence->SetEventOnCompletion(m_FenceValues[m_FrameIndex], m_FenceEvent));
        //ThrowIfFailed(hr, "Failed to set completion event on fence");
        WaitForSingleObjectEx(m_FenceEvent, INFINITE, FALSE);

        // Wait for the submitted work on the compute queue to finish.
        D3DCall(m_Fence->SetEventOnCompletion(m_FenceValues[m_FrameIndex], m_ComputeFenceEvent));
        //ThrowIfFailed(hr, "Failed to set completion event on fence");
        WaitForSingleObjectEx(m_ComputeFenceEvent, INFINITE, FALSE);
    }

    m_FenceValues[m_FrameIndex] = currentFenceValue + 1;
}

void WaitForGPU()
{
    // Schedule a Signal command in the queue.
    D3DCall(m_GraphicsCmdQueue->Signal(m_Fence.Get(), m_FenceValues[m_FrameIndex])/*, "Fialed to signal fence event on graphics command queue."*/);

    // Wait until the fence has been processed.
    D3DCall(m_Fence->SetEventOnCompletion(m_FenceValues[m_FrameIndex], m_FenceEvent)/*, "Failed to set completion event on fence."*/);
    WaitForSingleObjectEx(m_FenceEvent, INFINITE, FALSE);

    // Schedule a Signal command in the queue.
    D3DCall(m_ComputeCmdQueue->Signal(m_Fence.Get(), m_FenceValues[m_FrameIndex])/*, "Failed to signal fence event on graphics command queue."*/);

    // Wait until the fence has been processed.
    D3DCall(m_Fence->SetEventOnCompletion(m_FenceValues[m_FrameIndex], m_ComputeFenceEvent)/*, "Failed to set completion event on fence."*/);
    WaitForSingleObjectEx(m_ComputeFenceEvent, INFINITE, FALSE);

    // Increment the fence value for the current frame.
    m_FenceValues[m_FrameIndex]++;
}

void ExecuteComputeCommandList()
{
    D3DCall(m_ComputeCmdList->Close());

    ID3D12CommandList* ppCommandLists[]{
        m_ComputeCmdList.Get(),
    };

    m_GraphicsCmdQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    WaitForGPU();
}

void ResetComputeCommandList()
{
    D3DCall(m_ComputeCmdList->Reset(m_FrameObjects[m_FrameIndex].pComputeCmdAllocator.Get(), nullptr));
}

ComPtr<ID3D12Resource> createBuffer(ID3D12Device* pDevice, uint64_t size, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES initState, const D3D12_HEAP_PROPERTIES& heapProps)
{
    D3D12_RESOURCE_DESC bufDesc = {};
    bufDesc.Alignment = 0;
    bufDesc.DepthOrArraySize = 1;
    bufDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufDesc.Flags = flags;
    bufDesc.Format = DXGI_FORMAT_UNKNOWN;
    bufDesc.Height = 1;
    bufDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    bufDesc.MipLevels = 1;
    bufDesc.SampleDesc.Count = 1;
    bufDesc.SampleDesc.Quality = 0;
    bufDesc.Width = size;

    ComPtr<ID3D12Resource> pBuffer;
    D3DCall(pDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufDesc, initState, nullptr, IID_PPV_ARGS(&pBuffer)));
    return pBuffer;
}

bool Init()
{
    //Window 생성
    m_hWnd = Window::Create(g_windowName, 1920, 1200, WndProc);

    RECT clientRect{};
    GetClientRect(m_hWnd, &clientRect);
    m_Width = clientRect.right - clientRect.left;
    m_Height = clientRect.bottom - clientRect.top;

    m_Viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(m_Width), static_cast<float>(m_Height));
    m_ScissorRect = CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX);

    //DX12 초기화
    if (!InitDX())
        return false;

    //Window 화면 출력
    ::ShowWindow(m_hWnd, SW_SHOWDEFAULT);
    ::SetFocus(m_hWnd);

    return true;
}
void Destroy()
{
    WaitForGPU();

    ::DestroyWindow(m_hWnd);
    ::UnregisterClass(L"Anime Window", GetModuleHandle(NULL));
}

void BeginFrame()
{
    m_CmdList->RSSetViewports(1, &m_Viewport);
    m_CmdList->RSSetScissorRects(1, &m_ScissorRect);

}

void EndFrame()
{
    D3DCall(m_CmdList->Close());
    D3DCall(m_RTCmdList->Close());
    D3DCall(m_ComputeCmdList->Close());

    ID3D12CommandList* ppCommandLists[]{
        m_CmdList.Get(),
        m_RTCmdList.Get(),
        m_ComputeCmdList.Get(),
    };

    m_GraphicsCmdQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    WaitForGPU();
    
    auto hr = m_SwapChain->Present(0, 0);

    if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
    {
    #ifdef _DEBUG
        char buff[64] = {};
        sprintf_s(buff, "Device Lost on Present: Reason code 0x%08X\n", (hr == DXGI_ERROR_DEVICE_REMOVED) ? m_Device->GetDeviceRemovedReason() : hr);
        OutputDebugStringA(buff);
    #endif
    }

    MoveToNextFrame();

    D3DCall(m_FrameObjects[m_FrameIndex].pCmdAllocator->Reset());
    D3DCall(m_FrameObjects[m_FrameIndex].pRTCmdAllocator->Reset());
    D3DCall(m_FrameObjects[m_FrameIndex].pComputeCmdAllocator->Reset());

    D3DCall(m_ComputeCmdList->Reset(m_FrameObjects[m_FrameIndex].pComputeCmdAllocator.Get(), nullptr));
    D3DCall(m_CmdList->Reset(m_FrameObjects[m_FrameIndex].pCmdAllocator.Get(), nullptr));
    D3DCall(m_RTCmdList->Reset(m_FrameObjects[m_FrameIndex].pRTCmdAllocator.Get(), nullptr));
}

void ClearRenderTarget(ID3D12Resource* resource, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle, D3D12_RESOURCE_STATES stateBefore, math::vec4 clearColor)
{
    ResourceBarrier(m_CmdList.Get(), resource, stateBefore, D3D12_RESOURCE_STATE_RENDER_TARGET);
    float color[4] = { clearColor.r, clearColor.g, clearColor.b, clearColor.a };
    m_CmdList->ClearRenderTargetView(rtvHandle, color, 1, &m_ScissorRect);
    ResourceBarrier(m_CmdList.Get(), resource, D3D12_RESOURCE_STATE_RENDER_TARGET, stateBefore);
}

ComPtr<ID3D12Resource> CreateBuffer(uint64_t size, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES initState, D3D12_HEAP_PROPERTIES HeapProperty)
{
    return createBuffer(m_Device.Get(), size, flags, initState, HeapProperty);
}
void ResourceBarrier(ID3D12GraphicsCommandList* pCmdList, ID3D12Resource* pResource, D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter)
{
    resourceBarrier(pCmdList, pResource, stateBefore, stateAfter);
}
ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(uint32_t count, D3D12_DESCRIPTOR_HEAP_TYPE type, bool shaderVisible)
{
    return createDescriptorHeap(m_Device.Get(), count, type, shaderVisible);
}
HWND GetHWND()
{
    return m_hWnd;
}
ID3D12Device5* GetDevice()
{
    return m_Device.Get();
}
ID3D12GraphicsCommandList4* GetCommandList()
{
    return m_CmdList.Get();
}
ID3D12GraphicsCommandList4* GetComputeCommandList()
{
    return m_ComputeCmdList.Get();
}
ID3D12GraphicsCommandList4* GetRTCommandList()
{
    return m_RTCmdList.Get();
}
u32 GetWidth()
{
    return m_Width;
}
u32 GetHeight()
{
    return m_Height;
}
ID3D12Resource* GetBackBuffer()
{
    return m_FrameObjects[m_SwapChain->GetCurrentBackBufferIndex()].pSwapChainBuffer.Get();
}

void ExecuteRTCommandList()
{
    D3DCall(m_RTCmdList->Close());

    ID3D12CommandList* ppCommandLists[]{
        m_RTCmdList.Get(),
    };

    m_GraphicsCmdQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    WaitForGPU();
}

void ResetRTCommandList()
{
    D3DCall(m_RTCmdList->Reset(m_FrameObjects[m_FrameIndex].pRTCmdAllocator.Get(), nullptr));
}



D3D12_CPU_DESCRIPTOR_HANDLE& GetBackbufferRtvHandle()
{
    return m_FrameObjects[m_SwapChain->GetCurrentBackBufferIndex()].rtvHandle;
}

LRESULT __stdcall WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    default:
        return Input::Input_WindowProc(hWnd, msg, wParam, lParam);
    }
}
} // namespace Gfx

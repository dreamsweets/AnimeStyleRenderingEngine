#include "IMGUIManager.h"
#include "DX12/Gfx.h"

IMPLEMENT_SINGLE(IMGUIManager)

void IMGUIManager::Init(HWND hwnd, ID3D12Device* device, u32 numFrames)
{
	m_ImGuiHeap.Create(Gfx::GetDevice(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 100, true);

	IMGUI_CHECKVERSION();
	m_Context = ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable | ImGuiConfigFlags_ViewportsEnable;


	//io.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleViewports;
	//io.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleFonts;

	ImGui::StyleColorsDark();
	SetDarkThemeColors();
	// backend Setup
	if (!ImGui_ImplWin32_Init(hwnd))
	{
		throw;
	}
	if (!ImGui_ImplDX12_Init(
		device,
		numFrames,
		DXGI_FORMAT_R8G8B8A8_UNORM,
		m_ImGuiHeap, m_ImGuiHeap.hCPUHeapStart, m_ImGuiHeap.hGPUHeapStart))
	{
		throw;
	}
}

void IMGUIManager::Update()
{
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	ImGui::ShowDemoWindow();
	for (auto& [type, window] : m_mapWindow)
	{
		window->Update(0.f);
	}
	ImGui::Render();
}

void IMGUIManager::Render(ID3D12GraphicsCommandList* CmdList)
{
	ID3D12DescriptorHeap* heaps[] = { m_ImGuiHeap };
	CmdList->SetDescriptorHeaps(arraysize(heaps), heaps);
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), CmdList);

	ImGui::UpdatePlatformWindows();
	ImGui::RenderPlatformWindowsDefault();
}

ImGuiTexture IMGUIManager::AddTexture(ID3D12Resource* Texture, D3D12_SHADER_RESOURCE_VIEW_DESC& viewDesc)
{
	D3D12_CPU_DESCRIPTOR_HANDLE cpuhandle{};
	D3D12_GPU_DESCRIPTOR_HANDLE gpuhandle{};
	
	if (m_deqFreeID.empty())
	{
		int index = m_vecGPUHandle.size();
		
		cpuhandle = m_ImGuiHeap.hCPU(m_CurrentHeapSize);
		gpuhandle = m_ImGuiHeap.hGPU(m_CurrentHeapSize);
		++m_CurrentHeapSize;

		Gfx::GetDevice()->CreateShaderResourceView(Texture, &viewDesc, cpuhandle);
		m_vecGPUHandle.push_back(gpuhandle);
		return {gpuhandle, index};
	}

	u32 index = m_deqFreeID.front();
	m_deqFreeID.pop_front();
	cpuhandle = m_ImGuiHeap.hCPU(index);
	gpuhandle = m_ImGuiHeap.hGPU(index);

	Gfx::GetDevice()->CreateShaderResourceView(Texture, &viewDesc, cpuhandle);
	m_vecGPUHandle[index] = gpuhandle;

	return { gpuhandle, (int)index };
}

void IMGUIManager::RemoveTexture(D3D12_GPU_DESCRIPTOR_HANDLE handle, int index)
{
	ImGuiTexture tex{ handle, index };
	RemoveTexture(tex);

}

void IMGUIManager::RemoveTexture(ImGuiTexture& texture)
{
	if (texture.index == -1)
	{
		for (int i = 0; i < m_vecGPUHandle.size(); ++i)
		{
			if (m_vecGPUHandle[i].ptr == texture.handle.ptr)
			{
				m_vecGPUHandle[i].ptr = 0;
				m_deqFreeID.push_back(i);
			}
		}
		return;
	}

	if (texture.index >= m_vecGPUHandle.size()) return;

	m_vecGPUHandle[texture.index].ptr = 0;
	m_deqFreeID.push_back(texture.index);
}

void IMGUIManager::SetDarkThemeColors()
{
	auto& colors = ImGui::GetStyle().Colors;
	colors[ImGuiCol_WindowBg] = ImVec4{ 0.1f, 0.105f, 0.11f, 1.0f };

	// Headers
	colors[ImGuiCol_Header] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };
	colors[ImGuiCol_HeaderHovered] = ImVec4{ 0.3f, 0.305f, 0.31f, 1.0f };
	colors[ImGuiCol_HeaderActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

	// Buttons
	colors[ImGuiCol_Button] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };
	colors[ImGuiCol_ButtonHovered] = ImVec4{ 0.3f, 0.305f, 0.31f, 1.0f };
	colors[ImGuiCol_ButtonActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

	// Frame BG
	colors[ImGuiCol_FrameBg] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };
	colors[ImGuiCol_FrameBgHovered] = ImVec4{ 0.3f, 0.305f, 0.31f, 1.0f };
	colors[ImGuiCol_FrameBgActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

	// Tabs
	colors[ImGuiCol_Tab] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
	colors[ImGuiCol_TabHovered] = ImVec4{ 0.38f, 0.3805f, 0.381f, 1.0f };
	colors[ImGuiCol_TabActive] = ImVec4{ 0.28f, 0.2805f, 0.281f, 1.0f };
	colors[ImGuiCol_TabUnfocused] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
	colors[ImGuiCol_TabUnfocusedActive] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };

	// Title
	colors[ImGuiCol_TitleBg] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
	colors[ImGuiCol_TitleBgActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
	colors[ImGuiCol_TitleBgCollapsed] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
}

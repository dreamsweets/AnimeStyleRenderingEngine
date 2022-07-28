#pragma once
#include "imgui.h"
#include "imgui_impl_dx12.h"
#include "imgui_impl_win32.h"
#include "imgui/Window/IMGUIWindow.h"
#include "DX12/DX12Common.h"

#undef FindWindow

struct ImGuiTexture {
	D3D12_GPU_DESCRIPTOR_HANDLE handle{};
	int index = -1;
};

class IMGUIManager {
	friend class Helper;
public:
	void Init(HWND hwnd, ID3D12Device* device, u32 numFrames);
	
	void Update();

	//Render ȣ�� ���� SrvHeap�� ���� ���ε� �Ǿ� �־�� ��!
	void Render(ID3D12GraphicsCommandList* CmdList);

	ID3D12DescriptorHeap* GetImGuiDescHeap() { return m_ImGuiHeap; }

	template<typename T>
	T* AddWindow() {
		T* Window = FindWindow<T>();
		if (Window)
			return Window;
		Window = new T;
		
		Window->Init();

		m_mapWindow.insert({ typeid(T), Window });
		
		return Window;
	}

	template<typename T>
	T* FindWindow() {
		auto iter = m_mapWindow.find(typeid(T));
		if (iter == m_mapWindow.end())
			return nullptr;
		return (T*)iter->second;
	}

	ImGuiTexture AddTexture(ID3D12Resource* Texture, D3D12_SHADER_RESOURCE_VIEW_DESC& viewDesc);
	void RemoveTexture(D3D12_GPU_DESCRIPTOR_HANDLE handle, int index = -1);
	void RemoveTexture(ImGuiTexture& texture);

private:
	void SetDarkThemeColors();

private:
	ImGuiContext* m_Context;
	std::unordered_map<std::type_index, IMGUIWindow*> m_mapWindow;
	DescriptorHeapWrapper m_ImGuiHeap;
	std::vector<D3D12_GPU_DESCRIPTOR_HANDLE> m_vecGPUHandle;
	u32 m_CurrentHeapSize{ 1 }; // 0�� ������ ImGui���� Text ��� �����͸� ����µ� ���� ������ 1���� ������.
	std::deque<u32> m_deqFreeID;
	DECLARE_SINGLE(IMGUIManager)
};
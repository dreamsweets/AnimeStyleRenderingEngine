#pragma once
#include "imgui/Window/IMGUIWindow.h"
#include "Resources/Texture/Texture.h"
#include "imgui/IMGUIManager.h"

struct TextureShowInfo {
	std::string title;
	ImGuiTexture imguiTexture;
	math::vec2 computedSize;
};

class TextureViewer : public IMGUIWindow
{
public:
	static constexpr u32 g_ImageWidth = 500;
	virtual bool Init() override;
	virtual void Update(float DeltaTime) override;

	void AddTexture(std::string title, ID3D12Resource* texture);
	void AddDepthTexture(std::string title, ID3D12Resource* texture);
private:
	std::vector<TextureShowInfo> m_textures;
};